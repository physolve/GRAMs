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

#include "recipes/VacuumTaskTree.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <memory>
#include <string>
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
        // Выключателя форвакуума больше нет: этапы 11.5–11.7 обязательны по ТЗ.
        // Чтобы тесты Ф1–Ф3 оставались быстрыми и детерминированными, ДВ301
        // мокается уже на цели — удержание набирается с первого тика.
        ctx.pressureVacPa    = [] { return 0.0; };
        ctx.foreVacHoldSec   = 1;
        ctx.foreVacTimeoutSec = 10;
        ctx.k178PulseMs      = 1;
        ctx.pumpRateCheck    = false;  // dP/dt-watchdog — отдельные тесты
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

    // std::string, а не QString: gtest печатает QString как массив 2-байтовых
    // объектов, и упавшая проверка последовательности становится нечитаемой.
    std::string seq() const { return sequence().toStdString(); }

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

// Эталон прогона при всех включённых ветках (изменения состояний клапанов).
// Ф1–Ф3 — это s1–s24 из GramQt; этапы 11.5–11.7 идут следом и теперь ВСЕГДА
// выполняются: выключателя форвакуума больше нет ни в рецепте, ни в UI
// (REQ-047/050/053/056/077). Поэтому эталон описывает весь прогон, а не только
// легаси-часть — иначе он молча перестал бы покрывать половину режима.
const std::string kFullSequence =
    "+AR4 -AR4 "                             // Ф1: триплет К118
    "+S3 +AR4 -AR4 -S3 "                     // Ф2: RK300 + триплет + закрытие
    "+S1 +S2 +AR4 -AR4 -S2 -S1 "             // Ф2: RK10/RK50 + триплет + закрытия
    "+SL1 +SL2 -SL1 -SL2 "                   // Ф3: второй тракт (К192, затем К179)
    "+AR4 -AR4 +AR5 -AR5 +AR6 -AR6 "         // 11.5 A1: сброс, импульс К178, К176
    "+S3 +S2 +S1 +AR5 -AR5 +AR6 -AR6 -S3 -S2 -S1 "  // 11.6 B/C: объёмы C под откачку
    "+R3 +AR5 -AR5 +AR6 -AR6 -R3";           // 11.7 E/F: К151, импульс К178, К176

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
    EXPECT_EQ(rec.seq(), kFullSequence);
    EXPECT_TRUE(rec.allClosed());
}

TEST(VacuumTree, AllSkippedAndPressureLowProducesNoValveOps)
{
    Recorder rec;
    rec.pressure = [] { return 1.0; };  // <= DB_SBR_LIM: стражи ДВ_СБР пропускают
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = true;
    ctx.secondTract = false;

    // Легаси-часть Ф1–Ф3 не выполняет ни одной операции, но форвакуумные
    // этапы обязательны и идут всегда — «пусто» относится только к Ф1–Ф3.
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.seq(),
              std::string("+AR5 -AR5 +AR6 -AR6 "
                          "+AR5 -AR5 +AR6 -AR6 "
                          "+R3 +AR5 -AR5 +AR6 -AR6 -R3"));
}

TEST(VacuumTree, SkipFlagsStillRunUnconditionalTriplets)
{
    // Пропуск всего блока C не отменяет безусловные триплеты s2–s4 и s15–s17.
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = true;
    ctx.secondTract = false;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    // Форвакуум выполняется всегда: за легаси-триплетами идут этапы 11.5–11.7
    // (блок C пропущен флагами, поэтому в 11.6 остаются только импульс и насос).
    EXPECT_EQ(rec.seq(),
              std::string("+AR4 -AR4 +AR4 -AR4 "
                          "+AR4 -AR4 +AR5 -AR5 +AR6 -AR6 "
                          "+AR5 -AR5 +AR6 -AR6 "
                          "+R3 +AR5 -AR5 +AR6 -AR6 -R3"));
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
    EXPECT_EQ(calls, 4);  // Ф1, Ф2-S3, Ф2 s15–s17 и сброс перед откачкой A1
    EXPECT_EQ(rec.seq(),
              std::string("+AR4 -AR4 +S3 -S3 +S1 +S2 -S2 -S1 "
                          "+AR5 -AR5 +AR6 -AR6 "                 // 11.5 (сброс пропущен)
                          "+S3 +S2 +S1 +AR5 -AR5 +AR6 -AR6 -S3 -S2 -S1 "
                          "+R3 +AR5 -AR5 +AR6 -AR6 -R3"));
}

TEST(VacuumTree, SkipRk300OnlyOmitsItsSubgroup)
{
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.skipRK300 = true;
    ctx.secondTract = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.seq(),
              std::string("+AR4 -AR4 "
                          "+S1 +S2 +AR4 -AR4 -S2 -S1 "
                          "+SL1 +SL2 -SL1 -SL2 "
                          "+AR4 -AR4 +AR5 -AR5 +AR6 -AR6 "
                          "+S2 +S1 +AR5 -AR5 +AR6 -AR6 -S2 -S1 "   // C1 исключён и здесь
                          "+R3 +AR5 -AR5 +AR6 -AR6 -R3"));
}

TEST(VacuumTree, ProgressTicksMatchDwells)
{
    // 5 выдержек по reliefDwellSec=5 тиков: elapsed накапливается 1..25.
    Recorder rec;
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;
    QList<int> progress;
    ctx.onProgress = [&progress](int elapsed, int /*repeat*/) {
        progress << elapsed;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    // 5 выдержек по 5 тиков: Ф1, Ф2-S3, Ф2 s15–s17, Ф3 и сброс перед откачкой A1.
    ASSERT_EQ(progress.size(), 25);
    for (int i = 0; i < progress.size(); ++i)
        EXPECT_EQ(progress.at(i), i + 1);
}

// ═════════════════════════════════════════════════════════════════════════════
// Критерий B: отмена в каждой точке «клапан открыт, идёт выдержка»
// ═════════════════════════════════════════════════════════════════════════════
// Точки (all-on, elapsed по выдержкам): Ф1 К118 = 1–5; Ф2 S3-триплет = 6–10;
// Ф2 s15–s17 (S1+S2 открыты) = 11–15; Ф3 (SL1+SL2) = 16–20.

TEST(VacuumTree, CancelDuringPhase1ReliefClosesK118)
{
    CancelRun run = runWithCancelAt(3);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    EXPECT_EQ(run.rec.seq(), std::string("+AR4 -AR4"));
}

TEST(VacuumTree, CancelDuringS3TripletClosesInsideOut)
{
    CancelRun run = runWithCancelAt(8);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    // Изнутри-наружу: сначала К118 (вложенный триплет), затем S3 (объемлющая группа)
    EXPECT_EQ(run.rec.seq(),
              std::string("+AR4 -AR4 +S3 +AR4 -AR4 -S3"));
}

TEST(VacuumTree, CancelDuringBlockCTripletClosesAllOpened)
{
    CancelRun run = runWithCancelAt(13);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    // К118 (триплет) → S2 → S1 (порядок закрытий CSV s18–s19)
    EXPECT_EQ(run.rec.seq(),
              std::string("+AR4 -AR4 +S3 +AR4 -AR4 -S3 "
                          "+S1 +S2 +AR4 -AR4 -S2 -S1"));
}

TEST(VacuumTree, CancelDuringSecondTractClosesBothValves)
{
    CancelRun run = runWithCancelAt(18);
    EXPECT_EQ(run.result, DoneWith::Cancel);
    EXPECT_TRUE(run.rec.allClosed());
    // Отмена в Ф3 добирает закрытия SL1/SL2 и на этом заканчивает прогон:
    // до форвакуумных этапов дело не доходит.
    EXPECT_EQ(run.rec.seq(),
              std::string("+AR4 -AR4 "
                          "+S3 +AR4 -AR4 -S3 "
                          "+S1 +S2 +AR4 -AR4 -S2 -S1 "
                          "+SL1 +SL2 -SL1 -SL2"));
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
        EXPECT_FALSE(s.k179TractOpen);
        EXPECT_FALSE(s.k192Open);
        EXPECT_EQ(s.elapsedSec, 0);
        EXPECT_EQ(s.reliefCount, 0);
    };

    QTaskTree tree1(recipe);
    tree1.onStorageSetup(st, assertFresh);
    EXPECT_EQ(tree1.runBlocking(), DoneWith::Success);
    EXPECT_EQ(rec.seq(), kFullSequence);

    rec.ops.clear();

    QTaskTree tree2(recipe);
    tree2.onStorageSetup(st, assertFresh);
    EXPECT_EQ(tree2.runBlocking(), DoneWith::Success);
    EXPECT_EQ(rec.seq(), kFullSequence);

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
    EXPECT_EQ(rec.seq(), kFullSequence + " " + kFullSequence);
    EXPECT_EQ(repeats, (QList<int>{0, 1}));
    EXPECT_EQ(maxElapsed, 25);  // elapsedSec пер-повторный, не накапливается между повторами
}

// ═════════════════════════════════════════════════════════════════════════════
// Ошибка открытия (Security-блокировка), пауза, condition-фаза, итоги
// ═════════════════════════════════════════════════════════════════════════════

TEST(VacuumTree, BlockedSecondTractValveClosesAlreadyOpened)
{
    Recorder rec;
    rec.blockedOpens.insert(QStringLiteral("SL2"));
    VacuumTreeContext ctx = rec.makeCtx();
    ctx.secondTract = true;

    QList<DoneWith> repeatResults;
    ctx.onRepeatDone = [&repeatResults](DoneWith w, int /*repeat*/) {
        repeatResults << w;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.allClosed());
    // SL1 (К192) успел открыться, SL2 (К179) заблокирован → откат: закрыть SL1
    EXPECT_TRUE(rec.sequence().endsWith(QStringLiteral("+SL1 -SL1")))
        << rec.sequence().toStdString();
    EXPECT_EQ(repeatResults, (QList<DoneWith>{DoneWith::Error}));
}

TEST(VacuumTree, RunFinishedReportsCounts)
{
    Recorder rec;
    rec.blockedOpens.insert(QStringLiteral("SL2"));
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
    EXPECT_EQ(rec.seq(), kFullSequence);
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
    EXPECT_EQ(rec.seq(),
              std::string("+AR5 -AR5 +AR6 -AR6 "     // 11.5 A1
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
    EXPECT_EQ(rec.seq(), std::string("+AR5 -AR5 +AR6 -AR6"));
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

// ─── dP/dt проверяется только выше цели этапа ────────────────────────────────
//
// Цель 50 Па, а в тракте уже 30 Па: откачивать нечего, скорость откачки по
// физике близка к нулю. Вердикт «мала» здесь означал бы ложный отказ.

TEST(VacuumTree, PumpWatchdogSkippedBelowTarget)
{
    PumpRateWatchdog w;
    w.pressurePa    = [] { return 30.0; };   // ниже цели и не меняется
    w.intervalMs    = 20;
    w.startDelaySec = 50;                    // 1 с: пропуск обязан быть мгновенным
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    w.targetPa      = 50.0;

    QElapsedTimer t;
    t.start();
    EXPECT_TRUE(runWatchdog(w));
    EXPECT_LT(t.elapsed(), 100);             // «мёртвую зону» не выжидали
}

TEST(VacuumTree, PumpWatchdogChecksAboveTarget)
{
    // То же давление, но цель ниже его — проверка обязана состояться и
    // закончиться отказом: давление стоит.
    PumpRateWatchdog w;
    w.pressurePa    = [] { return 30.0; };
    w.intervalMs    = 20;
    w.startDelaySec = 0;
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    w.targetPa      = 10.0;
    EXPECT_FALSE(runWatchdog(w));
}

TEST(VacuumTree, PumpWatchdogIgnoresInvalidReadingBelowTarget)
{
    // Мёртвый датчик, отдающий «0 Па», формально ниже цели. Снимать по нему
    // проверку откачки нельзя — иначе отказ датчика маскирует отказ насоса.
    PumpRateWatchdog w;
    w.pressurePa    = [] { return Reading(0.0, Quality::NoResponse); };
    w.intervalMs    = 20;
    w.startDelaySec = 0;
    w.windowSec     = 1;
    w.minDropPa     = 1.0;
    w.targetPa      = 50.0;
    EXPECT_FALSE(runWatchdog(w));
}

TEST(VacuumTree, PumpWatchdogReachingTargetInsideWindowPasses)
{
    // Давление проваливается ниже цели посреди окна — окно досчитывать незачем,
    // откачка очевидно шла.
    auto p = std::make_shared<double>(100.0);
    PumpRateWatchdog w;
    w.pressurePa    = [p] { double v = *p; *p -= 10.0; return v; };
    w.intervalMs    = 10;
    w.startDelaySec = 0;
    w.windowSec     = 100;                   // окно заведомо не успевает закрыться
    w.minDropPa     = 1.0;
    w.targetPa      = 50.0;
    EXPECT_TRUE(runWatchdog(w));
}

TEST(VacuumTree, ForevacBelowTargetPassesWatchdogInRecipe)
{
    // Рецепт целиком: тракт уже откачан ниже цели, dP/dt-watchdog включён и
    // оператора нет. До правки прогон падал бы в Error на первом же этапе.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pumpRateCheck      = true;
    ctx.pumpCheckWindowSec = 2;
    ctx.pumpMinDropPa      = 1.0;
    ctx.targetVacPa        = 50.0;
    ctx.foreVacHoldSec     = 1;
    ctx.pressureVacPa      = [] { return 30.0; };   // ниже цели, не меняется

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

// ─── Непрерывная откачка: тракт остаётся открытым после прогона ──────────────

// Хвост оставляет открытым рабочий тракт на ТУРБОНАСОСЕ: камера E/F (К151),
// эталон B (на магистрали, своего клапана нет), бочка (К178) и К179; К176
// закрыт (интерлок REQ-008), баллоны C и К192 не открываются вовсе.
const QString kContinuousTail = QStringLiteral("+R3 +AR5 -AR6 +SL2");

TEST(VacuumTree, ContinuousPumpingLeavesTractOpen)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.skipRK10 = ctx.skipRK50 = ctx.skipRK300 = false;  // блок C откачивается…
    ctx.pressureVacPa      = [] { return 5.0; };
    ctx.confirmValve       = [&rec](bool expectedOpen, const QString& name) {
        return rec.state.value(name, false) == expectedOpen;
    };
    ctx.continuousPumping  = true;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_FALSE(rec.allClosed());
    EXPECT_TRUE(rec.state.value("R3"));    // К151 — камера F + линия E
    EXPECT_TRUE(rec.state.value("AR5"));   // К178 — бочка / магистраль (эталон B)
    EXPECT_TRUE(rec.state.value("SL2"));   // К179 — турбомолекулярный насос
    EXPECT_FALSE(rec.state.value("AR6"));  // К176 — форвакуум закрыт (REQ-008)
    EXPECT_FALSE(rec.state.value("AR4"));  // К118 — сброс в атмосферу
    EXPECT_FALSE(rec.state.value("SL1"));  // К192 — выход второго тракта
    // …но в открытом тракте баллоны C не остаются: качается рабочий тракт.
    EXPECT_FALSE(rec.state.value("S3"));
    EXPECT_FALSE(rec.state.value("S2"));
    EXPECT_FALSE(rec.state.value("S1"));
    // Насос подключается последним, к уже собранному тракту.
    EXPECT_TRUE(rec.sequence().endsWith(kContinuousTail)) << rec.sequence().toStdString();
}

// Турбоклапан хвоста проходит ту же проверку REQ-082, что и переход 12.2:
// без подтверждения закрытия К176 он не открывается, тракт не остаётся открытым.
TEST(VacuumTree, ContinuousPumpingRequiresValveReadback)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 5.0; };
    ctx.continuousPumping = true;
    ctx.confirmValve      = nullptr;      // readback-шва нет

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Error);
    EXPECT_FALSE(rec.state.value("SL2"));
    EXPECT_FALSE(reason.isEmpty());
}

// Опция читается в момент выполнения хвоста, а не на старте: оператор вправе
// передумать, пока режим идёт.
TEST(VacuumTree, ContinuousPumpingReadsLiveFlagAtTailTime)
{
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa     = [] { return 5.0; };
    ctx.confirmValve      = [&rec](bool expectedOpen, const QString& name) {
        return rec.state.value(name, false) == expectedOpen;
    };
    ctx.continuousPumping     = false;             // снимок со «Старта» — выключено
    ctx.continuousPumpingLive = [] { return true; };  // оператор включил на ходу

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.sequence().endsWith(kContinuousTail)) << rec.sequence().toStdString();

    // И обратно: включено на старте, выключено на ходу — тракт закрывается.
    Recorder rec2;
    VacuumTreeContext ctx2 = makeForevacCtx(rec2);
    ctx2.pressureVacPa         = [] { return 5.0; };
    ctx2.continuousPumping     = true;
    ctx2.continuousPumpingLive = [] { return false; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx2)), DoneWith::Success);
    EXPECT_TRUE(rec2.allClosed());
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
    ctx.confirmValve      = [&rec](bool expectedOpen, const QString& name) {
        return rec.state.value(name, false) == expectedOpen;
    };
    ctx.continuousPumping = true;
    ctx.totalRepeats      = 3;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(rec.sequence().count(kContinuousTail), 1);
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
    EXPECT_DOUBLE_EQ(def.pumpMinDropPa, 0.25);
    // Гейт турбоперехода и порог отката — тоже не из UI (гейты безопасности).
    // Значения уточнены по прогону на стенде: 10 Па оказались недостижимы.
    EXPECT_DOUBLE_EQ(def.turboSwitchPressurePa, 50.0);
    EXPECT_DOUBLE_EQ(def.turboReturnPressurePa, 150.0);
    // Гистерезис обязан сохраняться при любом изменении порогов.
    EXPECT_GT(def.turboReturnPressurePa, def.turboSwitchPressurePa);
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

TEST(VacuumTree, ForevacAlwaysRunsAndCannotBeDisabled)
{
    // Выключателя форвакуума нет ни в контексте рецепта, ни в VacuumOptions,
    // ни в UI: этапы 11.5–11.7 обязательны и предшествуют любому турбо-этапу
    // (REQ-047/050/053/056/077). Раньше здесь проверялось обратное — что
    // выключенный форвакуум ничего не эмитит; теперь проверяется, что все три
    // этапа отрабатывают всегда, каким бы ни был остальной набор опций.
    Recorder rec;
    VacuumTreeContext ctx = makeForevacCtx(rec);
    ctx.pressureVacPa = [] { return 5.0; };

    QSet<int> progressedNodes;
    int failureCalls = 0;
    ctx.onForevacProgress = [&](VacuumNode n, double, int, int) {
        progressedNodes.insert(int(n));
    };
    ctx.onFailure = [&](const QString&) { ++failureCalls; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(failureCalls, 0);
    EXPECT_TRUE(progressedNodes.contains(int(VacuumNode::F5A1)));
    EXPECT_TRUE(progressedNodes.contains(int(VacuumNode::F6BC)));
    EXPECT_TRUE(progressedNodes.contains(int(VacuumNode::F7EF)));
    EXPECT_TRUE(rec.allClosed());
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
// Топология: AR6/К176 (форвакуум) и SL2/К179 (турбо) на одной магистрали;
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
    ctx.turboSwitchPressurePa = 50.0;
    ctx.turboSwitchHoldSec    = 2;
    ctx.turboReturnPressurePa = 150.0;
    ctx.turboTimeoutSec       = 50;
    ctx.overrangeWaitSec      = 2;
    ctx.overrangeWaitSec2     = 2;
    // PumpDownProcedure больше не висит отдельным шагом после 11.7: по REQ-077
    // она принадлежит общей откачке (11.7б) и финальной откачке камеры (11.10).
    // Чтобы тесты 12.2 продолжали проверять ровно 12.2, здесь конфигурируется
    // ТОЛЬКО тракт общей откачки; 11.8 и 11.9-11.10 остаются несконфигурированными
    // и пропускаются с причиной, поэтому переход на турбо в прогоне ровно один.
    ctx.generalPumpingValves = { VacuumValve::K171, VacuumValve::K173, VacuumValve::K151 };
    ctx.testEvacTimeSec      = 1;
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

    // Гейт (50 Па) шире штатной цели форвакуума (40 Па), поэтому цель
    // здесь ослаблена до 100 Па — иначе точку выше гейта не задать, не сломав
    // этапы 11.5–11.7. Давление «дышит» между 5 и 60 Па: обе точки ≤ targetVacPa,
    // поэтому форвакуум проходит, но 60 Па > turboSwitchPressurePa (50), и
    // непрерывного удержания гейта не набирается никогда — он обязан упасть по
    // таймауту. Проверяется именно сброс счётчика удержания.
    ctx.targetVacPa = 100.0;
    auto tick = std::make_shared<int>(0);
    ctx.pressureVacPa = [tick] {
        return (((*tick)++ % 2) == 0) ? 5.0 : 60.0;
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
        return *switched ? Reading(200.0) : Reading(1.0);
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

// ═════════════════════════════════════════════════════════════════════════════
// Этапы 11.7б / 11.8 / 11.9–11.11 — тракт до камеры (REQ-055…073)
// ═════════════════════════════════════════════════════════════════════════════
//
// Инвариант REQ-008/084 (К176 и К179 никогда не открыты одновременно)
// проверяется В КАЖДОМ тесте этих этапов — pumpsNeverBothOpen(rec). Открытый
// одновременно с форвакуумным турбоклапан означает откачку форвакуумного
// тракта турбонасосом, то есть его порчу, и никакая другая проверка этого
// не поймает.

namespace {

// Полная конфигурация тракта: 11.7б, 11.8 и 11.9–11.11 включены. Значения
// намеренно повторяют profile/GRAMsPfp.json → vacuumTract, чтобы тест ломался
// при расхождении кода с конфигурацией стенда, а не только с самим собой.
VacuumTreeContext makeTractCtx(Recorder& rec)
{
    VacuumTreeContext ctx = makeTurboCtx(rec);
    ctx.generalPumpingValves = { VacuumValve::K171, VacuumValve::K173, VacuumValve::K151 };
    ctx.leakTestValves       = { VacuumValve::K171, VacuumValve::K173 };
    ctx.finalPumpingValves   = { VacuumValve::K173, VacuumValve::K151,
                                 VacuumValve::K171, VacuumValve::K178 };
    ctx.testEvacTimeSec      = 1;
    ctx.leakTestDurationSec  = 2;
    ctx.evacTimeSec          = 1;
    // Цель ниже гейта перехода (50 Па) ⇒ турбо-область, контроль по ДВ302.
    ctx.targetVacuumPa       = 0.5;
    return ctx;
}

// Клапаны газовых баллонов: в режиме «Вакуум» обязаны оставаться закрытыми
// (REQ-026). В рецепте они не упоминаются вовсе, и тест это фиксирует —
// иначе первая же правка набора клапанов в профиле сможет их туда внести.
bool gasValvesNeverTouched(const Recorder& rec)
{
    static const QStringList kGas { QStringLiteral("AR1"),    // К104
                                    QStringLiteral("AR2"),    // К109
                                    QStringLiteral("AR3") };  // К114
    for (const ValveOp& op : rec.ops)
        if (kGas.contains(op.name))
            return false;
    return true;
}

// Был ли клапан открыт хоть раз за прогон.
bool everOpened(const Recorder& rec, const QString& name)
{
    for (const ValveOp& op : rec.ops)
        if (op.open && op.name == name)
            return true;
    return false;
}

} // namespace

// ── 11.7б: общая откачка (REQ-055–058) ───────────────────────────────────────

TEST(VacuumTree, GeneralPumpingOpensTractBeforePumpAndClosesAfter)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.leakTestValves.clear();      // 11.8 и 11.9–11.10 — отдельные тесты
    ctx.finalPumpingValves.clear();

    QStringList skipped;
    ctx.onStageSkipped = [&skipped](VacuumNode, const QString& r) { skipped << r; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(gasValvesNeverTouched(rec));

    // REQ-055: К171 и К173 открываются ДО форвакуумного тракта. Порядок здесь
    // и есть требование — переставить их за К178/К176 нельзя.
    const QList<int> openK171 = opIndexes(rec, true, VacuumValve::K171);
    const QList<int> openK173 = opIndexes(rec, true, VacuumValve::K173);
    const QList<int> openK178 = opIndexes(rec, true, VacuumValve::K178);
    const QList<int> openK176 = opIndexes(rec, true, VacuumValve::K176);
    ASSERT_FALSE(openK171.isEmpty());
    ASSERT_FALSE(openK173.isEmpty());
    ASSERT_FALSE(openK178.isEmpty());
    ASSERT_FALSE(openK176.isEmpty());
    EXPECT_LT(openK171.last(), openK178.last());
    EXPECT_LT(openK173.last(), openK178.last());
    EXPECT_LT(openK178.last(), openK176.last());

    // REQ-056/057: переход на турбонасос состоялся именно на этом этапе.
    EXPECT_TRUE(everOpened(rec, VacuumValve::K179));

    // К192 (SL1) — выход второго тракта в атмосферу. В тракте до камеры его
    // быть не должно ни при каких условиях.
    EXPECT_FALSE(everOpened(rec, VacuumValve::K192));

    // Два непроведённых этапа обязаны объявиться причиной, а не тишиной.
    EXPECT_EQ(skipped.size(), 3);   // 11.8 + 11.9 + 11.10
}

TEST(VacuumTree, GeneralPumpingIncludesApplicableBlockCValves)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.leakTestValves.clear();
    ctx.finalPumpingValves.clear();
    ctx.skipRK300 = false;           // C1 включён
    ctx.skipRK50  = true;            // C2 исключён
    ctx.skipRK10  = false;           // C3 включён

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));

    // Открытие C на этапе 11.7б идёт ПОСЛЕ К171/К173 и ДО магистрали К178.
    const QList<int> openC1  = opIndexes(rec, true, VacuumValve::K135);
    const QList<int> openC3  = opIndexes(rec, true, VacuumValve::K131);
    const QList<int> openK171 = opIndexes(rec, true, VacuumValve::K171);
    const QList<int> openK178 = opIndexes(rec, true, VacuumValve::K178);
    ASSERT_FALSE(openC1.isEmpty());
    ASSERT_FALSE(openC3.isEmpty());
    EXPECT_LT(openK171.last(), openC1.last());
    EXPECT_LT(openC1.last(),  openK178.last());
    EXPECT_LT(openC3.last(),  openK178.last());
}

TEST(VacuumTree, GeneralPumpingSkippedWithReasonWhenNotConfigured)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();
    ctx.finalPumpingValves.clear();

    QList<VacuumNode> skippedNodes;
    QStringList       reasons;
    ctx.onStageSkipped = [&](VacuumNode n, const QString& r) {
        skippedNodes << n;
        reasons << r;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    // Пропуск объявлен и назван — «этап не выполнялся» не должно выглядеть
    // как «этап пройден».
    EXPECT_TRUE(skippedNodes.contains(VacuumNode::GeneralPumping));
    EXPECT_TRUE(reasons.join(QStringLiteral(" ")).contains(QStringLiteral("generalPumping")));
    // Тракт не собирался — К171/К173 не трогались.
    EXPECT_FALSE(everOpened(rec, VacuumValve::K171));
    EXPECT_FALSE(everOpened(rec, VacuumValve::K173));
}

TEST(VacuumTree, GeneralPumpingBlockedValveClosesWhatWasOpened)
{
    Recorder rec;
    // Блокируем К173 — второй клапан набора. К151 для этого не годится: он же
    // участвует в 11.7 (E/F), и прогон свалился бы раньше, не дойдя до 11.7б.
    rec.blockedOpens << VacuumValve::K173;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.leakTestValves.clear();
    ctx.finalPumpingValves.clear();

    QStringList failures;
    ctx.onFailure = [&failures](const QString& r) { failures << r; };

    runBlocking(buildVacuumRecipe(ctx));
    // Главное — не исход дерева, а состояние установки: уже открытые К171/К173
    // обязаны закрыться, насос не подключаться.
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_FALSE(everOpened(rec, VacuumValve::K179));
    EXPECT_FALSE(failures.isEmpty());
}

TEST(VacuumTree, CancelDuringGeneralPumpingClosesTractAndPumps)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.leakTestValves.clear();
    ctx.finalPumpingValves.clear();
    ctx.turboSwitchHoldSec = 50;               // застрять на наборе гейта
    ctx.turboTimeoutSec    = 500;

    QTaskTree tree;
    ctx.onNode = [&tree](VacuumNode node, NodeState state) {
        if (node == VacuumNode::TurboGate && state == NodeState::Running)
            QTimer::singleShot(0, &tree, [&tree] { tree.cancel(); });
    };
    tree.setRecipe(buildVacuumRecipe(ctx));

    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&loop](DoneWith) { loop.quit(); });
    tree.start();
    loop.exec();

    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
}

// ── 11.8: проверка герметичности (REQ-059–063) ───────────────────────────────

TEST(VacuumTree, LeakTestPassesWhenRiseWithinThreshold)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.finalPumpingValves.clear();

    // Первое чтение — начальное давление, второе — после выдержки. Прирост
    // 0.002 бар меньше порога 0.01 ⇒ герметичность подтверждена.
    auto reads = std::make_shared<int>(0);
    LeakChannel ch;
    ch.name    = QStringLiteral("DD312");
    ch.maxRise = 0.01;
    ch.read    = [reads] {
        *reads += 1;
        return Reading(*reads > 1 ? 0.102 : 0.100);
    };
    ctx.leakChannels << ch;

    QStringList warnings;
    ctx.onWarning = [&warnings](const QString& w) { warnings << w; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(warnings.isEmpty());
    // REQ-059: участок открывался, и это были именно К171/К173.
    EXPECT_TRUE(everOpened(rec, VacuumValve::K171));
    EXPECT_TRUE(everOpened(rec, VacuumValve::K173));
}

TEST(VacuumTree, LeakTestWarnsButContinuesOnLeak)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.finalPumpingValves.clear();

    // Показание растёт на каждом чтении: за выдержку прирост заведомо больше
    // порога, значит LEAK_DETECTED.
    auto value = std::make_shared<double>(0.100);
    LeakChannel ch;
    ch.name    = QStringLiteral("DD312");
    ch.maxRise = 0.001;
    ch.read    = [value] { const double v = *value; *value += 0.05; return Reading(v); };
    ctx.leakChannels << ch;

    QStringList warnings;
    ctx.onWarning = [&warnings](const QString& w) { warnings << w; };

    // REQ-062: в режиме «Вакуум» это ПРЕДУПРЕЖДЕНИЕ — режим обязан продолжиться.
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    ASSERT_FALSE(warnings.isEmpty());
    EXPECT_TRUE(warnings.first().contains(QStringLiteral("LEAK_DETECTED")));
    EXPECT_TRUE(warnings.first().contains(QStringLiteral("DD312")));
}

TEST(VacuumTree, LeakTestSkippedWithReasonWithoutThreshold)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.finalPumpingValves.clear();
    ctx.leakChannels.clear();          // dP_leak_max в конфигурации нет

    QList<VacuumNode> skippedNodes;
    QStringList       reasons;
    ctx.onStageSkipped = [&](VacuumNode n, const QString& r) {
        skippedNodes << n;
        reasons << r;
    };
    QStringList warnings;
    ctx.onWarning = [&warnings](const QString& w) { warnings << w; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    // Ключевое: «не проверяли» объявлено явно и НЕ выглядит как «пройдено».
    EXPECT_TRUE(skippedNodes.contains(VacuumNode::LeakTest));
    EXPECT_TRUE(reasons.join(QStringLiteral(" ")).contains(QStringLiteral("dP_leak_max")));
    EXPECT_TRUE(warnings.isEmpty());
}

TEST(VacuumTree, LeakTestCountsInvalidReadingAsUnmeasuredNotTight)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.finalPumpingValves.clear();

    // Мёртвый датчик, отдающий ноль. Без проверки качества это выглядело бы
    // как идеальная герметичность — самый опасный из возможных ложных «ОК».
    LeakChannel ch;
    ch.name    = QStringLiteral("DD312");
    ch.maxRise = 0.01;
    ch.read    = [] { return Reading(0.0, Quality::NoResponse); };
    ctx.leakChannels << ch;

    QStringList warnings;
    ctx.onWarning = [&warnings](const QString& w) { warnings << w; };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    ASSERT_FALSE(warnings.isEmpty());
    EXPECT_TRUE(warnings.first().contains(QStringLiteral("не проверены")));
}

TEST(VacuumTree, LeakTestRunsWithBothPumpsClosed)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.finalPumpingValves.clear();

    LeakChannel ch;
    ch.name    = QStringLiteral("DD312");
    ch.maxRise = 0.01;
    ch.read    = [] { return Reading(0.1); };
    ctx.leakChannels << ch;

    // Насосы на время выдержки обязаны быть закрыты: иначе меряется не
    // натекание, а работа насоса.
    bool pumpOpenDuringLeakTest = false;
    bool inLeakTest = false;
    ctx.onNode = [&](VacuumNode node, NodeState state) {
        if (node == VacuumNode::LeakTest)
            inLeakTest = (state == NodeState::Running);
    };
    ctx.setValve = [&](bool open, const QString& name) -> bool {
        if (inLeakTest && open
            && (name == VacuumValve::K176 || name == VacuumValve::K179))
            pumpOpenDuringLeakTest = true;
        rec.ops.append({open, name});
        rec.state[name] = open;
        return true;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_FALSE(pumpOpenDuringLeakTest);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(rec.allClosed());
}

// ── 11.9–11.11: финальная откачка камеры (REQ-064–073) ───────────────────────

TEST(VacuumTree, FinalPumpingOpensChamberTractAndReachesTurbo)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();   // изолируем 11.9–11.11
    ctx.leakTestValves.clear();

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(gasValvesNeverTouched(rec));

    // REQ-066: минимальный набор тракта до камеры открыт целиком.
    EXPECT_TRUE(everOpened(rec, VacuumValve::K173));
    EXPECT_TRUE(everOpened(rec, VacuumValve::K151));
    EXPECT_TRUE(everOpened(rec, VacuumValve::K171));
    EXPECT_TRUE(everOpened(rec, VacuumValve::K178));
    // Цель 0.5 Па лежит ниже гейта 50 Па ⇒ турбо-область (REQ-067/069).
    EXPECT_TRUE(everOpened(rec, VacuumValve::K179));
    // Выход в атмосферу в тракте до камеры недопустим.
    EXPECT_FALSE(everOpened(rec, VacuumValve::K192));

    // REQ-064/066: тракт собран ДО подключения насоса.
    const QList<int> openK151 = opIndexes(rec, true, VacuumValve::K151);
    const QList<int> openK176 = opIndexes(rec, true, VacuumValve::K176);
    ASSERT_FALSE(openK151.isEmpty());
    ASSERT_FALSE(openK176.isEmpty());
    EXPECT_LT(openK151.last(), openK176.last());
}

TEST(VacuumTree, FinalPumpingStaysOnForevacWhenTargetIsInForevacRange)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();
    // REQ-067: цель выше порога перехода — форвакуумная область, К179 не нужен.
    ctx.targetVacuumPa = 100.0;

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(everOpened(rec, VacuumValve::K176));
    EXPECT_FALSE(everOpened(rec, VacuumValve::K179));
}

TEST(VacuumTree, FinalPumpingReportsReasonWhenTargetNotReached)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();
    // Гейт перехода опущен ниже цели ⇒ цель попадает в форвакуумную область,
    // контроль идёт по ДВ301 (REQ-069). ДВ301 отдаёт 5 Па, цель 1 Па —
    // время истекло, цель не достигнута.
    ctx.turboSwitchPressurePa = 0.1;
    ctx.targetVacuumPa        = 1.0;
    ctx.pressureVacPa         = [] { return 5.0; };

    QStringList failures;
    ctx.onFailure = [&failures](const QString& r) { failures << r; };

    // Недостигнутая цель — не отказ режима: прогон завершается, причина
    // записана, клапаны закрыты (REQ-069 + согласованная дивергенция по
    // maxAdditionalEvacTime).
    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    ASSERT_FALSE(failures.isEmpty());
    EXPECT_TRUE(failures.join(QStringLiteral(" ")).contains(QStringLiteral("не достигнута")));
}

TEST(VacuumTree, FinalPumpingSkippedWithReasonWhenNotConfigured)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();
    ctx.finalPumpingValves.clear();

    QList<VacuumNode> skippedNodes;
    QStringList       reasons;
    ctx.onStageSkipped = [&](VacuumNode n, const QString& r) {
        skippedNodes << n;
        reasons << r;
    };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(skippedNodes.contains(VacuumNode::FinalPrep));
    EXPECT_TRUE(skippedNodes.contains(VacuumNode::FinalPumping));
    EXPECT_TRUE(reasons.join(QStringLiteral(" ")).contains(QStringLiteral("finalPumping")));
}

TEST(VacuumTree, FinalCloseClosesEveryValveItOpened)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    // REQ-070/071: каждое закрытие — отдельная операция, значит отдельная
    // строка журнала. Проверяем, что для каждого открытия нашлось закрытие.
    for (const QString& v : { VacuumValve::K173, VacuumValve::K151,
                              VacuumValve::K171, VacuumValve::K178,
                              VacuumValve::K176, VacuumValve::K179 }) {
        if (!everOpened(rec, v))
            continue;
        EXPECT_FALSE(opIndexes(rec, false, v).isEmpty())
            << "клапан " << v.toStdString() << " открывался, но не закрывался";
    }
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
}

TEST(VacuumTree, CancelDuringFinalPumpingClosesChamberTract)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);
    ctx.generalPumpingValves.clear();
    ctx.leakTestValves.clear();
    ctx.evacTimeSec = 500;                      // застрять в откачке

    QTaskTree tree;
    ctx.onNode = [&tree](VacuumNode node, NodeState state) {
        if (node == VacuumNode::FinalPumping && state == NodeState::Running)
            QTimer::singleShot(0, &tree, [&tree] { tree.cancel(); });
    };
    tree.setRecipe(buildVacuumRecipe(ctx));

    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&loop](DoneWith) { loop.quit(); });
    tree.start();
    loop.exec();

    // Отмена посреди откачки камеры обязана оставить установку закрытой:
    // открытый насосный клапан после «Стоп» — то, чего быть не должно.
    EXPECT_TRUE(rec.allClosed());
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
}

TEST(VacuumTree, FullTractRunKeepsPumpInterlockAcrossAllStages)
{
    Recorder rec;
    VacuumTreeContext ctx = makeTractCtx(rec);   // 11.7б + 11.8 + 11.9–11.11

    LeakChannel ch;
    ch.name    = QStringLiteral("DD312");
    ch.maxRise = 0.01;
    ch.read    = [] { return Reading(0.1); };
    ctx.leakChannels << ch;

    QList<bool> switches;
    ctx.onTurboSwitched = [&switches](bool toTurbo) { switches.append(toTurbo); };

    EXPECT_EQ(runBlocking(buildVacuumRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
    // Главный инвариант прогона целиком, через все три новых этапа.
    EXPECT_TRUE(pumpsNeverBothOpen(rec));
    EXPECT_TRUE(gasValvesNeverTouched(rec));
    EXPECT_FALSE(everOpened(rec, VacuumValve::K192));
    // Переход на турбонасос состоялся дважды — в 11.7б и в 11.10, — и оба раза
    // именно на турбо, без откатов.
    EXPECT_EQ(switches, (QList<bool>{ true, true }));
}
