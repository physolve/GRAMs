// ─────────────────────────────────────────────────────────────────────────────
// LeakageTreeTests — тесты рецепта «Натекание» без железа.
//
// До рецепта натекание не было прогоном: тумблер включал сбор данных, и всё.
// Здесь проверяется именно то, что добавил рецепт — предусловие, отмеряемая
// длительность, различимая причина завершения и закрытие клапана на любом
// исходе. Физика GasLeakage не проверяется: она не менялась.
// ─────────────────────────────────────────────────────────────────────────────

#include "recipes/LeakageTaskTree.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QMap>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <qtasktree.h>

using namespace QtTaskTree;

namespace {

class LeakageRecorder
{
public:
    QMap<QString, bool> state;
    QStringList         blockedOpens;
    QStringList         calls;        // порядок вызовов швов GasLeakage

    int  addMeasures  = 0;
    int  saveCalls    = 0;
    int  endCalls     = 0;
    bool measureOn    = false;

    std::function<Reading()> storage  = [] { return Reading(2.0); };
    std::function<Reading()> reaction = [] { return Reading(0.0); };

    LeakageTreeContext makeCtx()
    {
        LeakageTreeContext ctx;
        ctx.valve          = QStringLiteral("R1");
        ctx.tickIntervalMs = 1;
        ctx.durationSec    = 5;

        ctx.setValve = [this](bool open, const QString& name) -> bool {
            if (open && blockedOpens.contains(name))
                return false;
            state[name] = open;
            calls << (open ? QStringLiteral("+") : QStringLiteral("-")) + name;
            return true;
        };
        ctx.confirmValve = [this](bool expectedOpen, const QString& name) {
            return state.value(name, false) == expectedOpen;
        };
        ctx.pressureStorageBar  = [this] { return storage(); };
        ctx.pressureReactionBar = [this] { return reaction(); };
        ctx.temperatureReactionK = [] { return 298.15; };

        ctx.setMeasureMode = [this](bool on) {
            measureOn = on;
            calls << (on ? QStringLiteral("measure:on") : QStringLiteral("measure:off"));
        };
        ctx.leakageStart = [this](double, double, double) { calls << QStringLiteral("startCalc"); };
        ctx.leakageAdd   = [this](double, double, double) { ++addMeasures; };
        ctx.leakageSetOpen = [this](bool o) {
            calls << (o ? QStringLiteral("open:true") : QStringLiteral("open:false"));
        };
        ctx.leakageEnd  = [this] { ++endCalls;  calls << QStringLiteral("endCalc"); };
        ctx.leakageSave = [this] { ++saveCalls; calls << QStringLiteral("save"); };
        return ctx;
    }

    bool allClosed() const
    {
        for (bool open : state)
            if (open)
                return false;
        return true;
    }
};

DoneWith runBlockingLeakage(const Group& recipe)
{
    QTaskTree tree(recipe);
    return tree.runBlocking();
}

} // namespace

// ─── Гейт цепочки ────────────────────────────────────────────────────────────

TEST(LeakageTree, GateRejectsEmptyStorage)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.gateMinStorageBar = 0.5;
    rec.storage = [] { return Reading(0.1); };    // накопитель пуст

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.calls.isEmpty());             // ничего не трогали
    EXPECT_TRUE(reason.contains(QStringLiteral("не заряжен")));
}

TEST(LeakageTree, GateRejectsUnevacuatedChamber)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.gateMaxReactionBar = 0.05;
    rec.reaction = [] { return Reading(0.9); };   // камера под давлением

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(reason.contains(QStringLiteral("не откачана")));
}

TEST(LeakageTree, GateRejectsInvalidReadings)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    rec.storage = [] { return Reading(2.0, Quality::NoResponse); };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.calls.isEmpty());
}

// ─── Причины завершения ──────────────────────────────────────────────────────

TEST(LeakageTree, StopsOnDuration)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.durationSec = 4;

    LeakageStop stop = LeakageStop::None;
    ctx.onFinished = [&stop](LeakageStop s) { stop = s; };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, LeakageStop::DurationElapsed);
    EXPECT_EQ(rec.addMeasures, 4);                // по точке на тик
    EXPECT_TRUE(rec.allClosed());
    EXPECT_EQ(rec.saveCalls, 1);
    EXPECT_EQ(rec.endCalls, 1);
}

TEST(LeakageTree, StopsOnTargetDelta)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.durationSec    = 100000;                  // по времени не остановится
    ctx.targetDeltaBar = 1.0;

    // Накопитель стравливается по 0.4 бар за чтение.
    auto p = std::make_shared<double>(3.0);
    rec.storage = [p] {
        const double v = *p;
        *p -= 0.4;
        return Reading(v);
    };

    LeakageStop stop = LeakageStop::None;
    ctx.onFinished = [&stop](LeakageStop s) { stop = s; };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, LeakageStop::TargetReached);
    EXPECT_TRUE(rec.allClosed());
}

TEST(LeakageTree, StopsWhenSensorGoesInvalid)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.durationSec = 100000;

    // Датчик отваливается после нескольких чтений: продолжать измерение,
    // которому нечем мерить, бессмысленно.
    auto n = std::make_shared<int>(0);
    rec.storage = [n] {
        return (++(*n) < 4) ? Reading(2.0) : Reading(2.0, Quality::NoResponse);
    };

    LeakageStop stop = LeakageStop::None;
    ctx.onFinished = [&stop](LeakageStop s) { stop = s; };

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, LeakageStop::SensorInvalid);
    EXPECT_TRUE(rec.allClosed());
}

// ─── Отказы и отмена ─────────────────────────────────────────────────────────

TEST(LeakageTree, BlockedValveStillClosesCalcAndMeasure)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    rec.blockedOpens << ctx.valve;

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Error);
    // startCalc уже был вызван — расчёт обязан быть закрыт, сбор данных снят.
    EXPECT_EQ(rec.endCalls, 1);
    EXPECT_FALSE(rec.measureOn);
}

TEST(LeakageTree, CancelClosesValveAndSaves)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.durationSec    = 1000000;
    ctx.tickIntervalMs = 5;

    LeakageStop stop = LeakageStop::None;
    ctx.onFinished = [&stop](LeakageStop s) { stop = s; };

    QTaskTree tree(buildLeakageRecipe(ctx));
    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&loop](DoneWith) { loop.quit(); });
    tree.start();
    QTimer::singleShot(60, &tree, [&tree] { tree.cancel(); });
    loop.exec();

    EXPECT_TRUE(rec.allClosed());
    EXPECT_FALSE(rec.measureOn);
    EXPECT_EQ(stop, LeakageStop::Cancelled);
    EXPECT_EQ(rec.saveCalls, 1);
}

TEST(LeakageTree, CallOrderMatchesLegacySequence)
{
    LeakageRecorder rec;
    LeakageTreeContext ctx = rec.makeCtx();
    ctx.durationSec = 2;

    EXPECT_EQ(runBlockingLeakage(buildLeakageRecipe(ctx)), DoneWith::Success);
    // Сбор данных включается до расчёта, клапан открывается после startCalc,
    // закрытие идёт в обратном порядке.
    EXPECT_EQ(rec.calls,
              (QStringList{ QStringLiteral("measure:on"),
                            QStringLiteral("startCalc"),
                            QStringLiteral("+R1"),
                            QStringLiteral("open:true"),
                            QStringLiteral("-R1"),
                            QStringLiteral("open:false"),
                            QStringLiteral("endCalc"),
                            QStringLiteral("save"),
                            QStringLiteral("measure:off") }));
}
