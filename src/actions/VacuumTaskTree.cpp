#include "VacuumTaskTree.h"

#include <QDebug>
#include <memory>

using namespace QtTaskTree;

// ═════════════════════════════════════════════════════════════════════════════
// PausableTicker
// ═════════════════════════════════════════════════════════════════════════════

PausableTicker::PausableTicker(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &PausableTicker::tick);
}

void PausableTicker::start()
{
    if (isComplete && isComplete(0)) {
        emit done(true);
        return;
    }
    if (pauseBus) {
        connect(pauseBus, &PauseBus::paused,  this, [this] { m_timer.stop(); });
        connect(pauseBus, &PauseBus::resumed, this, [this] { m_timer.start(intervalMs); });
        if (pauseBus->isPaused())
            return;  // стартуем замороженными, ждём resumed()
    }
    m_timer.start(intervalMs);
}

void PausableTicker::tick()
{
    ++m_elapsed;
    if (onTick)
        onTick(m_elapsed);
    if (!isComplete || isComplete(m_elapsed)) {
        m_timer.stop();
        emit done(true);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// PumpRateWatchdog — проверка dP/dt после открытия К176 (REQ-020/076)
// ═════════════════════════════════════════════════════════════════════════════

PumpRateWatchdog::PumpRateWatchdog(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &PumpRateWatchdog::tick);
}

void PumpRateWatchdog::start()
{
    if (pauseBus) {
        connect(pauseBus, &PauseBus::paused,  this, [this] { m_timer.stop(); });
        connect(pauseBus, &PauseBus::resumed, this, [this] {
            if (!m_awaitingOperator) m_timer.start(intervalMs);
        });
    }
    if (operatorBus)
        connect(operatorBus, &OperatorBus::decisionReceived, this,
                [this](OperatorBus::Decision d) { onDecision(int(d)); });
    beginWindow();
}

void PumpRateWatchdog::beginWindow()
{
    m_elapsed = 0;
    m_p0 = pressurePa ? pressurePa() : 0.0;
    m_awaitingOperator = false;
    if (pauseBus && pauseBus->isPaused())
        return;                         // стартуем замороженными, ждём resume
    m_timer.start(intervalMs);
}

void PumpRateWatchdog::tick()
{
    ++m_elapsed;
    if (m_elapsed < windowSec)
        return;
    m_timer.stop();
    const double p1   = pressurePa ? pressurePa() : 0.0;
    const double drop = m_p0 - p1;      // падение давления за окно
    if (drop >= minDropPa) {            // давление падает → откачка идёт
        emit done(true);
        return;
    }
    // Откачка не идёт (dP/dt мала). Без OperatorBus — безопасный отказ.
    if (!operatorBus) {
        emit done(false);
        return;
    }
    m_awaitingOperator = true;
    operatorBus->request(1, QStringLiteral(
        "Процесс откачки не происходит: скорость откачки мала (dP/dt меньше порога).\n"
        "«Продолжить» — повторить проверку откачки; «Остановить» — стоп режима."));
}

void PumpRateWatchdog::onDecision(int d)
{
    if (!m_awaitingOperator)
        return;
    m_awaitingOperator = false;
    if (d == OperatorBus::Continue)
        beginWindow();                  // повторить проверку dP/dt
    else
        emit done(false);               // Стоп → ошибка → останов режима
}

// ═════════════════════════════════════════════════════════════════════════════
// Примитивы рецепта
// ═════════════════════════════════════════════════════════════════════════════

using TickerTask    = QCustomTask<PausableTicker>;
using PumpRateTask  = QCustomTask<PumpRateWatchdog>;

namespace {

// ── Наблюдение за узлами развёртки (аддитивно, control-flow не меняет) ─────────
NodeState doneToNode(DoneWith w)
{
    switch (w) {
    case DoneWith::Success: return NodeState::Success;
    case DoneWith::Cancel:  return NodeState::Cancelled;
    default:                return NodeState::Error;
    }
}

// Оборачивает узел парой onGroupSetup(Running)/onGroupDone(final) для монитора.
// Для НЕ-пропускаемых узлов (Condition, F1Relief, F2BlockC, F2_ReliefMid,
// F4Pumpdown). Пропускаемые узлы эмитят Skipped inline у себя (см. skipUnlessNode
// и стражи s3Group/secondTractGroup).
ExecutableItem withNode(const VacuumTreeContext& ctx, VacuumNode id, ExecutableItem inner)
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

// ── Шаблон условия №3: проверка состояния (skip-предикат) ─────────────────────
// Легаси 21XXX-страж с false→goto_N, пропускающим группу шагов целиком.
// Структурный аналог goto: при !enabled группа завершается StopWithSuccess.
Group skipUnless(bool enabled, const GroupItems& body)
{
    GroupItems items { sequential };
    items << onGroupSetup([enabled]() -> SetupResult {
        return enabled ? SetupResult::Continue : SetupResult::StopWithSuccess;
    });
    items << body;
    return Group(items);
}

// Как skipUnless, но с наблюдением узла: !enabled → Skipped; иначе Running и
// финальное состояние в done. `enabled` фиксирован на время сборки рецепта,
// поэтому done-гард `if (enabled)` не эмитит терминал для пропущенного узла.
Group skipUnlessNode(const VacuumTreeContext& ctx, VacuumNode id,
                     bool enabled, const GroupItems& body)
{
    GroupItems items { sequential };
    items << onGroupSetup([ctx, id, enabled]() -> SetupResult {
        if (!enabled) {
            if (ctx.onNode) ctx.onNode(id, NodeState::Skipped);
            return SetupResult::StopWithSuccess;
        }
        if (ctx.onNode) ctx.onNode(id, NodeState::Running);
        return SetupResult::Continue;
    });
    items << body;
    items << onGroupDone([ctx, id, enabled](DoneWith w) {
        if (enabled && ctx.onNode) ctx.onNode(id, doneToNode(w));
    });
    return Group(items);
}

// Мгновенное открытие клапана с фиксацией в Storage. Отказ (Security
// заблокировал) => ошибка группы: остаток повтора пропускается, done-хендлеры
// закрывают уже открытое. Легаси игнорировал результат записи DO — здесь
// поведение ужесточено сознательно (см. docs/regimes/vacuum.md, неоднозначность №3).
ExecutableItem openValveTask(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx,
                             const QString& valve,
                             bool VacuumRunState::*openFlag)
{
    return QSyncTask([st, ctx, valve, openFlag]() -> bool {
        if (!ctx.setValve || !ctx.setValve(true, valve))
            return false;
        (*st).*openFlag = true;
        return true;
    });
}

// Пауз-совместимая выдержка на seconds тиков с накоплением elapsed в Storage
// и прогрессом в UI. Storage трогаем только в setup/done (активный контекст),
// из тиков таймера — никогда.
ExecutableItem pausableDelay(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx,
                             int seconds,
                             const RepeatIterator& iter)
{
    return TickerTask(
        [st, ctx, seconds, iter](PausableTicker& t) {
            const int base   = st->elapsedSec;
            const int repeat = int(iter.iteration());
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            t.isComplete = [seconds](int elapsed) { return elapsed >= seconds; };
            if (ctx.onProgress) {
                t.onTick = [ctx, base, repeat](int elapsed) {
                    ctx.onProgress(base + elapsed, repeat);
                };
            }
        },
        [st](const PausableTicker& t, DoneWith) {
            st->elapsedSec += t.elapsed();
        });
}

// Пауза-выдержка после каждого действия рецепта (perActionPauseMs). Даёт
// наблюдаемый темп разворачивания в песочнице. НЕ трогает Storage (не копит
// elapsedSec) и НЕ шлёт onProgress, поэтому не влияет на dry-run-ассерты тестов.
// При ms <= 0 возвращает nullItem — структура дерева не меняется.
GroupItem settlePauseMs(const VacuumTreeContext& ctx, int ms)
{
    if (ms <= 0)
        return nullItem;
    return TickerTask([ctx, ms](PausableTicker& t) {
        t.intervalMs = ms;
        t.pauseBus   = ctx.pauseBus;
        t.isComplete = [](int elapsed) { return elapsed >= 1; };
    });
}

// Задержка после действия конкретного узла: индивидуальное значение из
// stepPauseMs (таблица «для КАЖДОГО действия»), иначе глобальный perActionPauseMs.
GroupItem settlePauseFor(const VacuumTreeContext& ctx, VacuumNode node)
{
    return settlePauseMs(ctx, ctx.stepPauseMs.value(int(node), ctx.perActionPauseMs));
}

// Обобщённая пауза (глобальный дефолт) — для точек без своего узла.
GroupItem settlePause(const VacuumTreeContext& ctx)
{
    return settlePauseMs(ctx, ctx.perActionPauseMs);
}

// ── Ф1/Ф2: сброс-триплет К118 (s2–s4 и s15–s17: 21000 → 11118 time_5000 → 10118)
//
// Шаблон условия №1 — страж ДВ_СБР: мгновенный, без fault.
// Легаси-предикат (GramQt Conditions.cpp:284-285):
//   if (m_vir_B->GetPdata() > m_DB_SBR_LIM) return true;
// false → goto_3 (CSV s2): триплет пропускается целиком, это не ошибка.
//
// Инвариант 1: закрытие К118 гарантировано при ЛЮБОМ исходе, включая cancel
// посреди выдержки — onGroupDone с CallDoneFlag::Always (по умолчанию)
// срабатывает и при Cancel; флаг k118Open подавляет лишнее закрытие после
// StopWithSuccess-пропуска.
ExecutableItem reliefTriplet(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx,
                             const RepeatIterator& iter)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            const double pB = ctx.pressureB ? ctx.pressureB() : 0.0;
            if (pB <= ctx.dbSbrLim)
                return SetupResult::StopWithSuccess;       // s2: сброс не нужен
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K118))
                return SetupResult::StopWithError;         // s3: открытие заблокировано
            st->k118Open = true;
            ++st->reliefCount;
            return SetupResult::Continue;
        }),
        pausableDelay(st, ctx, ctx.reliefDwellSec, iter),  // s3: выдержка time_5000
        onGroupDone([st, ctx](DoneWith) {                  // s4: 10118
            if (st->k118Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K118);
                st->k118Open = false;
            }
        })
    };
}

// ── Condition-фаза повтора ────────────────────────────────────────────────────
// Семантика RegimeWorkerBase: "none"/time<=0 — подтверждение сразу; "time" —
// ожидание conditionTimeSec; "temp" — ожидание T<=target с таймаут-фолбэком.
ExecutableItem conditionPhase(const VacuumTreeContext& ctx, const RepeatIterator& iter)
{
    const bool immediate =
        (ctx.conditionType == QLatin1String("none")) || ctx.conditionTimeSec <= 0;

    GroupItems items { sequential };

    if (!immediate) {
        items << TickerTask([ctx, iter](PausableTicker& t) {
            const int repeat = int(iter.iteration());
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            if (ctx.conditionType == QLatin1String("temp")) {
                t.isComplete = [ctx](int elapsed) {
                    if (ctx.conditionTemp
                        && ctx.conditionTemp() <= ctx.conditionTargetTemp)
                        return true;
                    return elapsed >= ctx.conditionTimeSec;  // таймаут-фолбэк
                };
            } else { // "time"
                t.isComplete = [ctx](int elapsed) {
                    return elapsed >= ctx.conditionTimeSec;
                };
            }
            if (ctx.onConditionProgress) {
                t.onTick = [ctx, repeat](int elapsed) {
                    ctx.onConditionProgress(elapsed, repeat);
                };
            }
        });
    }

    items << onGroupDone([ctx, iter](DoneWith w) {
        if (w == DoneWith::Success && ctx.onConditionDone)
            ctx.onConditionDone(int(iter.iteration()));
    });

    return Group(items);
}

// ── Ф2 «Блок C» (s5–s19), параметризован под Ф6 (инвариант 4) ─────────────────
ExecutableItem buildBlockC(const Storage<VacuumRunState>& st,
                           const VacuumTreeContext& ctx,
                           const RepeatIterator& iter,
                           const BlockCOptions& opts)
{
    // s5–s10: подключение RK300. Страж s5 (21005, легаси cond_phase_5 =
    // flagIncludeRK300, Conditions.cpp:296-304; здесь инверсия — skipRK300),
    // false → goto_6: пропуск всей подгруппы. При проходе 1 после подключения —
    // сброс-триплет (s7–s9) и штатное закрытие (s10, 10135); при проходе 2
    // (Ф6) баллон остаётся открытым, но при cancel/error закрывается всё равно
    // (безопасное состояние).
    Group s3Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            if (ctx.skipRK300) {
                if (ctx.onNode) ctx.onNode(VacuumNode::F2_RK300, NodeState::Skipped);
                return SetupResult::StopWithSuccess;       // s5: false → goto_6
            }
            if (ctx.onNode) ctx.onNode(VacuumNode::F2_RK300, NodeState::Running);
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K135))
                return SetupResult::StopWithError;         // s6: 11135
            st->s3Open = true;
            return SetupResult::Continue;
        }),
        opts.reliefAndClose ? GroupItem(reliefTriplet(st, ctx, iter))  // s7–s9
                            : nullItem,
        // Держим S3 открытым свою настраиваемую задержку перед закрытием, иначе
        // при пропущенном сбросе (P ≤ порога) S3 открывается и тут же
        // закрывается — «мигание» RK300 на стенде.
        settlePauseFor(ctx, VacuumNode::F2_RK300),
        onGroupDone([st, ctx, opts](DoneWith w) {          // s10: 10135
            if (!ctx.skipRK300 && ctx.onNode)
                ctx.onNode(VacuumNode::F2_RK300, doneToNode(w));
            if (st->s3Open && (opts.reliefAndClose || w != DoneWith::Success)) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K135);
                st->s3Open = false;
            }
        })
    };

    GroupItems items { sequential };
    items << s3Group;
    items << settlePause(ctx);

    // s11–s12: страж RK10 (21003, cond_phase_3; false → goto_2 = пропустить
    // ровно строку открытия) → 11131. s13–s14: аналогично RK50 → 11133.
    // Стражи независимы и последовательны (не вложены): RK50 проверяется
    // даже когда RK10 пропущен — так в CSV.
    items << skipUnlessNode(ctx, VacuumNode::F2_RK10, !ctx.skipRK10,
                        { openValveTask(st, ctx, VacuumValve::K131, &VacuumRunState::s1Open) });
    items << settlePauseFor(ctx, VacuumNode::F2_RK10);
    items << skipUnlessNode(ctx, VacuumNode::F2_RK50, !ctx.skipRK50,
                        { openValveTask(st, ctx, VacuumValve::K133, &VacuumRunState::s2Open) });
    items << settlePauseFor(ctx, VacuumNode::F2_RK50);

    if (opts.reliefAndClose)
        items << withNode(ctx, VacuumNode::F2_ReliefMid,
                          reliefTriplet(st, ctx, iter));   // s15–s17: безусловный триплет

    // s18–s19: закрытия в порядке CSV — сначала S2 (10133), затем S1 (10131).
    // CSV шлёт эти команды безусловно; здесь Storage-флаги подавляют закрытие
    // никогда не открывавшихся клапанов (сравнение по изменениям состояний —
    // неоднозначность №5 в docs/regimes/vacuum.md). При cancel/error закрытие
    // выполняется и в проходе 2.
    items << onGroupDone([st, ctx, opts](DoneWith w) {
        const bool close = opts.reliefAndClose || w != DoneWith::Success;
        if (close && st->s2Open) {
            if (ctx.setValve)
                ctx.setValve(false, VacuumValve::K133);
            st->s2Open = false;
        }
        if (close && st->s1Open) {
            if (ctx.setValve)
                ctx.setValve(false, VacuumValve::K131);
            st->s1Open = false;
        }
    });

    return Group(items);
}

// ── Ф3 «Второй тракт, проход 1» (s20–s24) ─────────────────────────────────────
// Страж s20 (21009, легаси cond_phase_9 = flagIncludeSecTract,
// Conditions.cpp:334), false → goto_5: пропуск всей фазы.
// s21: 11192 (SL2) → s22: 11179 (SL1) time_5000 → s23: 10192 → s24: 10179.
ExecutableItem secondTractGroup(const Storage<VacuumRunState>& st,
                                const VacuumTreeContext& ctx,
                                const RepeatIterator& iter)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            if (!ctx.secondTract) {
                if (ctx.onNode) ctx.onNode(VacuumNode::F3SecondTract, NodeState::Skipped);
                return SetupResult::StopWithSuccess;       // s20: false → goto_5
            }
            if (ctx.onNode) ctx.onNode(VacuumNode::F3SecondTract, NodeState::Running);
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K192))
                return SetupResult::StopWithError;         // s21: 11192
            st->sl2Open = true;
            if (!ctx.setValve(true, VacuumValve::K179))
                return SetupResult::StopWithError;         // s22: 11179
            st->sl1Open = true;
            return SetupResult::Continue;
        }),
        pausableDelay(st, ctx, ctx.reliefDwellSec, iter),  // s22: выдержка time_5000
        onGroupDone([st, ctx](DoneWith w) {                // s23–s24: SL2, затем SL1
            if (ctx.secondTract && ctx.onNode)
                ctx.onNode(VacuumNode::F3SecondTract, doneToNode(w));
            if (st->sl2Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K192);
                st->sl2Open = false;
            }
            if (st->sl1Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K179);
                st->sl1Open = false;
            }
        })
    };
}

// ═════════════════════════════════════════════════════════════════════════════
// Форвакуумная откачка 11.5–11.7 (A1, B/C, E/F) — перенос ТЗ v5, раздел 12.1
// ═════════════════════════════════════════════════════════════════════════════
//
// ОБЛАСТЬ: только форвакуум через К176 (ForeVacPumpTo10Pa, REQ-074/075). Турбо
// (К179, раздел 12.2), over-range/watchdog (REQ-076/079/081) и foundation-модули
// (БД C, interlock-матрица, OperatorBus) — сознательно отложены (план §4).
// Интерлок К176⇄К179 не нарушается: К179 здесь не открывается вовсе.

// ── T-03: pausableHoldUntil (REQ-075) ─────────────────────────────────────────
// Пред. pred должен держаться НЕПРЕРЫВНО holdSec тиков (аккумулятор; срыв → 0).
// Успех = удержание достигнуто. Таймаут по общему elapsed ⇒ группа возвращает
// Error (PausableTicker сам умеет только done(true), поэтому исход различаем
// разделяемым флагом succeeded в onGroupDone). pred читает ТОЛЬКО через швы.
ExecutableItem pausableHoldUntil(const VacuumTreeContext& ctx,
                                 std::function<bool()> pred,
                                 int holdSec, int timeoutSec)
{
    auto held      = std::make_shared<int>(0);
    auto succeeded = std::make_shared<bool>(false);
    return Group {
        sequential,
        TickerTask([ctx, pred, holdSec, timeoutSec, held, succeeded](PausableTicker& t) {
            *held = 0;
            *succeeded = false;
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            t.isComplete = [pred, holdSec, timeoutSec, held, succeeded](int elapsed) -> bool {
                if (pred && pred())
                    ++(*held);
                else
                    *held = 0;
                if (*held >= holdSec) {
                    *succeeded = true;
                    return true;                                  // удержание достигнуто
                }
                if (timeoutSec > 0 && elapsed >= timeoutSec)
                    return true;                                  // таймаут (succeeded=false)
                return false;
            };
        }),
        onGroupDone([succeeded](DoneWith w) -> DoneResult {
            // Success только при реальном удержании; таймаут/отмена → Error,
            // чтобы объемлющий этап закрыл клапаны в своём onGroupDone.
            return (w == DoneWith::Success && *succeeded)
                       ? DoneResult::Success : DoneResult::Error;
        })
    };
}

// Импульс клапана: open → выдержка ms → close. Закрытие гарантировано при любом
// исходе (флаг в Storage). Используется для К178 (2 с, REQ-046/049/052).
ExecutableItem valvePulse(const Storage<VacuumRunState>& st,
                          const VacuumTreeContext& ctx,
                          const QString& valve,
                          bool VacuumRunState::*openFlag,
                          int ms)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx, valve, openFlag]() -> SetupResult {
            if (!ctx.setValve || !ctx.setValve(true, valve))
                return SetupResult::StopWithError;
            (*st).*openFlag = true;
            return SetupResult::Continue;
        }),
        TickerTask([ctx, ms](PausableTicker& t) {
            t.intervalMs = qMax(1, ms);
            t.pauseBus   = ctx.pauseBus;
            t.isComplete = [](int elapsed) { return elapsed >= 1; };
        }),
        onGroupDone([st, ctx, valve, openFlag](DoneWith) {
            if ((*st).*openFlag) {
                if (ctx.setValve)
                    ctx.setValve(false, valve);
                (*st).*openFlag = false;
            }
        })
    };
}

// ── R-02: ForeVacPumpTo10Pa (REQ-047/050/053, 074/075) ────────────────────────
// Открыть К176 → удерживать ДВ301 ≤ targetVacPa не менее turboSwitchHoldSec.
// К179 НЕ открывается. К176 закрывается при любом исходе.
ExecutableItem foreVacPumpTo10Pa(const Storage<VacuumRunState>& st,
                                 const VacuumTreeContext& ctx)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            // К178 (AR5) уже закрыт импульсом стадии; открываем только К176 (AR6).
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K176))
                return SetupResult::StopWithError;
            st->k176Open = true;
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Форвакуум К176 → ДВ301 ≤ %1 Па")
                                .arg(ctx.targetVacPa));
            return SetupResult::Continue;
        }),
        // dP/dt-watchdog (REQ-020/076): убедиться, что после открытия К176
        // откачка реально идёт; иначе — операторский диалог Стоп/Продолжить.
        ctx.pumpRateCheck
            ? GroupItem(PumpRateTask([ctx](PumpRateWatchdog& w) {
                  w.pressurePa  = ctx.pressureVacPa;
                  w.intervalMs  = ctx.tickIntervalMs;
                  w.windowSec   = ctx.pumpCheckWindowSec;
                  w.minDropPa   = ctx.pumpMinDropPa;
                  w.pauseBus    = ctx.pauseBus;
                  w.operatorBus = ctx.operatorBus;
              }))
            : nullItem,
        pausableHoldUntil(ctx,
            [ctx]() -> bool {
                if (!ctx.pressureVacPa)
                    return false;                    // нет датчика → удержание не набирается
                return ctx.pressureVacPa() <= ctx.targetVacPa;
            },
            ctx.turboSwitchHoldSec, ctx.foreVacTimeoutSec),
        onGroupDone([st, ctx](DoneWith) {
            if (st->k176Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K176);
                st->k176Open = false;
            }
        })
    };
}

// ── 11.5: откачка A1 (REQ-045–048) ────────────────────────────────────────────
// Сброс К118 (без К178) → импульс К178 → форвакуум К176. Перед B/C К176/К178 закрыты.
ExecutableItem buildForevacA1(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              const RepeatIterator& iter)
{
    return Group {
        sequential,
        QSyncTask([ctx] { if (ctx.onLabel) ctx.onLabel(QStringLiteral("11.5: откачка A1")); }),
        reliefTriplet(st, ctx, iter),                              // REQ-045
        settlePause(ctx),
        valvePulse(st, ctx, VacuumValve::K178, &VacuumRunState::k178Open, ctx.k178PulseMs), // REQ-046
        settlePause(ctx),
        foreVacPumpTo10Pa(st, ctx)                                 // REQ-047 (закрытие К176 внутри)
    };
}

// ── 11.6: откачка B/C (REQ-049–051) ───────────────────────────────────────────
// Открыть применимые объёмы C (по skipRK*) → импульс К178 → форвакуум К176 →
// закрыть К176 и клапаны C. Открытие C — через onGroupSetup со Storage-флагами,
// закрытие C — в onGroupDone (гарантия при любом исходе).
ExecutableItem buildForevacBC(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            if (ctx.onLabel) ctx.onLabel(QStringLiteral("11.6: откачка B/C"));
            if (!ctx.skipRK300) {                                  // C1 = К135
                if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K135))
                    return SetupResult::StopWithError;
                st->s3Open = true;
            }
            if (!ctx.skipRK50) {                                   // C2 = К133
                if (!ctx.setValve(true, VacuumValve::K133))
                    return SetupResult::StopWithError;
                st->s2Open = true;
            }
            if (!ctx.skipRK10) {                                   // C3 = К131
                if (!ctx.setValve(true, VacuumValve::K131))
                    return SetupResult::StopWithError;
                st->s1Open = true;
            }
            return SetupResult::Continue;
        }),
        settlePause(ctx),
        valvePulse(st, ctx, VacuumValve::K178, &VacuumRunState::k178Open, ctx.k178PulseMs), // REQ-049
        settlePause(ctx),
        foreVacPumpTo10Pa(st, ctx),                                // REQ-050 (C открыты во время откачки)
        onGroupDone([st, ctx](DoneWith) {                          // REQ-051: закрыть C
            if (st->s3Open) { if (ctx.setValve) ctx.setValve(false, VacuumValve::K135); st->s3Open = false; }
            if (st->s2Open) { if (ctx.setValve) ctx.setValve(false, VacuumValve::K133); st->s2Open = false; }
            if (st->s1Open) { if (ctx.setValve) ctx.setValve(false, VacuumValve::K131); st->s1Open = false; }
        })
    };
}

// ── 11.7: откачка E/F (REQ-052–054) ───────────────────────────────────────────
// Открыть К151 → импульс К178 → форвакуум К176 → закрыть К151/К178/К176.
// (В ТЗ REQ-052 опечатки: ДД331→ДД311, «6.3»→11.3, «К178» вместо К176 —
//  план §7.2 п.A-C; здесь реализована исправленная логика без доп. сброса.)
ExecutableItem buildForevacEF(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx]() -> SetupResult {
            if (ctx.onLabel) ctx.onLabel(QStringLiteral("11.7: откачка E/F"));
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K151))  // REQ-052
                return SetupResult::StopWithError;
            st->k151Open = true;
            return SetupResult::Continue;
        }),
        settlePause(ctx),
        valvePulse(st, ctx, VacuumValve::K178, &VacuumRunState::k178Open, ctx.k178PulseMs),
        settlePause(ctx),
        foreVacPumpTo10Pa(st, ctx),                                // REQ-053
        onGroupDone([st, ctx](DoneWith) {                          // REQ-054: закрыть К151
            if (st->k151Open) {
                if (ctx.setValve) ctx.setValve(false, VacuumValve::K151);
                st->k151Open = false;
            }
        })
    };
}

// ── Execution-фаза одного повтора: Ф1 → Ф2 → Ф3 → 11.5 → 11.6 → 11.7 ─────────
ExecutableItem executionPhase(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              const RepeatIterator& iter)
{
    GroupItems items { sequential };

    items << QSyncTask([ctx] {                             // s1: 90018 метка
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("Ф1: стартовый сброс (s1)"));
    });
    items << settlePauseFor(ctx, VacuumNode::F1Relief);
    items << withNode(ctx, VacuumNode::F1Relief,
                      reliefTriplet(st, ctx, iter));       // Ф1: s2–s4
    items << settlePauseFor(ctx, VacuumNode::F2BlockC);
    items << withNode(ctx, VacuumNode::F2BlockC,
                      buildBlockC(st, ctx, iter, BlockCOptions{ /*reliefAndClose=*/true })); // Ф2: s5–s19
    items << settlePauseFor(ctx, VacuumNode::F3SecondTract);
    items << secondTractGroup(st, ctx, iter);              // Ф3: s20–s24 (узел inline)

    // Форвакуумная откачка 11.5–11.7 (gated foreVacuum; skipUnlessNode эмитит
    // Skipped при выключении — как ветки блока C).
    items << settlePauseFor(ctx, VacuumNode::F5A1);
    items << skipUnlessNode(ctx, VacuumNode::F5A1, ctx.foreVacuum,
                            { buildForevacA1(st, ctx, iter) });
    items << settlePauseFor(ctx, VacuumNode::F6BC);
    items << skipUnlessNode(ctx, VacuumNode::F6BC, ctx.foreVacuum,
                            { buildForevacBC(st, ctx) });
    items << settlePauseFor(ctx, VacuumNode::F7EF);
    items << skipUnlessNode(ctx, VacuumNode::F7EF, ctx.foreVacuum,
                            { buildForevacEF(st, ctx) });

    return Group(items);
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// buildVacuumRecipe
// ═════════════════════════════════════════════════════════════════════════════

Group buildVacuumRecipe(const VacuumTreeContext& ctx,
                        const Storage<VacuumRunState>& st,
                        const Storage<VacuumRunSummary>& summary)
{
    const RepeatIterator iter(qMax(1, ctx.totalRepeats));

    return Group {
        sequential,
        summary,
        For (iter) >> Do {
            // Storage прикреплён к группе повтора (не к самому циклу!):
            // группа входится заново на каждой итерации, поэтому состояние
            // пересоздаётся на каждый повтор — инвариант 3 по построению.
            Group {
                sequential,   // ошибка внутри повтора пропускает его остаток…
                st,
                withNode(ctx, VacuumNode::Condition, conditionPhase(ctx, iter)),
                executionPhase(st, ctx, iter),
                onGroupDone([summary, ctx, iter](DoneWith w) -> DoneResult {
                    const int repeat = int(iter.iteration());
                    if (ctx.onRepeatDone)
                        ctx.onRepeatDone(w, repeat);
                    if (w == DoneWith::Cancel)
                        return DoneResult::Error;
                    if (w == DoneWith::Success)
                        ++summary->repeatsDone;
                    else
                        ++summary->repeatsError;
                    return DoneResult::Success;  // …но не прерывает следующие повторы
                })
            }
        },
        onGroupDone([summary, ctx](DoneWith w) -> DoneResult {
            const bool ok = (w == DoneWith::Success) && summary->repeatsError == 0;
            if (ctx.onRunFinished)
                ctx.onRunFinished(w, summary->repeatsDone, summary->repeatsError);
            return ok ? DoneResult::Success : DoneResult::Error;
        })
    };
}

Group buildVacuumRecipe(const VacuumTreeContext& ctx)
{
    return buildVacuumRecipe(ctx, Storage<VacuumRunState>(), Storage<VacuumRunSummary>());
}
