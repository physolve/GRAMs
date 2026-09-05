#include "SupplyTaskTree.h"

#include <QDebug>
#include <memory>

using namespace QtTaskTree;

QString supplyStopName(SupplyStop stop)
{
    switch (stop) {
    case SupplyStop::None:            return QStringLiteral("не завершён");
    case SupplyStop::TimeElapsed:     return QStringLiteral("истекло время напуска");
    case SupplyStop::PressureReached: return QStringLiteral("достигнут предел давления");
    case SupplyStop::QuartileVeto:    return QStringLiteral("условие quartile запретило продолжение");
    case SupplyStop::Cancelled:       return QStringLiteral("остановлен оператором");
    }
    return QStringLiteral("неизвестно");
}

namespace {

using TickerTask = QCustomTask<PausableTicker>;

// Инструментация узла: аддитивная, control-flow не меняет.
ExecutableItem withNode(const SupplyTreeContext& ctx, SupplyNode id, ExecutableItem inner)
{
    GroupItems items { sequential };
    items << onGroupSetup([ctx, id] {
        if (ctx.onNode) ctx.onNode(id, NodeState::Running);
    });
    items << inner;
    items << onGroupDone([ctx, id](DoneWith w) {
        if (ctx.onNode) ctx.onNode(id, doneToNode(w));
    });
    return Group(items);
}

// Прерываемая выдержка. Именно этим заменён QThread::msleep из легаси:
// спящий поток не умеет замирать по PauseBus и не даёт точки отмены.
ExecutableItem pausableDelayMs(const SupplyTreeContext& ctx, int ms)
{
    return TickerTask([ctx, ms](PausableTicker& t) {
        t.intervalMs = qMax(1, ms);
        t.pauseBus   = ctx.pauseBus;
        t.isComplete = [](int elapsed) { return elapsed >= 1; };
    });
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Рецепт «Напуск»
// ═════════════════════════════════════════════════════════════════════════════

Group buildSupplyRecipe(const SupplyTreeContext& ctx, const Storage<SupplyRunState>& st)
{
    // Итог накопления живёт здесь, а не в Storage: пишется он из таймерного
    // контекста тикера, где Storage недоступен, а читается в done-хендлере.
    auto stop      = std::make_shared<SupplyStop>(SupplyStop::None);
    auto elapsedMs = std::make_shared<int>(0);

    return Group {
        st,
        sequential,

        // ── Гейт: тракт откачан и клапан порта закрыт ─────────────────────────
        //
        // Проверка не формальная: напуск на неоткачанный тракт смешивает
        // остаточный газ с напускаемым, и состав смеси после этого неизвестен.
        // Прогон при этом выглядит успешным — отсюда явный отказ с причиной.
        withNode(ctx, SupplyNode::Gate, QSyncTask([st, ctx]() -> bool {
            if (ctx.port.isEmpty()) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("Напуск: не задан клапан порта"));
                return false;
            }
            if (!ctx.gateCheck)
                return true;

            if (!ctx.pressureStorageBar) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Напуск: датчик накопителя не задан — проверить, "
                        "что тракт откачан, нечем"));
                return false;
            }
            const Reading p = ctx.pressureStorageBar();
            if (!p.isValid()) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Напуск: показание накопителя недостоверно (%1)")
                            .arg(qualityName(p.quality)));
                return false;
            }
            if (p.value > ctx.gateMaxStartBar) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Напуск: тракт не откачан — накопитель %1 бар при пороге %2 бар. "
                        "Выполните «Вакуум» перед напуском")
                            .arg(p.value, 0, 'g', 3)
                            .arg(ctx.gateMaxStartBar));
                return false;
            }
            // Клапан порта обязан быть закрыт: открытый порт означает
            // бесконтрольный поток ещё до старта отсчёта.
            if (ctx.confirmValve && !ctx.confirmValve(false, ctx.port)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Напуск: клапан порта %1 не подтверждён закрытым").arg(ctx.port));
                return false;
            }
            return true;
        })),

        // ── Подготовка: beginAction, нулевая точка, прогрев буфера ────────────
        withNode(ctx, SupplyNode::Prepare, Group {
            sequential,
            QSyncTask([st, ctx]() -> bool {
                if (ctx.setActionMode)
                    ctx.setActionMode(true);
                st->actionOpen = true;
                if (ctx.fillSupplyData)
                    ctx.fillSupplyData(0);
                if (ctx.onLabel)
                    ctx.onLabel(QStringLiteral("Напуск: подготовка, порт %1").arg(ctx.port));
                return true;
            }),
            // Легаси: msleep(1000) → fastBufferRead() → msleep(100).
            pausableDelayMs(ctx, ctx.preReadDelayMs),
            QSyncTask([ctx]() -> bool {
                if (ctx.fastBufferRead)
                    ctx.fastBufferRead();
                return true;
            }),
            pausableDelayMs(ctx, ctx.postReadDelayMs)
        }),

        // ── Открытие клапана порта ────────────────────────────────────────────
        withNode(ctx, SupplyNode::Open, QSyncTask([st, ctx]() -> bool {
            if (!ctx.setValve || !ctx.setValve(true, ctx.port)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("Напуск: клапан порта %1 не открылся")
                                      .arg(ctx.port));
                return false;
            }
            st->portOpen = true;
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Напуск: порт %1 открыт, предел %2 бар")
                                .arg(ctx.port).arg(ctx.pressureLimitBar));
            return true;
        })),

        // ── Накопление ────────────────────────────────────────────────────────
        //
        // Условия остановки различаются, и все три штатные. Легаси знал только
        // время и checkSupplyAction(); предел давления в цикле не проверялся
        // вовсе, поэтому «напустить до давления» было недоступно.
        withNode(ctx, SupplyNode::Run,
            TickerTask([ctx, stop, elapsedMs](PausableTicker& t) {
                *stop = SupplyStop::None;
                *elapsedMs = 0;
                t.intervalMs = qMax(1, ctx.tickIntervalMs);
                t.pauseBus   = ctx.pauseBus;
                // ВАЖНО: isComplete вызывается из QTimer ВНЕ активного контекста
                // дерева — обращаться отсюда к Tasking::Storage нельзя. Поэтому
                // итог складывается в shared-состояние, а в Storage его
                // переносит done-хендлер группы, который уже в контексте дерева.
                t.isComplete = [ctx, stop, elapsedMs](int ticks) -> bool {
                    *elapsedMs = ticks * qMax(1, ctx.tickIntervalMs);

                    if (ctx.fastBufferRead)
                        ctx.fastBufferRead();
                    if (ctx.runSupplyAction)
                        ctx.runSupplyAction();
                    if (ctx.appendSupplyData && !ctx.appendSupplyData(*elapsedMs))
                        qWarning() << "[Напуск] appendSupplyData отклонил точку на"
                                   << *elapsedMs << "мс";

                    const Reading p = ctx.pressureStorageBar
                                          ? ctx.pressureStorageBar()
                                          : Reading(0.0, Quality::NoResponse);
                    if (ctx.onProgress)
                        ctx.onProgress(*elapsedMs, p);

                    // Оператор трогал клапаны вручную — легаси перезапрашивал
                    // режим действия и продолжал. Поведение сохранено.
                    if (ctx.actionInterrupted && ctx.actionInterrupted()) {
                        qWarning() << "[Напуск] Клапаны трогали вручную — "
                                      "перезапрашиваем режим действия";
                        if (ctx.setActionMode)
                            ctx.setActionMode(true);
                    }

                    if (p.isValid() && p.value >= ctx.pressureLimitBar) {
                        *stop = SupplyStop::PressureReached;
                        return true;
                    }
                    if (ctx.checkSupplyAction && !ctx.checkSupplyAction()) {
                        *stop = SupplyStop::QuartileVeto;
                        return true;
                    }
                    if (*elapsedMs >= ctx.openTimeMs) {
                        *stop = SupplyStop::TimeElapsed;
                        return true;
                    }
                    return false;
                };
            })),

        // ── Закрытие ──────────────────────────────────────────────────────────
        //
        // onGroupDone всей группы: клапан закрывается на ЛЮБОМ исходе, включая
        // отмену и ошибку. Именно этого не давал легаси-цикл — там точка, в
        // которой прогон прервётся с открытым клапаном, была не определена.
        onGroupDone([st, ctx, stop, elapsedMs](DoneWith w) {
            st->stop      = *stop;
            st->elapsedMs = *elapsedMs;
            if (st->portOpen) {
                if (ctx.setValve)
                    ctx.setValve(false, ctx.port);
                st->portOpen = false;
            }
            if (st->actionOpen) {
                if (ctx.setActionMode)
                    ctx.setActionMode(false);
                st->actionOpen = false;
            }
            if (ctx.onNode)
                ctx.onNode(SupplyNode::Close,
                           w == DoneWith::Success ? NodeState::Success
                         : w == DoneWith::Cancel  ? NodeState::Cancelled
                                                  : NodeState::Error);
            // Результаты сохраняются и при отмене: собранные до остановки
            // точки — тоже данные прогона, терять их незачем.
            if (ctx.saveSupplyData)
                ctx.saveSupplyData();

            if (w == DoneWith::Cancel)
                st->stop = SupplyStop::Cancelled;
            if (ctx.onFinished)
                ctx.onFinished(st->stop);
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Напуск завершён: %1")
                                .arg(supplyStopName(st->stop)));
        })
    };
}

Group buildSupplyRecipe(const SupplyTreeContext& ctx)
{
    return buildSupplyRecipe(ctx, Storage<SupplyRunState>());
}
