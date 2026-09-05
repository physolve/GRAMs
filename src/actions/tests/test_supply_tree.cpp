// ─────────────────────────────────────────────────────────────────────────────
// SupplyTreeTests — тесты рецепта «Напуск» без железа.
//
// Проверяется то, чего легаси-InletAction не давал в принципе:
//   гейт цепочки (напуск на неоткачанный тракт не начинается);
//   различимые причины штатной остановки (время / давление / вето quartile);
//   закрытие клапана на ЛЮБОМ исходе, включая отмену.
//
// Все швы — через SupplyTreeContext; ValveControl/DataAcquisition не нужны.
// ─────────────────────────────────────────────────────────────────────────────

#include "recipes/SupplyTaskTree.h"

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

struct SupplyOp {
    bool    open;
    QString name;
};

// Мок железа и подсистем сбора данных.
class SupplyRecorder
{
public:
    QList<SupplyOp>     ops;
    QMap<QString, bool> state;
    QStringList         blockedOpens;

    int  actionModeCalls = 0;
    bool actionActive    = false;
    int  fastReads       = 0;
    int  runSupplyCalls  = 0;
    int  appendCalls     = 0;
    int  fillCalls       = 0;
    int  saveCalls       = 0;

    // По умолчанию: тракт откачан, давление не растёт, quartile не возражает.
    std::function<Reading()> pressure = [] { return Reading(0.0); };
    std::function<bool()>    quartileOk = [] { return true; };
    std::function<bool()>    interrupted = [] { return false; };

    SupplyTreeContext makeCtx()
    {
        SupplyTreeContext ctx;
        ctx.port            = QStringLiteral("AR1");
        ctx.tickIntervalMs  = 1;      // 1 тик = 1 мс, чтобы тесты были быстрыми
        ctx.preReadDelayMs  = 1;
        ctx.postReadDelayMs = 1;
        ctx.openTimeMs      = 10;
        ctx.pressureLimitBar = 100.0; // по умолчанию предел недостижим

        ctx.setValve = [this](bool open, const QString& name) -> bool {
            if (open && blockedOpens.contains(name))
                return false;
            ops.append({open, name});
            state[name] = open;
            return true;
        };
        ctx.confirmValve = [this](bool expectedOpen, const QString& name) {
            return state.value(name, false) == expectedOpen;
        };
        ctx.pressureStorageBar = [this] { return pressure(); };
        ctx.setActionMode = [this](bool active) {
            ++actionModeCalls;
            actionActive = active;
        };
        ctx.fastBufferRead  = [this] { ++fastReads; };
        ctx.runSupplyAction = [this] { ++runSupplyCalls; };
        ctx.fillSupplyData  = [this](int) { ++fillCalls; };
        ctx.appendSupplyData = [this](int) { ++appendCalls; return true; };
        ctx.saveSupplyData  = [this] { ++saveCalls; };
        ctx.checkSupplyAction = [this] { return quartileOk(); };
        ctx.actionInterrupted = [this] { return interrupted(); };
        return ctx;
    }

    QString sequence() const
    {
        QStringList parts;
        for (const SupplyOp& op : ops)
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

DoneWith runBlockingSupply(const Group& recipe)
{
    QTaskTree tree(recipe);
    return tree.runBlocking();
}

} // namespace

// ─── Гейт цепочки ────────────────────────────────────────────────────────────

TEST(SupplyTree, GateRejectsWhenTractNotEvacuated)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.gateMaxStartBar = 0.05;
    rec.pressure = [] { return Reading(1.2); };   // тракт под давлением

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Error);
    // Клапан порта не открывался вовсе.
    EXPECT_TRUE(rec.ops.isEmpty());
    EXPECT_TRUE(reason.contains(QStringLiteral("не откачан")));
}

TEST(SupplyTree, GateRejectsOnInvalidReading)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    rec.pressure = [] { return Reading(0.0, Quality::NoResponse); };

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    // Мёртвый датчик отдаёт 0 бар, что формально «ниже порога». Без проверки
    // качества гейт пропустил бы напуск на неизвестном давлении.
    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(rec.ops.isEmpty());
    EXPECT_TRUE(reason.contains(QStringLiteral("недостоверно")));
}

TEST(SupplyTree, GateRejectsWhenPortAlreadyOpen)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    rec.state[ctx.port] = true;      // клапан порта уже открыт

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Error);
}

TEST(SupplyTree, GateCanBeDisabled)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.gateCheck = false;
    rec.pressure = [] { return Reading(1.2); };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    EXPECT_TRUE(rec.allClosed());
}

// ─── Причины штатной остановки ───────────────────────────────────────────────

TEST(SupplyTree, StopsOnTimeElapsed)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs = 5;

    SupplyStop stop = SupplyStop::None;
    ctx.onFinished = [&stop](SupplyStop s) { stop = s; };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, SupplyStop::TimeElapsed);
    EXPECT_EQ(rec.sequence(), QStringLiteral("+AR1 -AR1"));
    EXPECT_TRUE(rec.allClosed());
    EXPECT_EQ(rec.saveCalls, 1);
}

TEST(SupplyTree, StopsOnPressureLimit)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs       = 100000;   // по времени не остановится
    ctx.pressureLimitBar = 2.0;

    // Давление растёт по 0.5 бар за вызов; гейт читает первым, поэтому старт
    // ещё «откачан», а предел достигается в цикле накопления.
    auto p = std::make_shared<double>(0.0);
    rec.pressure = [p] {
        const double v = *p;
        *p += 0.5;
        return Reading(v);
    };

    SupplyStop stop = SupplyStop::None;
    ctx.onFinished = [&stop](SupplyStop s) { stop = s; };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, SupplyStop::PressureReached);
    EXPECT_TRUE(rec.allClosed());
}

TEST(SupplyTree, StopsOnQuartileVeto)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs = 100000;

    auto calls = std::make_shared<int>(0);
    rec.quartileOk = [calls] { return ++(*calls) < 3; };

    SupplyStop stop = SupplyStop::None;
    ctx.onFinished = [&stop](SupplyStop s) { stop = s; };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    EXPECT_EQ(stop, SupplyStop::QuartileVeto);
    EXPECT_TRUE(rec.allClosed());
}

// ─── Отказы и отмена ─────────────────────────────────────────────────────────

TEST(SupplyTree, BlockedPortValveReportsReasonAndClosesAction)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    rec.blockedOpens << ctx.port;

    QString reason;
    ctx.onFailure = [&reason](const QString& r) { if (reason.isEmpty()) reason = r; };

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Error);
    EXPECT_TRUE(reason.contains(QStringLiteral("не открылся")));
    // Режим действия обязан быть снят даже при отказе открытия.
    EXPECT_FALSE(rec.actionActive);
}

TEST(SupplyTree, CancelDuringRunClosesPortAndAction)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs     = 1000000;    // сам не закончится
    ctx.tickIntervalMs = 5;

    SupplyStop stop = SupplyStop::None;
    ctx.onFinished = [&stop](SupplyStop s) { stop = s; };

    QTaskTree tree(buildSupplyRecipe(ctx));
    QEventLoop loop;
    QObject::connect(&tree, &QTaskTree::done, &loop, [&loop](DoneWith) { loop.quit(); });
    tree.start();
    QTimer::singleShot(60, &tree, [&tree] { tree.cancel(); });
    loop.exec();

    // Главное: клапан закрыт и режим действия снят, в какой бы момент ни
    // пришла отмена. Легаси-цикл такой гарантии не давал.
    EXPECT_TRUE(rec.allClosed());
    EXPECT_FALSE(rec.actionActive);
    EXPECT_EQ(stop, SupplyStop::Cancelled);
    EXPECT_EQ(rec.saveCalls, 1);     // собранные точки не теряются
}

// ─── Перенос поведения легаси ────────────────────────────────────────────────

TEST(SupplyTree, PreparesBufferBeforeOpeningValve)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs = 3;

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    // Легаси: fillSupplyActionData(0) → msleep → fastBufferRead → msleep →
    // открыть клапан. Нулевая точка и прогрев буфера — до открытия.
    EXPECT_EQ(rec.fillCalls, 1);
    EXPECT_GE(rec.fastReads, 1);
    EXPECT_GE(rec.appendCalls, 1);
    EXPECT_GE(rec.runSupplyCalls, 1);
}

TEST(SupplyTree, ManualValveInterferenceReclaimsActionMode)
{
    SupplyRecorder rec;
    SupplyTreeContext ctx = rec.makeCtx();
    ctx.openTimeMs = 5;

    auto calls = std::make_shared<int>(0);
    rec.interrupted = [calls] { return ++(*calls) == 2; };   // один раз вмешались

    EXPECT_EQ(runBlockingSupply(buildSupplyRecipe(ctx)), DoneWith::Success);
    // beginAction(true) на подготовке, ещё раз после вмешательства,
    // endAction(false) в конце — минимум три вызова.
    EXPECT_GE(rec.actionModeCalls, 3);
    EXPECT_FALSE(rec.actionActive);
}
