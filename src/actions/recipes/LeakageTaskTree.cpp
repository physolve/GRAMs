#include "LeakageTaskTree.h"

#include <QDebug>
#include <memory>

using namespace QtTaskTree;

QString leakageStopName(LeakageStop stop)
{
    switch (stop) {
    case LeakageStop::None:            return QStringLiteral("не завершён");
    case LeakageStop::DurationElapsed: return QStringLiteral("отработана длительность");
    case LeakageStop::TargetReached:   return QStringLiteral("достигнут целевой перепад");
    case LeakageStop::SensorInvalid:   return QStringLiteral("показание датчика недостоверно");
    case LeakageStop::Cancelled:       return QStringLiteral("остановлен оператором");
    }
    return QStringLiteral("неизвестно");
}

namespace {

using TickerTask = QCustomTask<PausableTicker>;

ExecutableItem withNode(const LeakageTreeContext& ctx, LeakageNode id, ExecutableItem inner)
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

Reading readOr(const std::function<Reading()>& seam)
{
    return seam ? seam() : Reading(0.0, Quality::NoResponse);
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Рецепт «Натекание»
// ═════════════════════════════════════════════════════════════════════════════

Group buildLeakageRecipe(const LeakageTreeContext& ctx, const Storage<LeakageRunState>& st)
{
    // Пишется из таймерного контекста тикера, где Storage недоступен; читается
    // в done-хендлере группы, который уже в контексте дерева.
    auto stop       = std::make_shared<LeakageStop>(LeakageStop::None);
    auto elapsedSec = std::make_shared<int>(0);
    auto deltaBar   = std::make_shared<double>(0.0);
    auto startBar   = std::make_shared<double>(0.0);

    return Group {
        st,
        sequential,

        // ── Гейт: накопитель заряжен, камера откачана ─────────────────────────
        withNode(ctx, LeakageNode::Gate, QSyncTask([ctx]() -> bool {
            if (ctx.valve.isEmpty()) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("Натекание: не задан дозирующий клапан"));
                return false;
            }
            if (!ctx.gateCheck)
                return true;

            const Reading s = readOr(ctx.pressureStorageBar);
            const Reading r = readOr(ctx.pressureReactionBar);
            if (!s.isValid() || !r.isValid()) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Натекание: недостоверные показания — накопитель %1, камера %2")
                            .arg(qualityName(s.quality)).arg(qualityName(r.quality)));
                return false;
            }
            // Пустой накопитель: пропускать через дозирующий клапан нечего,
            // прогон получится пустым и это выяснится только по файлу.
            if (s.value < ctx.gateMinStorageBar) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Натекание: накопитель не заряжен — %1 бар при минимуме %2 бар. "
                        "Выполните «Напуск» перед натеканием")
                            .arg(s.value, 0, 'g', 3).arg(ctx.gateMinStorageBar));
                return false;
            }
            // Неоткачанная камера: Δp считался бы от неизвестного уровня.
            if (r.value > ctx.gateMaxReactionBar) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Натекание: камера не откачана — %1 бар при пороге %2 бар. "
                        "Выполните «Вакуум» перед натеканием")
                            .arg(r.value, 0, 'g', 3).arg(ctx.gateMaxReactionBar));
                return false;
            }
            if (ctx.confirmValve && !ctx.confirmValve(false, ctx.valve)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral(
                        "Натекание: дозирующий клапан %1 не подтверждён закрытым")
                            .arg(ctx.valve));
                return false;
            }
            return true;
        })),

        // ── Подготовка: сбор данных и начальная точка ─────────────────────────
        withNode(ctx, LeakageNode::Prepare, QSyncTask([st, ctx, startBar]() -> bool {
            if (ctx.setMeasureMode)
                ctx.setMeasureMode(true);
            st->measureOn = true;

            const Reading s = readOr(ctx.pressureStorageBar);
            const Reading r = readOr(ctx.pressureReactionBar);
            const double  t = ctx.temperatureReactionK ? ctx.temperatureReactionK() : 0.0;
            *startBar = s.value;

            if (ctx.leakageStart)
                ctx.leakageStart(s.value, r.value, t);
            st->calcStarted = true;

            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Натекание: старт, накопитель %1 бар, камера %2 бар")
                                .arg(s.value, 0, 'g', 3).arg(r.value, 0, 'g', 3));
            return true;
        })),

        // ── Открытие дозирующего клапана ──────────────────────────────────────
        withNode(ctx, LeakageNode::Open, QSyncTask([st, ctx]() -> bool {
            if (!ctx.setValve || !ctx.setValve(true, ctx.valve)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("Натекание: дозирующий клапан %1 не открылся")
                                      .arg(ctx.valve));
                return false;
            }
            st->valveOpen = true;
            if (ctx.leakageSetOpen)
                ctx.leakageSetOpen(true);
            return true;
        })),

        // ── Измерение ─────────────────────────────────────────────────────────
        withNode(ctx, LeakageNode::Run,
            TickerTask([ctx, stop, elapsedSec, deltaBar, startBar](PausableTicker& t) {
                *stop = LeakageStop::None;
                *elapsedSec = 0;
                *deltaBar = 0.0;
                t.intervalMs = qMax(1, ctx.tickIntervalMs);
                t.pauseBus   = ctx.pauseBus;
                // isComplete работает из QTimer вне контекста дерева: Storage
                // отсюда недоступен, поэтому только shared-состояние и швы.
                t.isComplete = [ctx, stop, elapsedSec, deltaBar, startBar](int ticks) -> bool {
                    // PausableTicker::start() вызывает isComplete(0) как
                    // предварительную проверку «а не готово ли уже». Тик с
                    // номером 0 — не такт измерения, работы в нём быть не должно,
                    // иначе точек окажется на одну больше заданного числа.
                    if (ticks == 0)
                        return false;
                    *elapsedSec = ticks;

                    const Reading s = readOr(ctx.pressureStorageBar);
                    const Reading r = readOr(ctx.pressureReactionBar);
                    const double  k = ctx.temperatureReactionK ? ctx.temperatureReactionK() : 0.0;

                    // Недостоверное показание — не повод молча продолжать: весь
                    // смысл прогона в измеренном расходе.
                    if (!s.isValid() || !r.isValid()) {
                        *stop = LeakageStop::SensorInvalid;
                        return true;
                    }

                    if (ctx.leakageAdd)
                        ctx.leakageAdd(s.value, r.value, k);

                    *deltaBar = *startBar - s.value;
                    if (ctx.onProgress)
                        ctx.onProgress(*elapsedSec, s, r, *deltaBar);

                    if (ctx.targetDeltaBar > 0.0 && *deltaBar >= ctx.targetDeltaBar) {
                        *stop = LeakageStop::TargetReached;
                        return true;
                    }
                    if (*elapsedSec >= ctx.durationSec) {
                        *stop = LeakageStop::DurationElapsed;
                        return true;
                    }
                    return false;
                };
            })),

        // ── Закрытие ──────────────────────────────────────────────────────────
        //
        // На любом исходе: клапан закрыт, расчёт закрыт, файл сохранён, сбор
        // данных выключен. Порядок обратный подготовке.
        onGroupDone([st, ctx, stop, elapsedSec, deltaBar](DoneWith w) {
            st->stop       = *stop;
            st->elapsedSec = *elapsedSec;
            st->deltaBar   = *deltaBar;

            if (st->valveOpen) {
                if (ctx.setValve)
                    ctx.setValve(false, ctx.valve);
                if (ctx.leakageSetOpen)
                    ctx.leakageSetOpen(false);
                st->valveOpen = false;
            }
            if (st->calcStarted) {
                if (ctx.leakageEnd)
                    ctx.leakageEnd();
                // Сохраняем и при отмене: измеренные до остановки точки —
                // тоже результат прогона.
                if (ctx.leakageSave)
                    ctx.leakageSave();
                st->calcStarted = false;
            }
            if (st->measureOn) {
                if (ctx.setMeasureMode)
                    ctx.setMeasureMode(false);
                st->measureOn = false;
            }
            if (ctx.onNode)
                ctx.onNode(LeakageNode::Close, doneToNode(w));

            if (w == DoneWith::Cancel)
                st->stop = LeakageStop::Cancelled;
            if (ctx.onFinished)
                ctx.onFinished(st->stop);
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Натекание завершено: %1, Δp %2 бар за %3 с")
                                .arg(leakageStopName(st->stop))
                                .arg(st->deltaBar, 0, 'g', 3)
                                .arg(st->elapsedSec));
        })
    };
}

Group buildLeakageRecipe(const LeakageTreeContext& ctx)
{
    return buildLeakageRecipe(ctx, Storage<LeakageRunState>());
}
