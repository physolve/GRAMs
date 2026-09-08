#include "VacuumTaskTree.h"

#include <QDebug>
#include <limits>
#include <memory>

using namespace QtTaskTree;

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

// Давление уже на цели этапа или ниже — откачивать нечего. dP/dt в этой точке
// не характеризует насос: он может стоять на минимуме потока, а показание —
// колебаться около нуля. Проверять здесь нечего, и любой вердикт «мала» был бы
// ложным.
bool PumpRateWatchdog::atTarget() const
{
    if (!pressurePa)
        return false;
    const Reading r = pressurePa();
    // Только достоверное показание: мёртвый датчик, отдающий 0, иначе выглядел
    // бы как достигнутая цель и молча снимал проверку откачки.
    return r.isValid() && r.value <= targetPa;
}

void PumpRateWatchdog::beginWindow()
{
    // Проверка dP/dt только выше цели (см. targetPa в заголовке).
    if (atTarget()) {
        m_timer.stop();
        // Отложенный emit: beginWindow вызывается в том числе из start(), а
        // done прямо из start() пришёл бы раньше, чем вызывающая сторона
        // успела войти в своё ожидание.
        QTimer::singleShot(0, this, [this] { emit done(true); });
        return;
    }
    m_elapsed = 0;
    // Первое окно открывается только после «мёртвой зоны»: пока насос выходит
    // на режим, ДВ301 стоит в over-range и dP/dt ложно мала (ложный отказ).
    m_delayLeft = m_firstWindow ? qMax(0, startDelaySec) : 0;
    m_firstWindow = false;
    // p0 берётся здесь только если задержки нет; иначе — по её истечении,
    // чтобы падение за время выхода на режим не засчитывалось в окно.
    m_p0 = (m_delayLeft == 0 && pressurePa) ? pressurePa().value : 0.0;
    m_awaitingOperator = false;
    if (pauseBus && pauseBus->isPaused())
        return;                         // стартуем замороженными, ждём resume
    m_timer.start(intervalMs);
}

void PumpRateWatchdog::tick()
{
    // Цель могла быть достигнута прямо в «мёртвой зоне» или посреди окна —
    // тогда окно досчитывать незачем: откачка очевидно шла.
    if (atTarget()) {
        m_timer.stop();
        emit done(true);
        return;
    }
    if (m_delayLeft > 0) {
        if (--m_delayLeft == 0)
            m_p0 = pressurePa ? pressurePa().value : 0.0;  // отсчёт окна с этой точки
        return;
    }
    ++m_elapsed;
    if (m_elapsed < windowSec)
        return;
    m_timer.stop();
    const Reading r = pressurePa ? pressurePa() : Reading(0.0, Quality::NoResponse);
    // Over range — нормальная фаза выхода насоса на режим, а не «откачка не
    // идёт»: судить о dP/dt по зашкалившему датчику нельзя. Открываем новое
    // окно, не поднимая оператора.
    if (r.isOverRange()) {
        beginWindow();
        return;
    }
    const double drop = m_p0 - r.value;   // падение давления за окно
    if (r.isValid() && drop >= minDropPa) { // давление падает → откачка идёт
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
using PromptTask    = QCustomTask<OperatorPrompt>;

namespace {

// Короткая подпись узла для человекочитаемых причин отказа (UI/лог).
QString nodeTitle(VacuumNode node)
{
    switch (node) {
    case VacuumNode::Condition:     return QStringLiteral("Условие");
    case VacuumNode::F1Relief:      return QStringLiteral("Ф1 сброс");
    case VacuumNode::F2BlockC:      return QStringLiteral("Ф2 блок C");
    case VacuumNode::F2_RK300:      return QStringLiteral("Ф2 C1/RK300");
    case VacuumNode::F2_RK10:       return QStringLiteral("Ф2 C3/RK10");
    case VacuumNode::F2_RK50:       return QStringLiteral("Ф2 C2/RK50");
    case VacuumNode::F2_ReliefMid:  return QStringLiteral("Ф2 средний сброс");
    case VacuumNode::F3SecondTract: return QStringLiteral("Ф3 второй тракт");
    case VacuumNode::F5A1:          return QStringLiteral("11.5 форвакуум A1");
    case VacuumNode::F6BC:          return QStringLiteral("11.6 форвакуум B/C");
    case VacuumNode::F7EF:          return QStringLiteral("11.7 форвакуум E/F");
    case VacuumNode::TurboGate:     return QStringLiteral("12.2 гейт перехода на турбо");
    case VacuumNode::TurboSwitch:   return QStringLiteral("12.2 переключение К176→К179");
    case VacuumNode::TurboPumping:  return QStringLiteral("12.2 турбо-откачка");
    case VacuumNode::TurboFallback: return QStringLiteral("12.2 откат на форвакуум");
    case VacuumNode::GeneralPumping: return QStringLiteral("11.7б общая откачка");
    case VacuumNode::LeakTest:      return QStringLiteral("11.8 герметичность");
    case VacuumNode::FinalPrep:     return QStringLiteral("11.9 подготовка финальной откачки");
    case VacuumNode::FinalPumping:  return QStringLiteral("11.10 финальная откачка камеры");
    case VacuumNode::FinalClose:    return QStringLiteral("11.11 финальное закрытие");
    case VacuumNode::ContinuousPumping: return QStringLiteral("Непрерывная откачка");
    }
    return QStringLiteral("этап");
}

// Значение показания для UI: NaN, если датчика нет ИЛИ показание недостоверно.
// Монитор отличает NaN через std::isnan и гасит строку «есть показание».
double readingOrNan(const std::function<Reading()>& seam)
{
    if (!seam)
        return std::numeric_limits<double>::quiet_NaN();
    const Reading r = seam();
    return r.isValid() ? r.value : std::numeric_limits<double>::quiet_NaN();
}

// ── Наблюдение за узлами развёртки (аддитивно, control-flow не меняет) ─────────
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

// Пауза после действия (единая для всех шагов рецепта, ctx.perActionPauseMs).
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
        settlePause(ctx),
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
    items << settlePause(ctx);
    items << skipUnlessNode(ctx, VacuumNode::F2_RK50, !ctx.skipRK50,
                        { openValveTask(st, ctx, VacuumValve::K133, &VacuumRunState::s2Open) });
    items << settlePause(ctx);

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
// s21: 11192 (SL1) → s22: 11179 (SL2) time_5000 → s23: 10192 → s24: 10179.
// Открытие К179 определено ниже, вместе с процедурой 12.2: Ф3 обязана
// проходить через ту же единственную точку, что и турбо-переход.
ExecutableItem openTurboValve(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              bool requireConfirm);

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
            st->k192Open = true;
            // s22: 11179 — К179 это КЛАПАН ТУРБОНАСОСА. Открывается только
            // через единую точку openTurboValve, которая гарантирует закрытый
            // К176 (REQ-008). Раньше здесь был прямой setValve в обход проверки.
            return SetupResult::Continue;
        }),
        openTurboValve(st, ctx, /*requireConfirm=*/false),
        QSyncTask([st]() -> bool {
            st->k179TractOpen = st->k179Open;   // закрытием ниже управляет эта группа
            return true;
        }),
        pausableDelay(st, ctx, ctx.reliefDwellSec, iter),  // s22: выдержка time_5000
        onGroupDone([st, ctx](DoneWith w) {                // s23–s24: К192, затем К179
            if (ctx.secondTract && ctx.onNode)
                ctx.onNode(VacuumNode::F3SecondTract, doneToNode(w));
            if (st->k192Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K192);
                st->k192Open = false;
            }
            if (st->k179TractOpen) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K179);
                st->k179TractOpen = false;
                st->k179Open = false;
            }
        })
    };
}

// ═════════════════════════════════════════════════════════════════════════════
// Форвакуумная откачка 11.5–11.7 (A1, B/C, E/F) — перенос ТЗ v5, раздел 12.1
// ═════════════════════════════════════════════════════════════════════════════
//
// ОБЛАСТЬ: только форвакуум через К176 (ForeVacPumpToTarget, REQ-074/075). Турбо
// (К179, раздел 12.2), over-range/watchdog (REQ-076/079/081) и foundation-модули
// (БД C, interlock-матрица, OperatorBus) — сознательно отложены (план §4).
// Интерлок К176⇄К179 не нарушается: К179 здесь не открывается вовсе.

// ── T-03: pausableHoldUntil (REQ-075) ─────────────────────────────────────────
// Пред. pred должен держаться НЕПРЕРЫВНО holdSec тиков (аккумулятор; срыв → 0).
// Успех = удержание достигнуто. Таймаут по общему elapsed ⇒ группа возвращает
// Error (PausableTicker сам умеет только done(true), поэтому исход различаем
// разделяемым флагом succeeded в onGroupDone). pred читает ТОЛЬКО через швы.
// onTick (nullable) вызывается КАЖДЫЙ тик с (held, elapsed) — для live-прогресса
// в UI. onTimeout (nullable) — только при реальном исчерпании timeoutSec (не при
// отмене), чтобы объемлющий этап сообщил человекочитаемую причину.
ExecutableItem pausableHoldUntil(const VacuumTreeContext& ctx,
                                 std::function<bool()> pred,
                                 int holdSec, int timeoutSec,
                                 std::function<void(int heldSec, int elapsedSec)> onTick = {},
                                 std::function<void()> onTimeout = {})
{
    auto held      = std::make_shared<int>(0);
    auto succeeded = std::make_shared<bool>(false);
    return Group {
        sequential,
        TickerTask([ctx, pred, holdSec, timeoutSec, held, succeeded, onTick](PausableTicker& t) {
            *held = 0;
            *succeeded = false;
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            t.isComplete = [pred, holdSec, timeoutSec, held, succeeded, onTick](int elapsed) -> bool {
                if (pred && pred())
                    ++(*held);
                else
                    *held = 0;
                if (onTick)
                    onTick(*held, elapsed);
                if (*held >= holdSec) {
                    *succeeded = true;
                    return true;                                  // удержание достигнуто
                }
                if (timeoutSec > 0 && elapsed >= timeoutSec)
                    return true;                                  // таймаут (succeeded=false)
                return false;
            };
        }),
        onGroupDone([succeeded, onTimeout](DoneWith w) -> DoneResult {
            // Success только при реальном удержании; таймаут/отмена → Error,
            // чтобы объемлющий этап закрыл клапаны в своём onGroupDone.
            if (w == DoneWith::Success && !*succeeded && onTimeout)
                onTimeout();
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

// ── R-02: ForeVacPumpToTarget (REQ-047/050/053, 074/075) ────────────────────────
// Открыть К176 → удерживать ДВ301 ≤ targetVacPa не менее foreVacHoldSec.
// К179 НЕ открывается. К176 закрывается при любом исходе.
// node — узел развёртки, которому принадлежит этот этап (F5A1/F6BC/F7EF):
// нужен только для адресации live-прогресса в UI.
ExecutableItem foreVacPumpToTarget(const Storage<VacuumRunState>& st,
                                 const VacuumTreeContext& ctx,
                                 VacuumNode node)
{
    return Group {
        sequential,
        onGroupSetup([st, ctx, node]() -> SetupResult {
            // К178 (AR5) уже закрыт импульсом стадии; открываем только К176 (AR6).
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K176)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("%1: не удалось открыть К176 (форвакуумный насос)")
                                      .arg(nodeTitle(node)));
                return SetupResult::StopWithError;
            }
            st->k176Open = true;
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Форвакуум К176 → ДВ301 ≤ %1 Па")
                                .arg(ctx.targetVacPa));
            // Стартовая отсечка прогресса: цель видна в UI сразу, до первого тика.
            if (ctx.onForevacProgress)
                ctx.onForevacProgress(node, readingOrNan(ctx.pressureVacPa), 0, 0);
            return SetupResult::Continue;
        }),
        // dP/dt-watchdog (REQ-020/076): убедиться, что после открытия К176
        // откачка реально идёт; иначе — операторский диалог Стоп/Продолжить.
        ctx.pumpRateCheck
            ? GroupItem(PumpRateTask([ctx](PumpRateWatchdog& w) {
                  w.pressurePa  = ctx.pressureVacPa;
                  w.intervalMs   = ctx.tickIntervalMs;
                  w.startDelaySec = ctx.pumpCheckDelaySec;
                  w.windowSec    = ctx.pumpCheckWindowSec;
                  w.minDropPa   = ctx.pumpMinDropPa;
                  w.targetPa    = ctx.targetVacPa;
                  w.pauseBus    = ctx.pauseBus;
                  w.operatorBus = ctx.operatorBus;
              }))
            : nullItem,
        pausableHoldUntil(ctx,
            [ctx]() -> bool {
                if (!ctx.pressureVacPa)
                    return false;                    // нет датчика → удержание не набирается
                const Reading r = ctx.pressureVacPa();
                // Недостоверное показание НЕ засчитывается: мёртвый датчик,
                // отдающий 0, иначе выглядел бы как достигнутая цель.
                if (!r.isValid())
                    return false;
                return r.value <= ctx.targetVacPa;
            },
            ctx.foreVacHoldSec, ctx.foreVacTimeoutSec,
            [ctx, node](int held, int elapsed) {     // live-прогресс в UI
                if (ctx.onForevacProgress)
                    ctx.onForevacProgress(node, readingOrNan(ctx.pressureVacPa),
                                          held, elapsed);
            },
            [ctx, node] {                            // причина отказа этапа
                if (!ctx.onFailure)
                    return;
                if (!ctx.pressureVacPa) {
                    ctx.onFailure(QStringLiteral("%1: датчик ДВ301 не задан — удержание "
                                                 "≤ %2 Па не набрано за %3 с")
                                      .arg(nodeTitle(node)).arg(ctx.targetVacPa)
                                      .arg(ctx.foreVacTimeoutSec));
                    return;
                }
                const Reading r = ctx.pressureVacPa();
                ctx.onFailure(QStringLiteral("%1: таймаут форвакуума — ДВ301 %2 Па (%3) не "
                                             "удержалось ≤ %4 Па в течение %5 с (лимит %6 с)")
                                  .arg(nodeTitle(node))
                                  .arg(r.value, 0, 'g', 3)
                                  .arg(qualityName(r.quality))
                                  .arg(ctx.targetVacPa)
                                  .arg(ctx.foreVacHoldSec)
                                  .arg(ctx.foreVacTimeoutSec));
            }),
        onGroupDone([st, ctx, node](DoneWith w) {
            if (st->k176Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K176);
                st->k176Open = false;
            }
            if (ctx.onForevacDone)
                ctx.onForevacDone(node, w == DoneWith::Success);
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
        foreVacPumpToTarget(st, ctx, VacuumNode::F5A1)               // REQ-047 (закрытие К176 внутри)
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
        foreVacPumpToTarget(st, ctx, VacuumNode::F6BC),              // REQ-050 (C открыты во время откачки)
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
        foreVacPumpToTarget(st, ctx, VacuumNode::F7EF),              // REQ-053
        onGroupDone([st, ctx](DoneWith) {                          // REQ-054: закрыть К151
            if (st->k151Open) {
                if (ctx.setValve) ctx.setValve(false, VacuumValve::K151);
                st->k151Open = false;
            }
        })
    };
}

// ── Execution-фаза одного повтора: Ф1 → Ф2 → Ф3 → 11.5 → 11.6 → 11.7 ─────────
// ═════════════════════════════════════════════════════════════════════════════
// Раздел 12.2 — переход на турбомолекулярный насос (REQ-077…084)
// ═════════════════════════════════════════════════════════════════════════════
//
// Топология (подтверждена на стенде):
//   магистраль ──┬── AR6 / К176 ── форвакуумный насос
//                └── SL2 / К179 ── турбомолекулярный насос ── ДВ302 ──
//                                   SL1 / К192 ── выход второго тракта
//
// К176 и К179 НИКОГДА не открыты одновременно (REQ-008/084): турбонасос,
// запущенный на форвакуумный тракт, выходит из строя. Это не «сбой режима»,
// а порча оборудования, поэтому проверка дублируется на трёх уровнях —
// матрица interlock (модуль M-02, отдельная работа), явное подтверждение
// readback здесь, и Security::checkValveAction как последний рубеж.

// ── Обработка over range (REQ-079 для ДВ301, REQ-081 для ДВ302) ───────────────
//
// Выход за диапазон — это не отказ датчика, а нормальная фаза процесса: в
// начале откачки давление заведомо выше верхней границы измерения. ТЗ задаёт
// двухступенчатое ожидание: выдержка → перепроверка → предупреждение и
// БЕЗОПАСНОЕ ДЕЙСТВИЕ → повторная выдержка → окончательный отказ.
//
// Безопасное действие у датчиков РАЗНОЕ и не взаимозаменяемо:
//   ДВ301 → закрыть К176;  ДВ302 → не открывать (или закрыть) К179.
//
// awaitOperator = true — окончательный отказ передаётся оператору (ДВ302,
// REQ-081); false — критическая ошибка с остановом режима (ДВ301, REQ-079).
ExecutableItem overRangeGuard(const VacuumTreeContext& ctx,
                              std::function<Reading()> read,
                              int waitSec,
                              int operatorCode,
                              const QString& sensorName,
                              std::function<void()> safeAction,
                              bool awaitOperator)
{
    auto phase  = std::make_shared<int>(0);      // 0 = первая выдержка, 1 = вторая
    auto failed = std::make_shared<bool>(false);

    return Group {
        sequential,
        onGroupSetup([ctx, read, sensorName]() -> SetupResult {
            // Датчик в норме — этап не нужен вовсе, дерево не задерживаем.
            if (!read || !read().isOverRange())
                return SetupResult::StopWithSuccess;
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("%1 вне диапазона — ожидание").arg(sensorName));
            return SetupResult::Continue;
        }),
        TickerTask([ctx, read, waitSec, sensorName, safeAction, phase, failed](PausableTicker& t) {
            *phase = 0;
            *failed = false;
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            t.isComplete = [ctx, read, waitSec, sensorName, safeAction, phase, failed](int elapsed) -> bool {
                if (!read)
                    return true;
                if (!read().isOverRange())
                    return true;                       // показание вернулось в диапазон
                if (elapsed < waitSec * (*phase + 1))
                    return false;                      // выдержка ещё идёт
                if (*phase == 0) {
                    // Предупреждение + безопасное действие, затем вторая выдержка.
                    if (safeAction)
                        safeAction();
                    if (ctx.onLabel)
                        ctx.onLabel(QStringLiteral("%1 всё ещё вне диапазона — "
                                                   "проверьте насос; повторное ожидание")
                                        .arg(sensorName));
                    *phase = 1;
                    return false;
                }
                *failed = true;                        // повторный отказ
                return true;
            };
        }),
        // Окончательный отказ: либо решение оператора (ДВ302), либо критическая
        // ошибка (ДВ301). Вложенная группа выполняется только при *failed.
        Group {
            sequential,
            onGroupSetup([failed]() -> SetupResult {
                return *failed ? SetupResult::Continue : SetupResult::StopWithSuccess;
            }),
            awaitOperator
                ? GroupItem(PromptTask([ctx, operatorCode, sensorName](OperatorPrompt& p) {
                      p.code = operatorCode;
                      p.message = QStringLiteral(
                          "%1 остаётся вне диапазона измерения.\n"
                          "Турбомолекулярный насос не будет подключён.\n"
                          "«Продолжить» — повторить ожидание; «Остановить» — стоп режима.")
                              .arg(sensorName);
                      p.operatorBus = ctx.operatorBus;
                  }))
                : GroupItem(QSyncTask([ctx, sensorName]() -> bool {
                      if (ctx.onFailure)
                          ctx.onFailure(QStringLiteral(
                              "%1 дважды вне диапазона измерения — критическая ошибка "
                              "(REQ-079)").arg(sensorName));
                      return false;      // критическая ошибка → останов режима
                  }))
        }
    };
}

// ── Открытие К179 — ЕДИНСТВЕННАЯ точка в рецепте (REQ-008/082/084) ───────────
//
// Раньше К179 открывался в двух местах: здесь и напрямую в Ф3 «второй тракт».
// Это ровно та ошибка, которую ТЗ запрещает: клапан турбонасоса нельзя
// открывать в обход проверки состояния К176.
//
// requireConfirm = true (турбо-переход): подтверждение readback обязательно;
// без шва confirmValve REQ-082 непроверяем и переход не выполняется вовсе.
// requireConfirm = false (Ф3): К176 в этой ветке не открывался ни разу, и до
// появления readback на стенде Ф3 обязана работать как прежде. Если шов задан,
// проверка выполняется и здесь — строго безопаснее прежнего поведения.
//
// k176Open — то, что рецепт знает о состоянии К176: nullptr означает «состояние
// неизвестно» (хвост непрерывной откачки живёт вне Storage повтора), и тогда
// закрытие выполняется безусловно — оно идемпотентно.
bool connectTurboPump(const VacuumTreeContext& ctx, bool requireConfirm, bool* k176Open)
{
    // 1. К176 обязан быть закрыт (REQ-008).
    if (!k176Open || *k176Open) {
        if (!ctx.setValve || !ctx.setValve(false, VacuumValve::K176)) {
            if (ctx.onFailure)
                ctx.onFailure(QStringLiteral(
                    "Переход на турбо: не удалось закрыть К176 — К179 не открывается"));
            return false;
        }
        if (k176Open)
            *k176Open = false;
    }

    // 2. Подтверждение ФАКТА закрытия, а не приёма команды (REQ-082).
    if (ctx.confirmValve) {
        if (!ctx.confirmValve(false, VacuumValve::K176)) {
            if (ctx.onFailure)
                ctx.onFailure(QStringLiteral(
                    "Переход на турбо: закрытие К176 не подтверждено чтением "
                    "состояния — К179 НЕ открыт (REQ-082)"));
            return false;
        }
    } else if (requireConfirm) {
        if (ctx.onFailure)
            ctx.onFailure(QStringLiteral(
                "Переход на турбо: нет шва подтверждения состояния клапана — "
                "REQ-082 непроверяем, К179 НЕ открыт"));
        return false;
    }

    // 3. Только теперь турбоклапан.
    if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K179)) {
        if (ctx.onFailure)
            ctx.onFailure(QStringLiteral("Переход на турбо: К179 не открылся"));
        return false;
    }
    return true;
}

ExecutableItem openTurboValve(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              bool requireConfirm)
{
    return QSyncTask([st, ctx, requireConfirm]() -> bool {
        bool k176Open = st->k176Open;
        const bool ok = connectTurboPump(ctx, requireConfirm, &k176Open);
        st->k176Open = k176Open;
        if (!ok)
            return false;
        st->k179Open = true;
        if (requireConfirm && ctx.onTurboSwitched)
            ctx.onTurboSwitched(true);
        return true;
    });
}

// ── Откат на форвакуум (REQ-083) ─────────────────────────────────────────────
// Закрыть К179, открыть К176. Порядок обратный переходу и так же неизменен.
ExecutableItem turboFallback(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx)
{
    return withNode(ctx, VacuumNode::TurboFallback, QSyncTask([st, ctx]() -> bool {
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("Откат на форвакуум: ДВ302 выше порога возврата"));
        if (st->k179Open) {
            if (ctx.setValve)
                ctx.setValve(false, VacuumValve::K179);
            st->k179Open = false;
        }
        if (ctx.setValve && ctx.setValve(true, VacuumValve::K176))
            st->k176Open = true;
        if (ctx.onTurboSwitched)
            ctx.onTurboSwitched(false);
        return true;
    }));
}

// ── Процедура 12.2 целиком ───────────────────────────────────────────────────
//
// Шаг 1 (REQ-078): К176 открыт, контроль ДВ301, обработка over range.
// Шаг 2 (REQ-080): У1 ДВ301 ≤ turboSwitchPressurePa, У2 удержание
//                  turboSwitchHoldSec непрерывно, У3 ДВ302 валиден.
// Шаг 3 (REQ-082): закрыть К176 → подтвердить readback → открыть К179.
// Шаг 4 (REQ-083): контроль ДВ302; ≥ turboReturnPressurePa → откат.
//
// НЕ РЕАЛИЗОВАНО СОЗНАТЕЛЬНО: автоматический повтор попытки после отката
// («возврат к шагу 1» из ТЗ). После отката система остаётся на форвакууме в
// безопасном состоянии, причина записана. Повтор требует счётчика попыток и
// критерия «когда прекратить», которых в ТЗ нет; до их появления цикл
// переключений К176/К179 опаснее одного отката.
//
// Критерий завершения турбо-откачки здесь — отведённое время: целевое давление
// targetVAC и EvacTime относятся к финальной откачке 11.9–11.11, которая в
// объём этой работы не входит.
// Параметры вызова процедуры. Значения по умолчанию воспроизводят прежнее
// поведение единственного вызова из цепочки Ф, поэтому существующие тесты
// 12.2 остаются в силе без правок.
struct PumpDownOptions {
    // Длительность фазы откачки после перехода на К179. <0 — ctx.turboTimeoutSec.
    // 11.7б задаёт testEvacTimeSec (REQ-057), 11.10 — evacTimeSec (REQ-068).
    int  pumpingSec = -1;
    // Закрыть оба насосных клапана по завершении. 11.9→11.10 идут на ОДНОМ
    // насосе, и закрытие между ними сорвало бы набранный вакуум.
    bool closePumps = true;
};

ExecutableItem buildPumpDownProcedure(const Storage<VacuumRunState>& st,
                                      const VacuumTreeContext& ctx,
                                      const PumpDownOptions& opts = {})
{
    // Предикат гейта: У1 и У3 сразу; У2 «непрерывно» обеспечивает
    // pausableHoldUntil, обнуляя счётчик при срыве условия.
    auto gateOk = [ctx]() -> bool {
        if (!ctx.pressureVacPa)
            return false;
        const Reading p301 = ctx.pressureVacPa();
        if (!p301.isValid() || p301.value > ctx.turboSwitchPressurePa)
            return false;                       // У1
        if (!ctx.pressureTurboPa)
            return false;                       // У3: без ДВ302 контроля нет
        return ctx.pressureTurboPa().isValid();
    };

    // Безопасные действия over range вызываются из isComplete тикера, то есть
    // из QTimer ВНЕ активного контекста дерева. Обращаться к Tasking::Storage
    // оттуда нельзя (см. предупреждение у PausableTicker) — поэтому здесь
    // только команда клапану, без правки VacuumRunState. Расхождение безопасно:
    // повторное закрытие идемпотентно, а openTurboValve всё равно закрывает
    // К176 заново перед открытием К179.
    auto closeK176 = [ctx] {
        if (ctx.setValve)
            ctx.setValve(false, VacuumValve::K176);
    };
    auto closeK179 = [ctx] {
        if (ctx.setValve)
            ctx.setValve(false, VacuumValve::K179);
    };

    auto fellBack = std::make_shared<bool>(false);
    const int pumpingSec = opts.pumpingSec < 0 ? ctx.turboTimeoutSec : opts.pumpingSec;
    const bool closePumps = opts.closePumps;

    return Group {
        sequential,

        // ── Шаги 1–2: гейт перехода ───────────────────────────────────────────
        withNode(ctx, VacuumNode::TurboGate, Group {
            sequential,
            onGroupSetup([st, ctx]() -> SetupResult {
                if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K176)) {
                    if (ctx.onFailure)
                        ctx.onFailure(QStringLiteral(
                            "12.2 гейт: не удалось открыть К176 (форвакуумный насос)"));
                    return SetupResult::StopWithError;
                }
                st->k176Open = true;
                if (ctx.onLabel)
                    ctx.onLabel(QStringLiteral("12.2 гейт: ДВ301 ≤ %1 Па удержать %2 с")
                                    .arg(ctx.turboSwitchPressurePa)
                                    .arg(ctx.turboSwitchHoldSec));
                return SetupResult::Continue;
            }),
            // Шаг 1а: over range ДВ301 → безопасное действие = закрыть К176.
            overRangeGuard(ctx, ctx.pressureVacPa, ctx.overrangeWaitSec,
                           OperatorBus::Dv301OverRange, QStringLiteral("ДВ301"),
                           closeK176, /*awaitOperator=*/false),
            // Шаг 2а: over range ДВ302 → безопасное действие = закрыть К179.
            overRangeGuard(ctx, ctx.pressureTurboPa, ctx.overrangeWaitSec2,
                           OperatorBus::Dv302OverRange, QStringLiteral("ДВ302"),
                           closeK179, /*awaitOperator=*/true),
            // Шаг 2: удержание условия.
            pausableHoldUntil(ctx, gateOk,
                ctx.turboSwitchHoldSec, ctx.turboTimeoutSec,
                [ctx](int held, int elapsed) {
                    if (ctx.onTurboProgress)
                        ctx.onTurboProgress(VacuumNode::TurboGate,
                                            ctx.pressureVacPa   ? ctx.pressureVacPa()
                                                                : Reading(0.0, Quality::NoResponse),
                                            ctx.pressureTurboPa ? ctx.pressureTurboPa()
                                                                : Reading(0.0, Quality::NoResponse),
                                            held, elapsed);
                },
                [ctx] {
                    if (!ctx.onFailure)
                        return;
                    const Reading p301 = ctx.pressureVacPa
                                             ? ctx.pressureVacPa()
                                             : Reading(0.0, Quality::NoResponse);
                    const Reading p302 = ctx.pressureTurboPa
                                             ? ctx.pressureTurboPa()
                                             : Reading(0.0, Quality::NoResponse);
                    ctx.onFailure(QStringLiteral(
                        "12.2 гейт: условие перехода не набрано за %1 с — "
                        "ДВ301 %2 Па (%3), ДВ302 %4 Па (%5), порог ≤ %6 Па")
                            .arg(ctx.turboTimeoutSec)
                            .arg(p301.value, 0, 'g', 3).arg(qualityName(p301.quality))
                            .arg(p302.value, 0, 'g', 3).arg(qualityName(p302.quality))
                            .arg(ctx.turboSwitchPressurePa));
                })
        }),

        // ── Шаг 3: переключение ───────────────────────────────────────────────
        withNode(ctx, VacuumNode::TurboSwitch,
                 openTurboValve(st, ctx, /*requireConfirm=*/true)),

        // ── Шаг 4: турбо-откачка и порог возврата ─────────────────────────────
        withNode(ctx, VacuumNode::TurboPumping, Group {
            sequential,
            onGroupSetup([ctx, fellBack]() -> SetupResult {
                *fellBack = false;
                if (ctx.onLabel)
                    ctx.onLabel(QStringLiteral("Турбо-откачка: контроль ДВ302, "
                                               "порог возврата %1 Па")
                                    .arg(ctx.turboReturnPressurePa));
                return SetupResult::Continue;
            }),
            TickerTask([ctx, fellBack, pumpingSec](PausableTicker& t) {
                t.intervalMs = ctx.tickIntervalMs;
                t.pauseBus   = ctx.pauseBus;
                t.isComplete = [ctx, fellBack, pumpingSec](int elapsed) -> bool {
                    const Reading p302 = ctx.pressureTurboPa
                                             ? ctx.pressureTurboPa()
                                             : Reading(0.0, Quality::NoResponse);
                    if (ctx.onTurboProgress)
                        ctx.onTurboProgress(VacuumNode::TurboPumping,
                                            ctx.pressureVacPa ? ctx.pressureVacPa()
                                                              : Reading(0.0, Quality::NoResponse),
                                            p302, 0, elapsed);
                    // Порог возврата — по достоверному показанию. Недостоверное
                    // не повод откатываться: этим занимается overRangeGuard.
                    if (p302.isValid() && p302.value >= ctx.turboReturnPressurePa) {
                        *fellBack = true;
                        return true;
                    }
                    return elapsed >= pumpingSec;
                };
            })
        }),

        // Откат выполняется, только если сработал порог возврата.
        Group {
            sequential,
            onGroupSetup([fellBack]() -> SetupResult {
                return *fellBack ? SetupResult::Continue : SetupResult::StopWithSuccess;
            }),
            turboFallback(st, ctx)
        },

        // Этап закрывает за собой оба насосных клапана — тот же контракт, что
        // у foreVacPumpToTarget. Откат оставляет К176 открытым сознательно
        // (система вернулась на форвакуум), но по завершении этапа тракт всё
        // равно приводится в закрытое состояние; непрерывная откачка, если она
        // включена, откроет его заново своим хвостом.
        onGroupDone([st, ctx, closePumps](DoneWith w) {
            // closePumps = false оставляет активный насос работать — но только
            // при штатном исходе. Отмена и ошибка обязаны привести тракт в
            // закрытое состояние в любом случае, иначе останов режима оставил
            // бы установку с открытым насосным клапаном.
            if (!closePumps && w == DoneWith::Success)
                return;
            if (st->k179Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K179);
                st->k179Open = false;
            }
            if (st->k176Open) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K176);
                st->k176Open = false;
            }
        })
    };
}

// ═════════════════════════════════════════════════════════════════════════════
// Этапы 11.7б / 11.8 / 11.9–11.11 — тракт до камеры (REQ-055…073)
// ═════════════════════════════════════════════════════════════════════════════
//
// До этих этапов режим физически ничего не откачивал: цепочка Ф закрывает за
// собой К151 (11.7), К178 живёт только импульсом, а К171/К173 не открывались
// вовсе — турбонасос подключался к отсечённой магистрали. Здесь собирается
// настоящий тракт, и набор клапанов приходит ИЗ КОНФИГУРАЦИИ (REQ-055),
// а не из кода: см. profile/GRAMsPfp.json → vacuumTract.

// Открыть набор клапанов тракта в порядке, заданном конфигурацией. Порядок —
// часть требования: REQ-055 требует открыть К171 и К173 ДО форвакуумного
// тракта, и переставить их местами нельзя. Первый отказ останавливает этап:
// открывать остаток на заведомо неполном тракте бессмысленно и небезопасно.
bool openTract(const Storage<VacuumRunState>& st, const VacuumTreeContext& ctx,
               const QStringList& valves)
{
    for (const QString& v : valves) {
        if (!ctx.setValve || !ctx.setValve(true, v))
            return false;
        st->tractOpen.append(v);
    }
    return true;
}

// Закрыть всё, что этап открыл, обратным обходом (REQ-070/071). Обратный
// порядок не косметика: насосный конец тракта отсекается раньше, чем
// разъединяются объёмы между собой. Групповой команды у контроллера нет —
// ValveControl пишет DO-регистр целиком на каждую запись, — поэтому каждое
// закрытие идёт отдельным вызовом шва и, значит, отдельной строкой журнала,
// чего REQ-071 и требует даже от групповой команды.
void closeTract(const Storage<VacuumRunState>& st, const VacuumTreeContext& ctx)
{
    while (!st->tractOpen.isEmpty()) {
        const QString v = st->tractOpen.takeLast();
        if (ctx.setValve)
            ctx.setValve(false, v);
    }
}

// Применимые клапаны блока C по флагам исключения (REQ-055 «применимые клапаны
// группы C»). Порядок C1→C2→C3 совпадает с 11.6, чтобы журналы двух этапов
// читались одинаково.
QStringList blockCValves(const VacuumTreeContext& ctx)
{
    QStringList out;
    if (!ctx.skipRK300) out << VacuumValve::K135;   // C1
    if (!ctx.skipRK50)  out << VacuumValve::K133;   // C2
    if (!ctx.skipRK10)  out << VacuumValve::K131;   // C3
    return out;
}

// Этап не выполняется и почему. Пропуск отмечается И в развёртке (Skipped), И
// отдельным швом с текстом: без текста оператор увидит серую строку и не
// узнает, что именно не сконфигурировано.
void skipStage(const VacuumTreeContext& ctx, VacuumNode node, const QString& reason)
{
    if (ctx.onNode)         ctx.onNode(node, NodeState::Skipped);
    if (ctx.onStageSkipped) ctx.onStageSkipped(node, reason);
}

// ── 11.7б: общая откачка с подключением турбонасоса (REQ-055–058) ─────────────
//
// Порядок из ТЗ: К171 и К173 → К151 и применимые C → К178 → К176 → гейт
// ≤ turboSwitchPressurePa с удержанием → переход на К179 → testEvacTimeSec.
// Гейт и переход не дублируются здесь намеренно: это ровно шаги 1–3
// PumpDownProcedure (REQ-056 «только после этого разрешается переход»), и
// второй экземпляр той же логики разошёлся бы с первым при первой же правке.
ExecutableItem buildGeneralPumping(const Storage<VacuumRunState>& st,
                                   const VacuumTreeContext& ctx)
{
    const bool configured = !ctx.generalPumpingValves.isEmpty();

    PumpDownOptions opts;
    opts.pumpingSec = ctx.testEvacTimeSec;   // REQ-057
    opts.closePumps = true;                  // REQ-058

    return Group {
        sequential,
        onGroupSetup([st, ctx, configured]() -> SetupResult {
            if (!configured) {
                skipStage(ctx, VacuumNode::GeneralPumping,
                          QStringLiteral("11.7б: набор клапанов общей откачки не задан "
                                         "в профиле (vacuumTract.generalPumping) — "
                                         "этап не выполнялся"));
                return SetupResult::StopWithSuccess;
            }
            if (ctx.onNode)  ctx.onNode(VacuumNode::GeneralPumping, NodeState::Running);
            if (ctx.onLabel) ctx.onLabel(QStringLiteral("11.7б: общая откачка"));

            // REQ-055: сначала тракт (К171, К173, К151), потом применимые C.
            if (!openTract(st, ctx, ctx.generalPumpingValves)
                || !openTract(st, ctx, blockCValves(ctx))) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("11.7б: тракт общей откачки не собран — "
                                                 "клапан не открылся"));
                return SetupResult::StopWithError;
            }
            // REQ-056: форвакуумный тракт открывается ПОСЛЕ тракта, не раньше.
            // К176 откроет сама PumpDownProcedure — здесь только магистраль.
            if (!ctx.setValve(true, VacuumValve::K178)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("11.7б: К178 (магистраль) не открылся"));
                return SetupResult::StopWithError;
            }
            st->tractOpen.append(VacuumValve::K178);
            st->k178Open = true;
            return SetupResult::Continue;
        }),
        settlePause(ctx),
        buildPumpDownProcedure(st, ctx, opts),
        onGroupDone([st, ctx, configured](DoneWith w) {   // REQ-058
            closeTract(st, ctx);
            st->k178Open = false;
            if (configured && ctx.onNode)
                ctx.onNode(VacuumNode::GeneralPumping, doneToNode(w));
        })
    };
}

// ── 11.8: проверка герметичности (REQ-059–063) ────────────────────────────────
//
// Закрыть всё (это уже сделал предыдущий этап), открыть только контрольный
// участок, снять начальные показания, выдержать leakTestDurationSec, сравнить
// прирост с dP_leak_max ПОКАНАЛЬНО. Превышение хотя бы по одному каналу —
// LEAK_DETECTED; в режиме «Вакуум» это ПРЕДУПРЕЖДЕНИЕ, режим продолжается
// (REQ-062), поэтому этап всегда завершается успехом группы.
//
// Этап пропускается, если не задан участок ИЛИ не заданы каналы с порогами.
// Молча «пройти» его нельзя: непроведённая проверка герметичности, показанная
// как успешная, — худший из возможных исходов этого этапа.
ExecutableItem buildLeakTest(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx)
{
    const bool configured = !ctx.leakTestValves.isEmpty() && !ctx.leakChannels.isEmpty();
    auto initial = std::make_shared<QList<Reading>>();

    return Group {
        sequential,
        onGroupSetup([st, ctx, configured, initial]() -> SetupResult {
            if (!configured) {
                const QString why =
                    ctx.leakTestValves.isEmpty()
                        ? QStringLiteral("11.8: участок проверки не задан в профиле "
                                         "(vacuumTract.leakTest) — проверка НЕ проводилась")
                        : QStringLiteral("11.8: порог dP_leak_max не задан в профиле "
                                         "(vacuumSafety.dP_leak_max) — проверка НЕ проводилась");
                skipStage(ctx, VacuumNode::LeakTest, why);
                return SetupResult::StopWithSuccess;
            }
            if (ctx.onNode)  ctx.onNode(VacuumNode::LeakTest, NodeState::Running);
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("11.8: герметичность — выдержка %1 с")
                                .arg(ctx.leakTestDurationSec));

            if (!openTract(st, ctx, ctx.leakTestValves)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("11.8: участок проверки не собран — "
                                                 "клапан не открылся"));
                return SetupResult::StopWithError;
            }
            initial->clear();                       // REQ-060: начальные давления
            for (const LeakChannel& ch : ctx.leakChannels)
                initial->append(ch.read ? ch.read() : Reading(0.0, Quality::NoResponse));
            return SetupResult::Continue;
        }),
        settlePause(ctx),
        // Выдержка идёт через тикер: этап обязан вставать на паузу и отменяться,
        // как всякое ожидание рецепта (см. src/actions/README.md).
        TickerTask([ctx](PausableTicker& t) {
            t.intervalMs = ctx.tickIntervalMs;
            t.pauseBus   = ctx.pauseBus;
            const int hold = ctx.leakTestDurationSec;
            t.isComplete = [hold](int elapsed) { return elapsed >= hold; };
        }),
        QSyncTask([ctx, configured, initial]() -> bool {
            if (!configured)
                return true;
            QStringList leaked;
            QStringList unmeasured;
            for (int i = 0; i < ctx.leakChannels.size(); ++i) {
                const LeakChannel& ch = ctx.leakChannels.at(i);
                const Reading before = i < initial->size() ? initial->at(i)
                                                           : Reading(0.0, Quality::NoResponse);
                const Reading after  = ch.read ? ch.read() : Reading(0.0, Quality::NoResponse);
                // Недостоверное показание на любом конце выдержки означает, что
                // канал НЕ проверен. Считать его герметичным нельзя: ноль с
                // мёртвого датчика выглядит как идеальный результат.
                if (!before.isValid() || !after.isValid()) {
                    unmeasured << ch.name;
                    continue;
                }
                const double rise = after.value - before.value;
                if (rise > ch.maxRise)
                    leaked << QStringLiteral("%1 +%2 (порог %3)")
                                  .arg(ch.name)
                                  .arg(rise,       0, 'g', 3)
                                  .arg(ch.maxRise, 0, 'g', 3);
            }
            if (!leaked.isEmpty() && ctx.onWarning)          // REQ-062
                ctx.onWarning(QStringLiteral("LEAK_DETECTED — проверка герметичности "
                                             "не пройдена: %1. Режим продолжается")
                                  .arg(leaked.join(QStringLiteral("; "))));
            if (!unmeasured.isEmpty() && ctx.onWarning)
                ctx.onWarning(QStringLiteral("11.8: каналы %1 не проверены — "
                                             "недостоверное показание")
                                  .arg(unmeasured.join(QStringLiteral(", "))));
            if (leaked.isEmpty() && unmeasured.isEmpty() && ctx.onLabel)  // REQ-061
                ctx.onLabel(QStringLiteral("11.8: герметичность подтверждена"));
            return true;      // REQ-062: предупреждение, а не остановка режима
        }),
        onGroupDone([st, ctx, configured](DoneWith w) {
            closeTract(st, ctx);
            if (configured && ctx.onNode)
                ctx.onNode(VacuumNode::LeakTest, doneToNode(w));
        })
    };
}

// ── 11.9–11.11: подготовка, финальная откачка камеры, финальное закрытие ──────
//
// REQ-064 и REQ-066 задают ОДИН и тот же набор клапанов (К173, К151, К171,
// К178) плюс активный насосный, поэтому подготовка и откачка идут одной
// группой на одном насосе: закрыть насос между ними означало бы сорвать
// набранный вакуум и начать заново.
//
// REQ-067: если targetVAC лежит в форвакуумной области (≥ порога перехода),
// переход на К179 не обязателен — качаем через К176 и контролируем ДВ301.
// Иначе идёт полная PumpDownProcedure с контролем по ДВ302 (REQ-069).
ExecutableItem buildFinalPumping(const Storage<VacuumRunState>& st,
                                 const VacuumTreeContext& ctx)
{
    const bool configured = !ctx.finalPumpingValves.isEmpty();
    // Выбор области и, вместе с ним, выбор датчика контроля (REQ-069).
    const bool foreVacArea = ctx.targetVacuumPa >= ctx.turboSwitchPressurePa;
    const std::function<Reading()> targetSeam =
        foreVacArea ? ctx.pressureVacPa : ctx.pressureTurboPa;
    const QString targetSensor = foreVacArea ? QStringLiteral("ДВ301")
                                             : QStringLiteral("ДВ302");

    PumpDownOptions opts;
    opts.pumpingSec = ctx.evacTimeSec;   // REQ-068: время — условие завершения
    opts.closePumps = false;             // закрытие — задача 11.11 ниже

    GroupItems items { sequential };

    // ── 11.9: подготовка тракта (REQ-064) ────────────────────────────────────
    items << onGroupSetup([st, ctx, configured]() -> SetupResult {
        if (!configured) {
            skipStage(ctx, VacuumNode::FinalPrep,
                      QStringLiteral("11.9/11.10: набор клапанов финальной откачки "
                                     "не задан в профиле (vacuumTract.finalPumping) — "
                                     "камера НЕ откачивалась"));
            skipStage(ctx, VacuumNode::FinalPumping,
                      QStringLiteral("11.10: пропущена вместе с 11.9"));
            return SetupResult::StopWithSuccess;
        }
        if (ctx.onNode)  ctx.onNode(VacuumNode::FinalPrep, NodeState::Running);
        if (ctx.onLabel) ctx.onLabel(QStringLiteral("11.9: подготовка финальной откачки камеры"));
        if (!openTract(st, ctx, ctx.finalPumpingValves)) {
            if (ctx.onFailure)
                ctx.onFailure(QStringLiteral("11.9: тракт до камеры не собран — "
                                             "клапан не открылся"));
            if (ctx.onNode) ctx.onNode(VacuumNode::FinalPrep, NodeState::Error);
            return SetupResult::StopWithError;
        }
        // REQ-066 называет К178 в минимальном наборе явно. Если конфигурация
        // его уже содержит — второй раз не открываем и в список не дублируем.
        if (!st->tractOpen.contains(VacuumValve::K178)) {
            if (!ctx.setValve(true, VacuumValve::K178)) {
                if (ctx.onFailure)
                    ctx.onFailure(QStringLiteral("11.9: К178 (магистраль) не открылся"));
                if (ctx.onNode) ctx.onNode(VacuumNode::FinalPrep, NodeState::Error);
                return SetupResult::StopWithError;
            }
            st->tractOpen.append(VacuumValve::K178);
        }
        st->k178Open = true;
        if (ctx.onNode) ctx.onNode(VacuumNode::FinalPrep, NodeState::Success);
        return SetupResult::Continue;
    });
    items << settlePause(ctx);

    // ── 11.10: собственно откачка (REQ-065–069) ──────────────────────────────
    items << QSyncTask([ctx, foreVacArea, targetSensor]() -> bool {
        if (ctx.onNode)  ctx.onNode(VacuumNode::FinalPumping, NodeState::Running);
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("11.10: финальная откачка камеры — %1 с, "
                                       "цель %2 Па по %3 (%4 область)")
                            .arg(ctx.evacTimeSec)
                            .arg(ctx.targetVacuumPa)
                            .arg(targetSensor)
                            .arg(foreVacArea ? QStringLiteral("форвакуумная")
                                             : QStringLiteral("турбомолекулярная")));
        return true;
    });

    if (foreVacArea) {
        // REQ-067: турбо не обязателен. Открываем К176 и качаем отведённое
        // время; гейт перехода не набирается вовсе — К179 не открывается.
        items << Group {
            sequential,
            onGroupSetup([st, ctx]() -> SetupResult {
                if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K176)) {
                    if (ctx.onFailure)
                        ctx.onFailure(QStringLiteral("11.10: К176 (форвакуумный насос) "
                                                     "не открылся"));
                    return SetupResult::StopWithError;
                }
                st->k176Open = true;
                return SetupResult::Continue;
            }),
            TickerTask([ctx](PausableTicker& t) {
                t.intervalMs = ctx.tickIntervalMs;
                t.pauseBus   = ctx.pauseBus;
                const int evac = ctx.evacTimeSec;
                t.isComplete = [ctx, evac](int elapsed) {
                    if (ctx.onTurboProgress)
                        ctx.onTurboProgress(VacuumNode::FinalPumping,
                                            ctx.pressureVacPa ? ctx.pressureVacPa()
                                                              : Reading(0.0, Quality::NoResponse),
                                            ctx.pressureTurboPa ? ctx.pressureTurboPa()
                                                                : Reading(0.0, Quality::NoResponse),
                                            0, elapsed);
                    return elapsed >= evac;
                };
            })
        };
    } else {
        items << buildPumpDownProcedure(st, ctx, opts);
    }

    // ── Оценка результата: достигнут ли targetVAC (REQ-069) ──────────────────
    //
    // maxAdditionalEvacTime сознательно НЕ реализован: продление отменено
    // бюджетом времени прогона (согласованная дивергенция от REQ-033/069,
    // см. CLAUDE.md и docs/regimes/vacuum.md). Недостигнутая цель — не отказ
    // режима, а зафиксированная причина завершения: этап закрывает тракт
    // штатно и сообщает оператору, чего именно не хватило.
    items << QSyncTask([ctx, targetSeam, targetSensor]() -> bool {
        const Reading r = targetSeam ? targetSeam()
                                     : Reading(0.0, Quality::NoResponse);
        if (r.isValid() && r.value <= ctx.targetVacuumPa) {
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("11.10: цель достигнута — %1 %2 Па ≤ %3 Па")
                                .arg(targetSensor).arg(r.value, 0, 'g', 3)
                                .arg(ctx.targetVacuumPa));
            return true;
        }
        if (ctx.onFailure)
            ctx.onFailure(QStringLiteral("11.10: время откачки (%1 с) истекло, цель "
                                         "%2 Па не достигнута — %3 %4 Па (%5)")
                              .arg(ctx.evacTimeSec)
                              .arg(ctx.targetVacuumPa)
                              .arg(targetSensor)
                              .arg(r.value, 0, 'g', 3)
                              .arg(qualityName(r.quality)));
        return true;   // не отказ режима: причина записана, идём в 11.11
    });

    // ── 11.11: финальное закрытие (REQ-070–073) ──────────────────────────────
    items << onGroupDone([st, ctx, configured, targetSeam, targetSensor](DoneWith w) {
        if (!configured)
            return;
        if (ctx.onNode) {
            ctx.onNode(VacuumNode::FinalPumping, doneToNode(w));
            ctx.onNode(VacuumNode::FinalClose, NodeState::Running);
        }
        // Насосные клапаны закрываются первыми и по отдельности: активный насос
        // обязан быть отсечён от тракта раньше, чем тракт начнёт разбираться.
        if (st->k179Open) {
            if (ctx.setValve) ctx.setValve(false, VacuumValve::K179);
            st->k179Open = false;
        }
        if (st->k176Open) {
            if (ctx.setValve) ctx.setValve(false, VacuumValve::K176);
            st->k176Open = false;
        }
        closeTract(st, ctx);                 // REQ-070/071, каждый клапан отдельно
        st->k178Open = false;
        st->k151Open = false;
        // REQ-072: финальное показание контрольного датчика в журнал.
        if (ctx.onLabel) {
            const Reading r = targetSeam ? targetSeam()
                                         : Reading(0.0, Quality::NoResponse);
            ctx.onLabel(QStringLiteral("11.11: клапаны закрыты, финальное %1 %2 Па (%3)")
                            .arg(targetSensor).arg(r.value, 0, 'g', 3)
                            .arg(qualityName(r.quality)));
        }
        if (ctx.onNode)
            ctx.onNode(VacuumNode::FinalClose, doneToNode(w));
    });

    return Group(items);
}

ExecutableItem executionPhase(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              const RepeatIterator& iter)
{
    GroupItems items { sequential };

    items << QSyncTask([ctx] {                             // s1: 90018 метка
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("Ф1: стартовый сброс (s1)"));
    });
    items << settlePause(ctx);
    items << withNode(ctx, VacuumNode::F1Relief,
                      reliefTriplet(st, ctx, iter));       // Ф1: s2–s4
    items << settlePause(ctx);
    items << withNode(ctx, VacuumNode::F2BlockC,
                      buildBlockC(st, ctx, iter, BlockCOptions{ /*reliefAndClose=*/true })); // Ф2: s5–s19
    items << settlePause(ctx);
    items << secondTractGroup(st, ctx, iter);              // Ф3: s20–s24 (узел inline)

    // Форвакуумная откачка 11.5–11.7 (gated foreVacuum; skipUnlessNode эмитит
    // Skipped при выключении — как ветки блока C).
    items << settlePause(ctx);
    items << skipUnlessNode(ctx, VacuumNode::F5A1, ctx.foreVacuum,
                            { buildForevacA1(st, ctx, iter) });
    items << settlePause(ctx);
    items << skipUnlessNode(ctx, VacuumNode::F6BC, ctx.foreVacuum,
                            { buildForevacBC(st, ctx) });
    items << settlePause(ctx);
    items << skipUnlessNode(ctx, VacuumNode::F7EF, ctx.foreVacuum,
                            { buildForevacEF(st, ctx) });

    // Раздел 12.2 — переход на турбомолекулярный насос. Условий три, и каждое
    // отключает этап целиком (узлы помечаются Skipped, как ветки блока C):
    //   turboTract      — Advanced-опция «исключить тракт турбонасоса»;
    //   foreVacuum      — без него ДВ301 не доводится до порога перехода;
    //   pressureTurboPa — без ДВ302 условие У3 гейта недостижимо в принципе.
    // Последнее — именно пропуск, а не ожидание таймаута: висеть turboTimeoutSec
    // ради заведомо известного исхода бессмысленно, а «этап не выполнялся» и
    // «этап не смог» — разные вещи для оператора и для журнала.
    const bool turboPossible = ctx.turboTract && ctx.foreVacuum && bool(ctx.pressureTurboPa);

    // 11.7б: общая откачка — тракт К171/К173/К151 + применимые C, магистраль
    // К178, затем PumpDownProcedure и testEvacTimeSec на турбонасосе.
    // Именно здесь режим впервые качает НЕ отсечённую магистраль, а систему.
    items << settlePause(ctx);
    items << skipUnlessNode(ctx, VacuumNode::TurboGate, turboPossible,
                            { buildGeneralPumping(st, ctx) });

    // 11.8: герметичность. Идёт независимо от турбо-тракта: этап работает на
    // закрытых насосах и меряет прирост давления, а не откачивает.
    items << settlePause(ctx);
    items << buildLeakTest(st, ctx);

    // 11.9–11.11: подготовка тракта до камеры, финальная откачка, закрытие.
    // Турбо-область требует ДВ302 и разрешённого тракта; форвакуумная область
    // (REQ-067) обходится К176 и потому от turboPossible не зависит.
    const bool finalNeedsTurbo = ctx.targetVacuumPa < ctx.turboSwitchPressurePa;
    const bool finalPossible    = !finalNeedsTurbo || turboPossible;
    items << settlePause(ctx);
    // Узлы FinalPrep/FinalPumping/FinalClose эмитит сама группа — обернуть её
    // в skipUnlessNode нельзя, иначе те же узлы получат по два состояния
    // подряд и развёртка начнёт мигать. Пропуск объявляется явно и с причиной.
    items << Group {
        sequential,
        onGroupSetup([ctx, finalPossible]() -> SetupResult {
            if (finalPossible)
                return SetupResult::Continue;
            skipStage(ctx, VacuumNode::FinalPrep,
                      QStringLiteral("11.9/11.10: цель %1 Па лежит в турбомолекулярной "
                                     "области, а тракт турбонасоса недоступен "
                                     "(ДВ302 или Advanced-опция) — камера НЕ откачивалась")
                          .arg(ctx.targetVacuumPa));
            skipStage(ctx, VacuumNode::FinalPumping,
                      QStringLiteral("11.10: пропущена вместе с 11.9"));
            return SetupResult::StopWithSuccess;
        }),
        buildFinalPumping(st, ctx)
    };

    return Group(items);
}

// ── Непрерывная откачка (опция) ───────────────────────────────────────────────
// «В конце автоматического режима оставить рабочий тракт открытым под
// продолжительную откачку ТУРБОМОЛЕКУЛЯРНЫМ насосом». Выполняется ОДИН раз —
// после последнего повтора, вне тела For, поэтому повтор N+1 никогда не
// стартует с открытым насосным клапаном.
//
// Тракт, который остаётся открытым (порядок — от объёмов к насосу):
//   К151 (R3)  — камера F + линия E;
//   эталонный резервуар B — собственного клапана не имеет, он на магистрали
//                 и попадает в тракт вместе с ней;
//   К178 (AR5) — бочка / магистраль;
//   К179 (SL2) — турбомолекулярный насос, подключается последним.
// Форвакуумный К176 (AR6) при этом ЗАКРЫТ — интерлок REQ-008/084; закрытие
// и подтверждение readback идут через ту же connectTurboPump, что и переход
// 12.2, поэтому обойти проверку состояния К176 здесь нельзя.
//
// Сознательно НЕ открываются:
//   К118 (AR4)       — сброс в атмосферу, открытие сорвало бы вакуум;
//   C1/C2/C3         — баллоны блока C изолируются: качается рабочий тракт,
//                      а не весь объём установки;
//   К192 (SL1)       — выход второго тракта.
// Клапаны остаются открытыми намеренно: закрывать их — задача оператора.
//
// Этап пропускается, если опция выключена ИЛИ хотя бы один повтор завершился
// ошибкой: тракт открывается только после штатного прогона. Отмена (Стоп)
// сюда не доходит вовсе — For возвращает Error, sequential-группа обрывается.
//
// Опция читается ЗДЕСЬ, в момент выполнения хвоста (шов continuousPumpingLive),
// а не на старте прогона: оператор вправе передумать, пока режим идёт.
ExecutableItem continuousPumpingTail(const VacuumTreeContext& ctx,
                                     const Storage<VacuumRunSummary>& summary)
{
    auto openValve = [ctx](const QString& valve) {
        return QSyncTask([ctx, valve]() -> bool {
            return ctx.setValve && ctx.setValve(true, valve);
        });
    };
    auto ran = std::make_shared<bool>(false);

    GroupItems items { sequential };
    items << onGroupSetup([ctx, summary, ran]() -> SetupResult {
        const bool wanted = ctx.continuousPumpingLive ? ctx.continuousPumpingLive()
                                                      : ctx.continuousPumping;
        *ran = wanted && summary->repeatsError == 0;
        if (!*ran) {
            if (ctx.onNode && wanted)
                ctx.onNode(VacuumNode::ContinuousPumping, NodeState::Skipped);
            return SetupResult::StopWithSuccess;
        }
        if (ctx.onNode)
            ctx.onNode(VacuumNode::ContinuousPumping, NodeState::Running);
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("Непрерывная откачка: тракт (камера E/F, "
                                       "эталон B, бочка) оставлен на турбонасосе"));
        return SetupResult::Continue;
    });

    items << openValve(VacuumValve::K151);       // камера F + линия E
    items << settlePause(ctx);
    items << openValve(VacuumValve::K178);       // бочка / магистраль (эталон B на ней)
    items << settlePause(ctx);
    // Насос подключается последним, к уже собранному тракту: закрыть К176,
    // подтвердить закрытие чтением состояния, открыть К179 (REQ-008/082/084).
    items << QSyncTask([ctx]() -> bool {
        if (!connectTurboPump(ctx, /*requireConfirm=*/true, /*k176Open=*/nullptr))
            return false;
        if (ctx.onTurboSwitched)
            ctx.onTurboSwitched(true);
        return true;
    });

    items << onGroupDone([ctx, ran](DoneWith w) {
        if (*ran && ctx.onNode)
            ctx.onNode(VacuumNode::ContinuousPumping, doneToNode(w));
        if (*ran && w != DoneWith::Success && ctx.onFailure)
            ctx.onFailure(QStringLiteral("Непрерывная откачка: не удалось открыть тракт"));
    });
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
        // Опциональный финал: тракт остаётся открытым под продолжительную откачку.
        continuousPumpingTail(ctx, summary),
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
