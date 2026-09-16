// Интеграционный тест: рецепт «Вакуума» на демо-данных через настоящий путь.
//
//   SimController (профиль) → SimSensorSource → DataAcquisition::processEvents
//   → ControllerData/DataCollection (калибровка, Торр) → швы рецепта
//   рецепт → ValveControl::setValveFromAction → Security → SimValveEcho
//   → фронт клапана → SimController
//
// Контекст рецепта собирает тест (решение Р2 плана): те же лямбды, что
// VacuumRegimeWorker::makeContext, поверх настоящих объектов. Время ускорено
// часами: 1 такт рецепта (10 мс) = 1 с демо-времени. Рецепт не меняется.

#include "test_support.h"

#include "core/SimController.h"
#include "hw/SimSensorSource.h"
#include "hw/SimValveEcho.h"

#include "DataAcquisition.h"
#include "DataCollection.h"
#include "Security.h"
#include "ValveControl.h"
#include "ValveModel.h"
#include "actions/recipes/VacuumTaskTree.h"

#include <QElapsedTimer>
#include <QTimer>
#include <qtasktree.h>

#include <memory>

using namespace sim;
using namespace simtest;
using namespace QtTaskTree;

namespace {

constexpr int kTickMs = 10;   // такт рецепта; 1 такт = 1 с демо-времени

// Демо-время идёт в том же масштабе, что и такты рецепта.
class WarpClock final : public ISimClock {
public:
    WarpClock() { m_timer.start(); }
    double nowSec() const override { return double(m_timer.elapsed()) / kTickMs; }

private:
    QElapsedTimer m_timer;
};

struct ValveOp {
    QString name;
    bool open;
    bool ok;
};

// Установка GRAM50 без железа: те же объекты и связи, что в Grams.
class Rig {
public:
    WarpClock clock;
    std::unique_ptr<SimController> controller;

    DataAcquisition daq;
    ControllerData time;
    QList<std::shared_ptr<ControllerData>> pressure;   // DD311 … DT314 (USB-4716)
    QList<std::shared_ptr<ControllerData>> temperature;  // DT350 … DT359 (USB-4718)
    QList<std::shared_ptr<FilterData>> filters;
    DataCollection dv301;
    DataCollection dv302;

    Security security;
    ValveControl valves;
    QList<std::shared_ptr<Valve>> valveObjects;
    Valve chamberValve;

    QList<ValveOp> ops;
    bool bothPumpsSeenOpen = false;

    Rig()
    {
        const CatalogBuild &build = gram50();
        controller = std::make_unique<SimController>(build.catalog, &clock);
        const CatalogInput input = gram50Input();

        QVector<ControllerData *> pres, temp;
        QVector<FilterData *> fl;
        for (const auto &s : input.pressureCard) {
            auto cd = std::make_shared<ControllerData>(s.name);
            cd->setCoeffs(s.A / s.R * 1000.0, s.B);   // Grams::initAnalogData
            pressure << cd;
            pres << cd.get();
            auto f = std::make_shared<FilterData>();
            filters << f;
            fl << f.get();
        }
        for (const auto &name : input.temperatureCard) {
            auto cd = std::make_shared<ControllerData>(name);
            cd->setCoeffs(1.0, 0.0);
            temperature << cd;
            temp << cd.get();
        }
        dv301.setAltUnitCoef(0.001333);
        dv302.setAltUnitCoef(0.001333);

        daq.setTimePointer(&time);
        daq.setPressurePointers(pres);
        daq.setFiltersDataPointers(fl);
        daq.setSensorSource(std::make_unique<SimSensorSource>(controller.get(), build.bindings));
        daq.setTempPointers(temp);
        daq.markControllersConnected();
        daq.setVacuumPointer(&dv301);
        daq.setTurboVacuumPointer(&dv302);

        // Клапаны и интерлоки — из профиля установки, как Grams::advDoController
        // и Grams::initSafeModule.
        const QJsonObject gram = readJson(QStringLiteral(GRAMS_PROFILE_JSON)).object().value("GRAM50").toObject();
        QVector<Valve *> list;
        for (const auto &code : input.valveCodes) {
            auto v = std::make_shared<Valve>();
            v->m_name = code;
            valveObjects << v;
            list << v.get();
        }
        chamberValve.m_name = "R5";
        valves.setValvePointers(list);
        valves.setChamberValvePointer(&chamberValve);
        valves.setSafeModule(&security);
        valves.setSafeModuleInitialValveState();

        QStringList ids;
        for (const auto &code : input.valveCodes)
            ids << build.catalog.valveByCode(code)->id;
        valves.setDoPort(std::make_unique<SimValveEcho>(ids, controller.get()));

        security.constructValveMap(input.valveCodes);
        QMap<QString, QStringList> contradictions;
        const QJsonObject sec = gram.value("security").toObject();
        const QJsonObject cv = sec.value("contradictionValves").toObject();
        for (auto it = cv.begin(); it != cv.end(); ++it)
            contradictions.insert(it.key(), it.value().toVariant().toStringList());
        security.setContradictionValves(contradictions);
        security.setRuleOfThreeValves(sec.value("twoOfThree").toVariant().toStringList());
    }

    void poll() { QMetaObject::invokeMethod(&daq, "processEvents", Qt::DirectConnection); }

    ControllerData *sensor(const QString &name)
    {
        for (const auto &p : pressure)
            if (p->m_name == name) return p.get();
        return nullptr;
    }

    // Как VacuumRegimeWorker::makeContext, с быстрыми тактами.
    VacuumTreeContext makeContext()
    {
        const QJsonObject gram = readJson(QStringLiteral(GRAMS_PROFILE_JSON)).object().value("GRAM50").toObject();
        const QJsonObject safety = gram.value("vacuumSafety").toObject();
        const QJsonObject tract = gram.value("vacuumTract").toObject();

        VacuumTreeContext ctx;
        ctx.tickIntervalMs = kTickMs;
        ctx.perActionPauseMs = 0;
        ctx.k178PulseMs = kTickMs;
        ctx.reliefDwellSec = 1;
        ctx.evacTimeSec = 900;

        ctx.turboSwitchPressurePa = safety.value("turboSwitchPressurePa").toDouble(50);
        ctx.turboSwitchHoldSec = safety.value("turboSwitchHoldSec").toInt(60);
        ctx.turboReturnPressurePa = safety.value("turboReturnPressurePa").toDouble(150);
        ctx.turboTimeoutSec = safety.value("turboTimeoutSec").toInt(600);
        ctx.testEvacTimeSec = safety.value("testEvacTimeSec").toInt(300);
        ctx.leakTestDurationSec = safety.value("leakTestDurationSec").toInt(60);
        ctx.generalPumpingValves = tract.value("generalPumping").toVariant().toStringList();
        ctx.leakTestValves = tract.value("leakTest").toVariant().toStringList();
        ctx.finalPumpingValves = tract.value("finalPumping").toVariant().toStringList();

        ControllerData *dd312 = sensor("DD312");
        const double maxRise = safety.value("dP_leak_max").toObject().value("DD312").toDouble(0.01);
        ctx.leakChannels.append({QStringLiteral("DD312"),
                                 [dd312] { return Reading(dd312->getCurValue(), dd312->quality()); }, maxRise});

        ctx.pressureVacPa = [this] { return Reading(dv301.getCurValue() * 133.322, dv301.quality()); };
        ctx.pressureTurboPa = [this] { return Reading(dv302.getCurValue() * 133.322, dv302.quality()); };
        // В Grams страж ДВ_СБР смотрит на объём B (prSB), выведенный квартилем
        // из DD311/DD312; квартилей в тесте нет — берём DD311 напрямую.
        ControllerData *dd311 = sensor("DD311");
        ctx.pressureB = [dd311] { return dd311->getCurValue(); };

        ctx.setValve = [this](bool open, const QString &name) {
            const bool ok = valves.setValveFromAction(open, name);
            ops.append({name, open, ok});
            if (valves.valveState("AR6") && valves.valveState("SL2"))
                bothPumpsSeenOpen = true;
            return ok;
        };
        ctx.confirmValve = [this](bool expectedOpen, const QString &name) {
            return valves.confirmValve(name, expectedOpen);
        };
        ctx.onNode = [this](VacuumNode node, NodeState state) { nodes.append({node, state}); };
        return ctx;
    }

    QList<std::pair<VacuumNode, NodeState>> nodes;

    bool nodeReached(VacuumNode node, NodeState state) const
    {
        for (const auto &n : nodes)
            if (n.first == node && n.second == state) return true;
        return false;
    }

    int lastOpIndex(const QString &name, bool open) const
    {
        for (int i = ops.size() - 1; i >= 0; --i)
            if (ops[i].name == name && ops[i].open == open && ops[i].ok) return i;
        return -1;
    }

    QStringList historyPhases() const
    {
        QStringList out;
        for (const auto &h : controller->history())
            out << h.phaseId;
        return out;
    }

    DoneWith run(const char *profileFile)
    {
        Profile profile;
        const auto v = parseProfile(readJson(profilePath(QString::fromLatin1(profileFile))).object(),
                                    gram50().catalog, &profile);
        EXPECT_TRUE(v.ok());
        EXPECT_TRUE(controller->run(profile).ok());

        // «Опрос DataAcquisition»: вдвое чаще такта рецепта, как 500 мс к 1 с.
        QTimer pollTimer;
        pollTimer.setTimerType(Qt::PreciseTimer);
        QObject::connect(&pollTimer, &QTimer::timeout, &pollTimer, [this] { poll(); });
        pollTimer.start(kTickMs / 2);
        poll();

        QTaskTree tree(buildVacuumRecipe(makeContext()));
        const DoneWith result = tree.runBlocking();
        pollTimer.stop();
        return result;
    }
};

std::string dumpOps(const QList<ValveOp> &ops)
{
    std::string s;
    for (const auto &op : ops)
        s += (op.open ? "+" : "-") + op.name.toStdString() + (op.ok ? " " : "! ");
    return s;
}

} // namespace

TEST(SimVacuumIntegration, CalibratedPathCarriesDemoValues)
{
    Rig rig;
    rig.controller->setValue("P.DD331", 250000.0);
    rig.controller->setValue("VAC.DV302", 40.0);
    rig.poll();
    rig.poll();   // давление приходит со следующего опроса, как у платы
    EXPECT_NEAR(rig.sensor("DD331")->getCurValue(), 2.5, 1e-9);
    EXPECT_NEAR(rig.dv302.getCurValue() * 133.322, 40.0, 1e-9);
    EXPECT_EQ(rig.dv302.quality(), Quality::Valid);

    // Интерлок AR6⇄SL2 из профиля работает на демо-порту так же, как на плате.
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "AR6"));
    EXPECT_TRUE(rig.controller->isValveOpen("K176"));
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "SL2"));
    EXPECT_FALSE(rig.controller->isValveOpen("K179"));
    EXPECT_TRUE(rig.valves.confirmValve("AR6", true));
}

TEST(SimVacuumIntegration, ForeTurboProfileRunsToCompletion)
{
    Rig rig;
    const DoneWith result = rig.run("vacuum_fore_turbo.json");
    EXPECT_EQ(result, DoneWith::Success) << dumpOps(rig.ops);

    EXPECT_TRUE(rig.nodeReached(VacuumNode::TurboSwitch, NodeState::Success));
    EXPECT_FALSE(rig.nodeReached(VacuumNode::TurboFallback, NodeState::Running));
    EXPECT_TRUE(rig.nodeReached(VacuumNode::FinalPumping, NodeState::Success));
    EXPECT_GE(rig.lastOpIndex("SL2", true), 0) << dumpOps(rig.ops);
    EXPECT_FALSE(rig.bothPumpsSeenOpen);

    const QStringList phases = rig.historyPhases();
    EXPECT_TRUE(phases.contains("pump_fore")) << phases.join(',').toStdString();
    EXPECT_TRUE(phases.contains("pump_turbo")) << phases.join(',').toStdString();
    for (const auto &h : rig.controller->history())
        if (h.phaseId != "static_atm")
            EXPECT_EQ(h.reason, "valve") << h.phaseId.toStdString();

    // Рецепт закрыл всё, что открывал.
    for (const auto &v : rig.valveObjects)
        EXPECT_FALSE(v->getState()) << v->m_name.toStdString();
}

TEST(SimVacuumIntegration, LeakProfileTriggersTurboFallback)
{
    Rig rig;
    const DoneWith result = rig.run("vacuum_leak.json");

    // ТЗ (vacuum.md, 12.2 и §7б): рост ДВ302 до turboReturnPressurePa — откат на
    // форвакуум, а не авария; финальная откачка идёт на К176 по ДВ301.
    EXPECT_TRUE(rig.nodeReached(VacuumNode::TurboFallback, NodeState::Running)) << dumpOps(rig.ops);
    EXPECT_TRUE(rig.historyPhases().contains("leak_turbo"));
    EXPECT_FALSE(rig.bothPumpsSeenOpen);

    const int turboClosed = rig.lastOpIndex("SL2", false);
    const int turboOpened = rig.lastOpIndex("SL2", true);
    ASSERT_GE(turboOpened, 0);
    EXPECT_GT(turboClosed, turboOpened) << dumpOps(rig.ops);
    EXPECT_GT(rig.lastOpIndex("AR6", true), turboOpened) << dumpOps(rig.ops);

    EXPECT_EQ(result, DoneWith::Success) << dumpOps(rig.ops);
    for (const auto &v : rig.valveObjects)
        EXPECT_FALSE(v->getState()) << v->m_name.toStdString();
}
