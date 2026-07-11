#include "VacuumTaskTree.h"

#include <QDebug>

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
// Примитивы рецепта
// ═════════════════════════════════════════════════════════════════════════════

using TickerTask = QCustomTask<PausableTicker>;

namespace {

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
            if (ctx.skipRK300)
                return SetupResult::StopWithSuccess;       // s5: false → goto_6
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K135))
                return SetupResult::StopWithError;         // s6: 11135
            st->s3Open = true;
            return SetupResult::Continue;
        }),
        opts.reliefAndClose ? GroupItem(reliefTriplet(st, ctx, iter))  // s7–s9
                            : nullItem,
        onGroupDone([st, ctx, opts](DoneWith w) {          // s10: 10135
            if (st->s3Open && (opts.reliefAndClose || w != DoneWith::Success)) {
                if (ctx.setValve)
                    ctx.setValve(false, VacuumValve::K135);
                st->s3Open = false;
            }
        })
    };

    GroupItems items { sequential };
    items << s3Group;

    // s11–s12: страж RK10 (21003, cond_phase_3; false → goto_2 = пропустить
    // ровно строку открытия) → 11131. s13–s14: аналогично RK50 → 11133.
    // Стражи независимы и последовательны (не вложены): RK50 проверяется
    // даже когда RK10 пропущен — так в CSV.
    items << skipUnless(!ctx.skipRK10,
                        { openValveTask(st, ctx, VacuumValve::K131, &VacuumRunState::s1Open) });
    items << skipUnless(!ctx.skipRK50,
                        { openValveTask(st, ctx, VacuumValve::K133, &VacuumRunState::s2Open) });

    if (opts.reliefAndClose)
        items << reliefTriplet(st, ctx, iter);             // s15–s17: безусловный триплет

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
            if (!ctx.secondTract)
                return SetupResult::StopWithSuccess;       // s20: false → goto_5
            if (!ctx.setValve || !ctx.setValve(true, VacuumValve::K192))
                return SetupResult::StopWithError;         // s21: 11192
            st->sl2Open = true;
            if (!ctx.setValve(true, VacuumValve::K179))
                return SetupResult::StopWithError;         // s22: 11179
            st->sl1Open = true;
            return SetupResult::Continue;
        }),
        pausableDelay(st, ctx, ctx.reliefDwellSec, iter),  // s22: выдержка time_5000
        onGroupDone([st, ctx](DoneWith) {                  // s23–s24: SL2, затем SL1
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

// ── Ф4–Ф8 (s25+): заглушка ────────────────────────────────────────────────────
// Откачка К178 (11178 time_5000), страж ВПА1 (21001, Conditions.cpp:288:
// m_virA_1 < UP_VAL /0.02/ — шаблон условия №2: поллинг + fault по таймауту,
// stopOnSuccess + For(RepeatIterator(20)); при реализации проверить полярность
// UntilIterator), пульсации К176 (12176 rep_20), блок C проход 2
// (buildBlockC с {reliefAndClose=false}), финальное закрытие 10900.
//
// Инвариант 2 закладывается структурно: пульсации К176 и открытие К178 будут
// взаимоисключающими ветвями одной группы, а не флагом.
ExecutableItem buildPumpdown(const Storage<VacuumRunState>& st,
                             const VacuumTreeContext& ctx)
{
    Q_UNUSED(st)
    return QSyncTask([ctx] {
        qDebug() << "[Вакуум] Ф4–Ф8 (s25+): откачка К178/К176 — TODO (заглушка)";
        if (ctx.onLabel)
            ctx.onLabel(QStringLiteral("Ф4–Ф8: заглушка (s25+)"));
    });
}

// ── Execution-фаза одного повтора: Ф1 → Ф2 → Ф3 → [Ф4+ заглушка] ─────────────
ExecutableItem executionPhase(const Storage<VacuumRunState>& st,
                              const VacuumTreeContext& ctx,
                              const RepeatIterator& iter)
{
    return Group {
        sequential,
        QSyncTask([ctx] {                                  // s1: 90018 метка
            if (ctx.onLabel)
                ctx.onLabel(QStringLiteral("Ф1: стартовый сброс (s1)"));
        }),
        reliefTriplet(st, ctx, iter),                      // Ф1: s2–s4
        buildBlockC(st, ctx, iter, BlockCOptions{ /*reliefAndClose=*/true }), // Ф2: s5–s19
        secondTractGroup(st, ctx, iter),                   // Ф3: s20–s24
        buildPumpdown(st, ctx),                            // Ф4–Ф8: заглушка
    };
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
                conditionPhase(ctx, iter),
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
