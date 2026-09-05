// ─────────────────────────────────────────────────────────────────────────────
// VacuumTreeTests — тесты рецепта режима «Вакуум» (Ф1–Ф3, s1–s24) без железа.
//
// Критерии приёмки:
//   A — dry-run: последовательность изменений состояний клапанов совпадает с
//       s1–s24 из GramQt testSeq/vacuum_cond.csv, включая все skip-ветки;
//   B — cancel в каждой точке «клапан открыт, идёт выдержка» приводит к
//       безопасному состоянию (все клапаны закрыты, порядок изнутри-наружу);
//   C — повторный запуск не наследует состояния (Storage пересоздаётся).
//
// Все швы — через VacuumTreeContext; ValveControl/biodaq не нужны.
// ─────────────────────────────────────────────────────────────────────────────

#include "VacuumTaskTree.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <memory>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <qtasktree.h>

using namespace QtTaskTree;

namespace {

// ─── Recorder: мок клапанов и давления ────────────────────────────────────────

struct ValveOp {
    bool    open;
    QString name;
};

class Recorder
{
public:
    QList<ValveOp>     ops;
    QMap<QString, bool> state;
    QSet<QString>      blockedOpens;                       // открытие этих имён — отказ
    std::function<double()> pressure = [] { return 2.0; }; // > DB_SBR_LIM по умолчанию

    VacuumTreeContext makeCtx()
    {
        VacuumTreeContext ctx;
        ctx.tickIntervalMs   = 1;  // 1 тик = 1 мс, чтобы тесты были быстрыми
        ctx.perActionPauseMs = 0;  // без settle-пауз: структура дерева как в эталоне
        ctx.reliefDwellSec   = 5;  // пин: эталонные ассерты elapsed не зависят от дефолта
        ctx.pumpCheckDelaySec = 0; // «мёртвая зона» dP/dt — отдельный тест
        ctx.foreVacuum       = false;  // Ф1–Ф3 в изоляции; форвакуум — отдельные тесты
        ctx.setValve = [this](bool open, const QString& name) -> bool {
            if (open && blockedOpens.contains(name))
                return false;
            ops.append({open, name});
            state[name] = open;
            return true;
        };
        ctx.pressureB = [this] { return pressure(); };
        return ctx;
    }

    QString sequence() const
    {
        QStringList parts;
        for (const ValveOp& op : ops)
            parts << (op.open ? QStringLiteral("+") : QStringLiteral("-")) + op.name;
        return parts.join(QStringLiteral(" "));
    }

    bool allClosed() const
    {
        for (bool open : state)
            if (open)
                return false;
        return true;
    }
};

// Эталон s1–s24 при всех включённых ветках (изменения состояний клапанов):
// Ф1: s2–s4; Ф2: s5–s19; Ф3: s20–s24.
const QString kFullSequence = QStringLiteral(
    "+AR4 -AR4 "                             // Ф1: триплет К118
    "+S3 +AR4 -AR4 -S3 "                     // Ф2: RK300 + триплет + закрытие
    "+S1 +S2 +AR4 -AR4 -S2 -S1 "             // Ф2: RK10/RK50 + триплет + закрытия
    "+SL2 +SL1 -SL2 -SL1");                  // Ф3: второй тракт

DoneWith runBlocking(const Group& recipe)
{
    QTaskTree tree(recipe);
    return tree.runBlocking();
}

// Запуск с отменой на заданном суммарном тике выдержки (elapsedSec).
struct CancelRun {
    Recorder rec;
    DoneWith result = DoneWith::Success;
};

CancelRun runWithCancelAt(int cancelTick)
{
    CancelRun run;
    VacuumTreeContext ctx = run.rec.makeCtx();
    ctx.secondTract = true;

    QTaskTree tree;
    ctx.onProgress = [&tree, cancelTick](int elapsed, int /*repeat*/) {
        // Отмена из чужого стека (queued), не из тика таймера задачи —
        // cancel() синхронно уничтожает выполняющуюся задачу.
        if (elapsed == cancelTick)
            QTimer::singleShot(0, &tree, [&tree] { tree.cancel(); });
    };
    tree.setRecipe(buildVacuumRecipe(ctx));

    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&](DoneWith w) {
        run.result = w;
        loop.quit();
    });
    tree.start();
    loop.exec();
    return run;
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Критерий A: dry-run последовательность
// ═════════════════════════════════════════════════════════════════════════════

TEST(VacuumTree, FullSequenceAllBranchesOn)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence(), kFullSequence);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, AllSkippedAndPressureLowProducesNoValveOps)
{
    Recorder rec;
    rec.pressure = [] { return 1.0; };  // <= DB_SBR_LIM: стражи ДВ_СБР пропускают
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = true;
    ctx.secondTract = false;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.ops.isEmpty()) << rec.sequence().toStdString();
}

TEST(VacuumTree, SkipFlagsStillRunUnconditionalTriplets)
{
    // Пропуск всего блока C не отменяет безусловные триплеты s2–s4 и s15–s17.
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = true;
    ctx.secondTract = false;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence(), QStringLiteral("+AR4 -AR4 +AR4 -AR4"));
}

TEST(VacuumTree, ReliefGuardIsEvaluatedPerTriplet)
{
    // Страж ДВ_СБР опрашивается на входе каждого триплета: первый сброс
    // выполняется (P > 1.65), последующие пропускаются (P упало).
    Recorder rec;
    int calls = 0;
    rec.pressure = [&calls] { return ++calls == 1 ? 2.0 : 1.0; };
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = false;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(calls, 3);  // Ф1, Ф2-S3, Ф2 s15–s17
    EXPECT_EQ(rec.sequence(),
              QStringLiteral("+AR4 -AR4 +S3 -S3 +S1 +S2 -S2 -S1"));
}

TEST(VacuumTree, SkipRk300OnlyOmitsItsSubgroup)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK300 = true;
    ctx.secondTract = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence(),
              QStringLiteral("+AR4 -AR4 "
                             "+S1 +S2 +AR4 -AR4 -S2 -S1 "
                             "+SL2 +SL1 -SL2 -SL1"));
}

TEST(VacuumTree, ProgressTicksMatchDwells)
{
    // 4 выдержки по reliefDwellSec=5 тиков: elapsed накапливается 1..20.
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;
    QList<int> progress;
    ctx.onProgress = [&progress](int elapsed, int /*repeat*/) {
        progress << elapsed;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    ASSERT_EQ(progress.size(), 20);
    for (int i = 0; i < progress.size(); ++i)
        EXPECT_EQ(progress.at(i), i + 1);
}

// ═════════════════════════════════════════════════════════════════════════════
// Критерий B: отмена в каждой точке «клапан открыт, идёт выдержка»
// ═════════════════════════════════════════════════════════════════════════════
// Точки (all-on, elapsed по выдержкам): Ф1 К118 = 1–5; Ф2 S3-триплет = 6–10;
// Ф2 s15–s17 (S1+S2 открыты) = 11–15; Ф3 (SL2+SL1) = 16–20.

TEST(VacuumTree, CancelDuringPhase1ReliefClosesK118)
{
    CancelRun run = runWithCancelAt(3);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    EXPECT_EQ(run.rec.sequence(), QStringLiteral("+AR4 -AR4"));
}

TEST(VacuumTree, CancelDuringS3TripletClosesInsideOut)
{
    CancelRun run = runWithCancelAt(8);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    // Изнутри-наружу: сначала К118 (вложенный триплет), затем S3 (объемлющая группа)
    EXPECT_EQ(run.rec.sequence(),
              QStringLiteral("+AR4 -AR4 +S3 +AR4 -AR4 -S3"));
}

TEST(VacuumTree, CancelDuringBlockCTripletClosesAllOpened)
{
    CancelRun run = runWithCancelAt(13);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    // К118 (триплет) → S2 → S1 (порядок закрытий CSV s18–s19)
    EXPECT_EQ(run.rec.sequence(),
              QStringLiteral("+AR4 -AR4 +S3 +AR4 -AR4 -S3 "
                             "+S1 +S2 +AR4 -AR4 -S2 -S1"));
}

TEST(VacuumTree, CancelDuringSecondTractClosesBothValves)
{
    CancelRun run = runWithCancelAt(18);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    EXPECT_EQ(run.rec.sequence(),
              kFullSequence);  // отмена в Ф3 добирает те же закрытия SL2, SL1
}

// ═════════════════════════════════════════════════════════════════════════════
// Критерий C: повторный запуск не наследует состояния
// ═════════════════════════════════════════════════════════════════════════════

TEST(VacuumTree, RerunSameRecipeStartsFresh)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;

    Storage<VacuumRunState>   st;
    Storage<VacuumRunSummary> summary;
    const Group recipe = buildVacuumRecipe(ctx, st, summary);

    int storagesCreated = 0;
    auto assertFresh = [&storagesCreated](VacuumRunState& s) {
        ++storagesCreated;
        EXPECT_FALSE(s.k118Open);
        EXPECT_FALSE(s.s1Open);
        EXPECT_FALSE(s.s2Open);
        EXPECT_FALSE(s.s3Open);
        EXPECT_FALSE(s.sl1Open);
        EXPECT_FALSE(s.sl2Open);
        EXPECT_EQ(s.elapsedSec, 0);
        EXPECT_EQ(s.reliefCount, 0);
    };

    QTaskTree tree1(recipe);
    tree1.onStorageSetup(st, assertFresh);
    EXPECT_EQ(tree1.runBlocking(), DoneWith::Success);
    EXPECT_EQ(rec.sequence(), kFullSequence);

    rec.ops.clear();

    QTaskTree tree2(recipe);
    tree2.onStorageSetup(st, assertFresh);
    EXPECT_EQ(tree2.runBlocking(), DoneWith::Success);
    EXPECT_EQ(rec.sequence(), kFullSequence);

    EXPECT_EQ(storagesCreated, 2);
}

TEST(VacuumTree, RepeatsRecreateStateAndDoubleSequence)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract  = true;
    ctx.totalRepeats = 2;

    QList<int> repeats;
    ctx.onRepeatDone = [&repeats](DoneWith w, int repeat) {
        EXPECT_EQ(w, DoneWith::Success);
        repeats << repeat;
    };
    int maxElapsed = 0;
    ctx.onProgress = [&maxElapsed](int elapsed, int /*repeat*/) {
        maxElapsed = qMax(maxElapsed, elapsed);
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence(), kFullSequence + QStringLiteral(" ") + kFullSequence);
    EXPECT_EQ(repeats, (QList<int>{0, 1}));
    EXPECT_EQ(maxElapsed, 20);  // elapsedSec пер-повторный, не накапливается между повторами
}

// ═════════════════════════════════════════════════════════════════════════════
// Ошибка открытия (Security-блокировка), пауза, condition-фаза, итоги
// ═════════════════════════════════════════════════════════════════════════════

TEST(VacuumTree, BlockedSecondTractValveClosesAlreadyOpened)
{
    Recorder rec;
    rec.blockedOpens.insert(QStringLiteral("SL1"));
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;

    QList<DoneWith> repeatResults;
    ctx.onRepeatDone = [&repeatResults](DoneWith w, int /*repeat*/) {
        repeatResults << w;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
    // SL2 успел открыться, SL1 заблокирован → откат: закрыть SL2
    EXPECT_TRUE(rec.sequence().endsWith(QStringLiteral("+SL2 -SL2")))
        << rec.sequence().toStdString();
    EXPECT_EQ(repeatResults, (QList<DoneWith>{DoneWith::Error}));
}

TEST(VacuumTree, RunFinishedReportsCounts)
{
    Recorder rec;
    rec.blockedOpens.insert(QStringLiteral("SL1"));
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;

    int reportedDone = -1, reportedError = -1;
    ctx.onRunFinished = [&](DoneWith /*w*/, int repeatsDone, int repeatsError) {
        reportedDone  = repeatsDone;
        reportedError = repeatsError;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_EQ(reportedDone, 0);
    EXPECT_EQ(reportedError, 1);
}

TEST(VacuumTree, PauseFreezesDwellAndResumeCompletes)
{
    Recorder rec;
    PauseBus bus;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;
    ctx.pauseBus    = &bus;

    QTaskTree tree;
    int ticksWhilePaused = 0;
    ctx.onProgress = [&](int elapsed, int /*repeat*/) {
        if (bus.isPaused())
            ++ticksWhilePaused;
        if (elapsed == 2) {
            QTimer::singleShot(0, &bus, [&bus] { bus.pause(); });
            QTimer::singleShot(30, &bus, [&bus] { bus.resume(); });
        }
    };
    tree.setRecipe(buildVacuumRecipe(ctx));

    QEventLoop loop;
    DoneWith result = DoneWith::Error;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&](DoneWith w) {
        result = w;
        loop.quit();
    });
    tree.start();
    loop.exec();

    EXPECT_EQ(result, DoneWith::Success);
    EXPECT_EQ(ticksWhilePaused, 0);  // во время паузы тики не идут
    EXPECT_EQ(rec.sequence(), kFullSequence);
}

TEST(VacuumTree, ConditionPhaseTimeRunsBeforeValves)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.conditionType    = QStringLiteral("time");
    ctx.conditionTimeSec = 3;

    QList<int> condProgress;
    ctx.onConditionProgress = [&](int elapsed, int /*repeat*/) {
        condProgress << elapsed;
    };
    int  condDoneCalls          = 0;
    bool valveOpsBeforeCondDone = false;
    ctx.onConditionDone = [&](int repeat) {
        ++condDoneCalls;
        EXPECT_EQ(repeat, 0);
        valveOpsBeforeCondDone = !rec.ops.isEmpty();
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(condProgress, (QList<int>{1, 2, 3}));
    EXPECT_EQ(condDoneCalls, 1);
    EXPECT_FALSE(valveOpsBeforeCondDone);
}

TEST(VacuumTree, ConditionNoneConfirmsImmediately)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    int condDoneCalls = 0;
    ctx.onConditionDone = [&condDoneCalls](int /*repeat*/) { ++condDoneCalls; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(condDoneCalls, 1);
}

// ═════════════════════════════════════════════════════════════════════════════
// Форвакуумная откачка 11.5–11.7 (A1, B/C, E/F)
// ═════════════════════════════════════════════════════════════════════════════

namespace {
// Контекст с включённым форвакуумом; блок C и триплеты подавлены (skip + низкое
// давление), чтобы в последовательности остались только форвак-операции.
VacuumTreeContext makeForevacCtx(Recorder& rec)
{
    rec.pressure = [] { return 1.0; };           // ≤ DB_SBR_LIM: триплеты пропущены
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.foreVacuum        = true;
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = true;  // блок C без операций
    ctx.secondTract       = false;
    ctx.foreVacHoldSec     = 2;
    ctx.k178PulseMs        = 1;
    ctx.foreVacTimeoutSec  = 100;
    ctx.pumpRateCheck      = false;      // dP/dt-watchdog — отдельные тесты
    return ctx;
}
} // namespace

TEST(VacuumTree, ForevacStagesOpenAndClosePumpTract)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa = [] { return 5.0; };      // ≤ targetVacPa → удержание набирается

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    // A1: импульс К178 + форвакуум К176; B/C: то же (C пропущены);
    // E/F: +К151, импульс К178, форвакуум К176, закрытие К151.
    EXPECT_EQ(rec.sequence(),
              QStringLiteral("+AR5 -AR5 +AR6 -AR6 "     // 11.5 A1
                             "+AR5 -AR5 +AR6 -AR6 "     // 11.6 B/C
                             "+R3 +AR5 -AR5 +AR6 -AR6 -R3"));  // 11.7 E/F
}

TEST(VacuumTree, ForevacTimesOutWhenPressureStaysHigh)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 100.0; };  // > targetVacPa: удержание не набрать
    ctx.foreVacTimeoutSec = 3;                      // быстрый таймаут

    // Первый же форвак-этап (A1) падает по таймауту → весь повтор Error,
    // но насосный тракт закрыт (К176), остальные этапы пропущены.
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_EQ(rec.sequence(), QStringLiteral("+AR5 -AR5 +AR6 -AR6"));
}

TEST(VacuumTree, ForevacMissingSensorFailsSafe)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = nullptr;   // датчик не задан
    ctx.foreVacTimeoutSec = 3;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, CancelDuringForevacHoldClosesPump)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa = [] { return 100.0; };   // никогда не удержится → висит на hold

    QTaskTree tree;
    tree.setRecipe(buildVacuumRecipe(ctx));

    QEventLoop loop;
    DoneWith result = DoneWith::Success;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&](DoneWith w) {
        result = w;
        loop.quit();
    });
    // Отмена посреди удержания форвакуума A1 (из чужого стека, queued).
    QTimer::singleShot(10, &tree, [&tree] { tree.cancel(); });
    tree.start();
    loop.exec();

    EXPECT_EQ(result, DoneWith::Cancel);
    EXPECT_TRUE(rec.allClosed());  // К176/К178 закрыты cleanup-хендлерами
}

// ═════════════════════════════════════════════════════════════════════════════
// dP/dt-watchdog после открытия К176 (REQ-020/076) + OperatorBus
// ═════════════════════════════════════════════════════════════════════════════

TEST(VacuumTree, PumpWatchdogFailsSafeWithoutOperator)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pumpRateCheck      = true;
    ctx.pressureVacPa      = [] { return 100.0; };  // давление не падает
    ctx.pumpCheckWindowSec = 2;
    ctx.pumpMinDropPa      = 1.0;
    // operatorBus не задан → «откачка не идёт» = безопасный отказ
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, PumpWatchdogStopDecisionStopsRegime)
{
    Recorder rec;
    OperatorBus bus;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pumpRateCheck      = true;
    ctx.pressureVacPa      = [] { return 100.0; };
    ctx.pumpCheckWindowSec = 2;
    ctx.pumpMinDropPa      = 1.0;
    ctx.operatorBus        = &bus;
    int prompts = 0;
    QObject::connect(&bus, &OperatorBus::decisionRequired, &bus,
                     [&bus, &prompts](int, QString) { ++prompts; bus.respond(OperatorBus::Stop); });

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_GE(prompts, 1);           // диалог был поднят
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, PumpWatchdogPassesWhenPressureDrops)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pumpRateCheck      = true;
    ctx.pumpCheckWindowSec = 1;
    ctx.pumpMinDropPa      = 1.0;
    ctx.foreVacHoldSec     = 1;
    ctx.targetVacPa        = 5.0;
    // Давление монотонно падает на 2 Па/вызов → watchdog всех стадий проходит,
    // а hold ≤5 Па набирается.
    auto p = std::make_shared<double>(100.0);
    ctx.pressureVacPa = [p] { double v = *p; *p -= 2.0; return v; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, PumpWatchdogContinueRetriesThenSucceeds)
{
    Recorder rec;
    OperatorBus bus;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pumpRateCheck      = true;
    ctx.pumpCheckWindowSec = 1;
    ctx.pumpMinDropPa      = 1.0;
    ctx.foreVacHoldSec     = 1;
    ctx.targetVacPa        = 5.0;
    ctx.operatorBus        = &bus;

    // Первое окно — давление стоит (100); после первого «Продолжить» начинает
    // падать → следующая проверка проходит, режим завершается успешно.
    auto p       = std::make_shared<double>(100.0);
    auto dropping = std::make_shared<bool>(false);
    ctx.pressureVacPa = [p, dropping] {
        double v = *p;
        if (*dropping) *p -= 2.0;
        return v;
    };
    QObject::connect(&bus, &OperatorBus::decisionRequired, &bus,
                     [&bus, dropping](int, QString) {
                         *dropping = true;               // включаем откачку
                         bus.respond(OperatorBus::Continue);
                     });

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

// ─── «Мёртвая зона» перед первым окном dP/dt (30 с в бою) ────────────────────
//
// Точка p0 берётся ПОСЛЕ задержки, поэтому падение давления за время выхода
// насоса на режим в окно не засчитывается — и наоборот, ложный отказ на
// over-range первых секунд невозможен. Оба теста используют один и тот же
// профиль давления и различаются только startDelaySec.
namespace {
// 1000 → 10 Па за первые 200 мс, дальше плато.
std::function<double()> decayingThenFlat()
{
    auto t0 = std::make_shared<QElapsedTimer>();
    t0->start();
    return [t0]() -> double {
        const qint64 ms = t0->elapsed();
        return ms < 200 ? 1000.0 - ms * 4.95 : 10.0;
    };
}

bool runWatchdog(PumpRateWatchdog& w)
{
    bool verdict = false;
    QEventLoop loop;
    QObject::connect(&w, &PumpRateWatchdog::done, &loop, [&](bool ok) {
        verdict = ok;
        loop.quit();
    });
    w.start();
    loop.exec();
    return verdict;
}
} // namespace

TEST(VacuumTree, PumpWatchdogWithoutDelayCountsInitialDrop)
{
    PumpRateWatchdog w;
    w.pressurePa    = decayingThenFlat();
    w.intervalMs    = 50;
    w.startDelaySec = 0;          // окно открывается сразу
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    EXPECT_TRUE(runWatchdog(w));  // падение первых миллисекунд засчитано
}

TEST(VacuumTree, PumpWatchdogDelaySkipsInitialDrop)
{
    PumpRateWatchdog w;
    w.pressurePa    = decayingThenFlat();
    w.intervalMs    = 50;
    w.startDelaySec = 6;          // 300 мс — падение целиком внутри «мёртвой зоны»
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    // Без operatorBus вердикт «откачка не идёт» = done(false): к моменту окна
    // давление уже на плато, значит p0 взята после задержки, а не до неё.
    EXPECT_FALSE(runWatchdog(w));
}

TEST(VacuumTree, PumpWatchdogDelayAppliesOnlyToFirstWindow)
{
    // «Продолжить» после отказа перепроверяет немедленно: насос уже работает.
    OperatorBus bus;
    PumpRateWatchdog w;
    auto p        = std::make_shared<double>(100.0);
    auto dropping = std::make_shared<bool>(false);
    w.pressurePa    = [p, dropping] { double v = *p; if (*dropping) *p -= 5.0; return v; };
    w.intervalMs    = 20;
    w.startDelaySec = 5;          // 100 мс только перед первым окном
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    w.operatorBus   = &bus;

    int prompts = 0;
    QObject::connect(&bus, &OperatorBus::decisionRequired, &bus,
                     [&bus, &prompts, dropping](int, QString) {
                         ++prompts;
                         *dropping = true;             // откачка «пошла»
                         bus.respond(OperatorBus::Continue);
                     });

    QElapsedTimer t;
    t.start();
    EXPECT_TRUE(runWatchdog(w));
    EXPECT_EQ(prompts, 1);
    // Первое окно не могло закрыться раньше 100 мс задержки.
    EXPECT_GE(t.elapsed(), 100);
}

// ─── Непрерывная откачка: тракт остаётся открытым после прогона ──────────────

TEST(VacuumTree, ContinuousPumpingLeavesTractOpen)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = false;  // весь блок C в тракте
    ctx.pressureVacPa      = [] { return 5.0; };
    ctx.continuousPumping  = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_FALSE(rec.allClosed());
    // Тракт собран от объёмов к насосу; К118 (атмосфера) и К179 (турбо) закрыты.
    EXPECT_TRUE(rec.state.value("S3"));    // C1 / К135
    EXPECT_TRUE(rec.state.value("S2"));    // C2 / К133
    EXPECT_TRUE(rec.state.value("S1"));    // C3 / К131
    EXPECT_TRUE(rec.state.value("R3"));    // К151 — линия E/F
    EXPECT_TRUE(rec.state.value("AR5"));   // К178 — магистраль
    EXPECT_TRUE(rec.state.value("AR6"));   // К176 — форвакуумный насос
    EXPECT_FALSE(rec.state.value("AR4"));  // К118 — сброс в атмосферу
    EXPECT_FALSE(rec.state.value("SL1"));  // К179 — турбо (интерлок с К176)
    // Насос подключается последним.
    EXPECT_TRUE(rec.sequence().endsWith(QStringLiteral("+R3 +AR5 -SL1 +AR6")));
}

TEST(VacuumTree, ContinuousPumpingOffClosesEverything)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 5.0; };
    ctx.continuousPumping = false;         // дефолт

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ContinuousPumpingSkippedAfterFailedRepeat)
{
    // Повтор упал по таймауту форвакуума → тракт открытым не остаётся.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 100.0; };   // цель не достигается
    ctx.foreVacTimeoutSec = 3;
    ctx.continuousPumping = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ContinuousPumpingRunsOnceAfterAllRepeats)
{
    // Тракт открывается один раз — после последнего повтора, а не в каждом:
    // иначе повтор N+1 стартовал бы со сброса К118 при открытом К176.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 5.0; };
    ctx.continuousPumping = true;
    ctx.totalRepeats      = 3;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence().count(QStringLiteral("+R3 +AR5 -SL1 +AR6")), 1);
}

// ─── Жёстко заданные длительности (не настраиваются из UI) ───────────────────

TEST(VacuumTree, HardcodedStepDurations)
{
    // Значения выставлены по прогону на стенде; UI их больше не переопределяет,
    // поэтому единственная защита от тихой регрессии — этот тест.
    const VacuumTreeContext def;
    EXPECT_EQ(def.perActionPauseMs, 3000);   // шаг клапана — 3 с
    EXPECT_EQ(def.reliefDwellSec, 10);       // сброс К118 — 10 с
    EXPECT_EQ(def.pumpCheckDelaySec, 30);    // «мёртвая зона» dP/dt — 30 с
    EXPECT_DOUBLE_EQ(def.targetVacPa, 40.0);
    EXPECT_DOUBLE_EQ(def.pumpMinDropPa, 0.5);
}

// ─── Цель форвакуума и её live-прогресс (11.5–11.7) ──────────────────────────

TEST(VacuumTree, ForevacTargetIsConfigurable)
{
    // Цель — не константа 10 Па: при targetVacPa = 50 давление 40 Па уже
    // засчитывается в удержание, и рецепт проходит.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.targetVacPa   = 50.0;
    ctx.pressureVacPa = [] { return 40.0; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ForevacTargetTightenedFailsSameData)
{
    // То же давление 40 Па против цели 10 Па — удержание не набирается.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.targetVacPa       = 10.0;
    ctx.pressureVacPa     = [] { return 40.0; };
    ctx.foreVacTimeoutSec = 3;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ForevacProgressReportsNodeHoldAndPressure)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.foreVacHoldSec     = 3;
    ctx.pressureVacPa      = [] { return 4.0; };

    struct Sample { VacuumNode node; double pa; int held; int elapsed; };
    QList<Sample> samples;
    QList<QPair<VacuumNode, bool>> finished;
    ctx.onForevacProgress = [&](VacuumNode n, double pa, int held, int elapsed) {
        samples.append({n, pa, held, elapsed});
    };
    ctx.onForevacDone = [&](VacuumNode n, bool ok) { finished.append({n, ok}); };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    ASSERT_FALSE(samples.isEmpty());

    // Первый отчёт — стартовая отсечка узла 11.5 (held/elapsed = 0).
    EXPECT_EQ(samples.first().node, VacuumNode::F5A1);
    EXPECT_EQ(samples.first().held, 0);
    EXPECT_EQ(samples.first().elapsed, 0);
    EXPECT_DOUBLE_EQ(samples.first().pa, 4.0);

    // Удержание накапливается до foreVacHoldSec на узле 11.5.
    int maxHeldA1 = 0;
    for (const Sample& s : samples)
        if (s.node == VacuumNode::F5A1)
            maxHeldA1 = qMax(maxHeldA1, s.held);
    EXPECT_EQ(maxHeldA1, 3);

    // Каждый из трёх этапов адресуется своим узлом и закрывается успехом.
    EXPECT_EQ(finished,
              (QList<QPair<VacuumNode, bool>>{ { VacuumNode::F5A1, true },
                                               { VacuumNode::F6BC, true },
                                               { VacuumNode::F7EF, true } }));
}

TEST(VacuumTree, ForevacTimeoutReportsFailureReason)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 100.0; };
    ctx.foreVacTimeoutSec = 3;

    QStringList reasons;
    QList<QPair<VacuumNode, bool>> finished;
    ctx.onFailure     = [&](const QString& r) { reasons << r; };
    ctx.onForevacDone = [&](VacuumNode n, bool ok) { finished.append({n, ok}); };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    ASSERT_FALSE(reasons.isEmpty());
    // Причина называет упавший этап и обе границы, по которым он судился.
    EXPECT_TRUE(reasons.first().contains(QStringLiteral("11.5")));
    EXPECT_TRUE(reasons.first().contains(QStringLiteral("10")));   // цель, Па
    EXPECT_TRUE(reasons.first().contains(QStringLiteral("3")));    // таймаут, с
    // Провалившийся этап отмечен неуспехом — UI гасит live-индикацию.
    ASSERT_FALSE(finished.isEmpty());
    EXPECT_EQ(finished.first().first, VacuumNode::F5A1);
    EXPECT_FALSE(finished.first().second);
}

TEST(VacuumTree, ForevacBlockedPumpValveReportsReason)
{
    Recorder rec;
    rec.blockedOpens.insert(VacuumValve::K176);   // Security не даёт открыть AR6
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 1.0; };

    QStringList reasons;
    ctx.onFailure = [&](const QString& r) { reasons << r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    ASSERT_FALSE(reasons.isEmpty());
    EXPECT_TRUE(reasons.first().contains(QStringLiteral("К176")));
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ForevacProgressAbsentWhenForevacDisabled)
{
    // Выключенный форвакуум не должен эмитить ни прогресс, ни причины отказа.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.foreVacuum = false;

    int progressCalls = 0, failureCalls = 0;
    ctx.onForevacProgress = [&](VacuumNode, double, int, int) { ++progressCalls; };
    ctx.onFailure         = [&](const QString&) { ++failureCalls; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(progressCalls, 0);
    EXPECT_EQ(failureCalls, 0);
}

// ─── main: QCoreApplication нужен для QTimer/QEventLoop внутри runBlocking ────

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// ═════════════════════════════════════════════════════════════════════════════
// Раздел 12.2 — переход на турбомолекулярный насос (REQ-077…084)
// ═════════════════════════════════════════════════════════════════════════════
//
// Топология: AR6/К176 (форвакуум) и SL1/К179 (турбо) на одной магистрали;
// одновременно открытыми они быть не должны никогда (REQ-008/084).

namespace {

// Контекст с включённым турбо-этапом: форвакуум отрабатывает мгновенно,
// ДВ302 валиден и глубоко под порогом возврата.
VacuumTreeContext makeTurboCtx(Recorder& rec)
{
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa   = [] { return 5.0; };    // ≤ targetVacPa и ≤ порога гейта
    ctx.pressureTurboPa = [] { return 1.0; };    // валиден, ниже порога возврата
    ctx.turboTract            = true;
    ctx.turboSwitchPressurePa = 10.0;
    ctx.turboSwitchHoldSec    = 2;
    ctx.turboReturnPressurePa = 30.0;
    ctx.turboTimeoutSec       = 50;
    ctx.overrangeWaitSec      = 2;
    ctx.overrangeWaitSec2     = 2;
    // Readback по умолчанию подтверждает то, что записано в моке клапанов.
    ctx.confirmValve = [&rec](bool expectedOpen, const QString& name) {
        return rec.state.value(name, false) == expectedOpen;
    };
    return ctx;
}

// Индексы операций над клапаном в записанной последовательности.
QList<int> opIndexes(const Recorder& rec, bool open, const QString& name)
{
    QList<int> out;
    for (int i = 0; i < rec.ops.size(); ++i)
        if (rec.ops.at(i).open == open && rec.ops.at(i).name == name)
            out << i;
    return out;
}

// Инвариант REQ-008: К176 и К179 никогда не открыты одновременно.
bool pumpsNeverBothOpen(const Recorder& rec)
{
    bool k176 = false, k179 = false;
    for (const ValveOp& op : rec.ops) {
        if (op.name == VacuumValve::K176) k176 = op.open;
        if (op.name == VacuumValve::K179) k179 = op.open;
        if (k176 && k179)
            return false;
    }
    return true;
}

} // namespace

TEST(VacuumTree, TurboSwitchHappensWhenAllConditionsMet)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);

    QList<bool> switches;
    ctx.onTurboSwitched = [&switches](bool toTurbo) { switches.append(toTurbo); };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));

    // Переход состоялся ровно один раз и именно на турбо.
    EXPECT_EQ(switches, QList<bool>{ true });

    // К179 открыт ПОСЛЕ последнего закрытия К176 — порядок из REQ-082.
    const QList<int> opensK179  = opIndexes(rec, true,  VacuumValve::K179);
    const QList<int> closesK176 = opIndexes(rec, false, VacuumValve::K176);
    ASSERT_FALSE(opensK179.isEmpty());
    ASSERT_FALSE(closesK176.isEmpty());
    EXPECT_LT(closesK176.last(), opensK179.last());
}

TEST(VacuumTree, TurboHoldResetsOnConditionBreak)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.turboSwitchHoldSec = 3;

    // Давление «дышит» между 5 и 20 Па: обе точки ≤ targetVacPa (40), поэтому
    // форвакуумные этапы 11.5–11.7 проходят, но 20 Па > turboSwitchPressurePa
    // (10), поэтому непрерывного удержания гейта не набирается никогда и он
    // обязан упасть по таймауту. Проверяется именно сброс счётчика удержания.
    auto tick = std::make_shared<int>(0);
    ctx.pressureVacPa = [tick] {
        return (((*tick)++ % 2) == 0) ? 5.0 : 20.0;
    };
    ctx.turboTimeoutSec = 12;

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    // Клапан турбонасоса не открывался ни разу.
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(reason.contains(QStringLiteral("гейт")));
}

TEST(VacuumTree, Dv301OverRangeSingleWaitsThenContinues)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.overrangeWaitSec = 3;

    // Первые тики — over range, затем показание возвращается в диапазон.
    auto calls = std::make_shared<int>(0);
    ctx.pressureVacPa = [calls] {
        const int n = (*calls)++;
        return n < 2 ? Reading(9e5, Quality::OverRange) : Reading(5.0);
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    // Переход всё равно состоялся: over range был временным.
    EXPECT_FALSE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
}

TEST(VacuumTree, Dv301OverRangeDoubleIsCritical)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.overrangeWaitSec = 2;
    ctx.pressureVacPa = [] { return Reading(9e5, Quality::OverRange); };

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    // Безопасное действие для ДВ301 — закрыть К176; К179 не открывается.
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(reason.contains(QStringLiteral("ДВ301")));
}

TEST(VacuumTree, Dv302OverRangeDoubleAwaitsOperatorAndKeepsK179Closed)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.overrangeWaitSec2 = 2;
    ctx.pressureTurboPa = [] { return Reading(9e5, Quality::OverRange); };

    OperatorBus bus;
    ctx.operatorBus = &bus;
    int prompts = 0;
    int seenCode = 0;
    QObject::connect(&bus, &OperatorBus::decisionRequired, &bus,
                     [&](int code, QString) {
                         ++prompts;
                         seenCode = code;
                         bus.respond(OperatorBus::Stop);
                     });

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_EQ(prompts, 1);
    EXPECT_EQ(seenCode, int(OperatorBus::Dv302OverRange));
    // Ключевое: турбоклапан не открыт ни при каком решении оператора.
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, ReadbackFailureBlocksTurboValve)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    // Команда на закрытие К176 проходит, но readback её НЕ подтверждает.
    ctx.confirmValve = [](bool, const QString&) { return false; };

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    // Единственное, что действительно важно: К179 не открылся.
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(reason.contains(QStringLiteral("не подтверждено")));
}

TEST(VacuumTree, TurboMissingReadbackSeamBlocksTurboValve)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.confirmValve = nullptr;      // шва нет ⇒ REQ-082 непроверяем

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, TurboFallbackOnReturnPressureReopensK176)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);

    // ДВ302 валиден на гейте, но после переключения уходит выше порога возврата.
    auto switched = std::make_shared<bool>(false);
    ctx.onTurboSwitched = [switched](bool toTurbo) { if (toTurbo) *switched = true; };
    ctx.pressureTurboPa = [switched] {
        return *switched ? Reading(40.0) : Reading(1.0);
    };

    QList<bool> switches;
    ctx.onTurboSwitched = [switched, &switches](bool toTurbo) {
        if (toTurbo) *switched = true;
        switches.append(toTurbo);
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    // Переход, затем откат — обе стороны обязаны попасть в журнал (REQ-083).
    EXPECT_EQ(switches, (QList<bool>{ true, false }));
    // К179 закрыт раньше, чем К176 снова открыт.
    const QList<int> closesK179 = opIndexes(rec, false, VacuumValve::K179);
    const QList<int> opensK176  = opIndexes(rec, true,  VacuumValve::K176);
    ASSERT_FALSE(closesK179.isEmpty());
    ASSERT_FALSE(opensK176.isEmpty());
    EXPECT_LT(closesK179.last(), opensK176.last());
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, TurboTractExcludedSkipsAllTurboNodes)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.turboTract = false;

    QMap<int, NodeState> states;
    ctx.onNode = [&states](VacuumNode n, NodeState s) { states[int(n)] = s; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(states.value(int(VacuumNode::TurboGate)), NodeState::Skipped);
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, TurboSkippedWithoutDv302)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.pressureTurboPa = nullptr;   // датчика нет ⇒ У3 недостижимо

    QMap<int, NodeState> states;
    ctx.onNode = [&states](VacuumNode n, NodeState s) { states[int(n)] = s; };

    // Пропуск, а не таймаут: «этап не выполнялся» ≠ «этап не смог».
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(states.value(int(VacuumNode::TurboGate)), NodeState::Skipped);
    EXPECT_TRUE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
}

TEST(VacuumTree, CancelDuringTurboGateClosesBothPumps)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.pressureVacPa   = [] { return 50.0; };   // гейт не набирается — висим
    ctx.turboTimeoutSec = 10000;

    QTaskTree tree(buildVacuumRecipe(ctx));
    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&loop](DoneWith) { loop.quit(); });
    tree.start();
    QTimer::singleShot(60, &tree, [&tree] { tree.cancel(); });
    loop.exec();

    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
}

TEST(VacuumTree, SecondTractOpensTurboValveOnlyWithK176Closed)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;
    // Readback подтверждает реальное состояние мока.
    ctx.confirmValve = [&rec](bool expectedOpen, const QString& name) {
        return rec.state.value(name, false) == expectedOpen;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    // Ф3 по-прежнему открывает второй тракт: К192, затем К179.
    EXPECT_FALSE(opIndexes(rec, true, VacuumValve::K179).isEmpty());
}
