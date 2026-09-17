// Клапаны на демо-данных: щелчок, интерлоки, «Тест клапанов», проверка давления.
//
// Путь тот же, что в Grams, и в том же порядке инициализации:
//   ValveControl::setValvePointers → setDoPort
//   → Grams::initSafeModule (интерлоки, пороги из профиля).
//
// Каждый тест с пометкой Dn воспроизводил дефект и падал до своего исправления.

#include "test_support.h"

#include "core/SimClock.h"
#include "core/SimController.h"
#include "core/SimProfile.h"
#include "hw/SimValveEcho.h"

#include "Security.h"
#include "ValveControl.h"
#include "ValveModel.h"
#include "actions/workers/RegimeWorkers.h"
#include "actions/workers/ValveTestWorker.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include <cmath>
#include <functional>
#include <memory>

using namespace sim;
using namespace simtest;

namespace {

void settle()
{
    for (int i = 0; i < 20; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents);
}

// Порт с управляемым исходом записи и начальным содержимым регистра —
// то, чего не умеет эхо: отказ платы и клапан, открытый с прошлого запуска.
class FakeDoPort final : public IDoPort {
public:
    explicit FakeDoPort(QVector<bool> initial) : m_data(std::move(initial)) {}
    bool writeOk = true;
    bool write(const QVector<bool> &states) override
    {
        if (!writeOk)
            return false;
        m_data = states;
        return true;
    }
    bool refresh() override { return true; }
    void setBit(int i, bool v) { m_data[i] = v; }   // клапан переключён мимо программы
    QVector<bool> data() override { return m_data; }

private:
    QVector<bool> m_data;
};

QJsonObject gram50Profile()
{
    return readJson(QStringLiteral(GRAMS_PROFILE_JSON)).object().value("GRAM50").toObject();
}

class ValveRig {
public:
    ManualClock clock;
    SimController controller{gram50().catalog, &clock};
    Security security;
    ValveControl valves;
    QList<std::shared_ptr<Valve>> valveObjects;
    Valve chamberValve;
    int guiChanged = 0;

    // port == nullptr — эхо демо-режима; иначе переданный порт.
    // withPort == false — порт не ставится вовсе (dev-машина без платы и без --sim).
    explicit ValveRig(std::unique_ptr<IDoPort> port = nullptr, bool withPort = true)
    {
        const CatalogInput input = gram50Input();
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
        if (withPort) {
            if (!port) {
                QStringList ids;
                for (const auto &code : input.valveCodes)
                    ids << gram50().catalog.valveByCode(code)->id;
                port = std::make_unique<SimValveEcho>(ids, &controller);
            }
            valves.setDoPort(std::move(port));
        }
        QObject::connect(&valves, &ValveControl::guiValsValveChanged, &valves, [this] { ++guiChanged; });

        // Grams::initSafeModule — вызывается в конструкторе Grams позже порта.
        const QJsonObject gram = gram50Profile();
        const QJsonObject sec = gram.value("security").toObject();
        QMap<QString, QStringList> contradictions;
        const QJsonObject cv = sec.value("contradictionValves").toObject();
        for (auto it = cv.begin(); it != cv.end(); ++it)
            contradictions.insert(it.key(), it.value().toVariant().toStringList());
        security.setContradictionValves(contradictions);
        security.setRuleOfThreeValves(sec.value("twoOfThree").toVariant().toStringList());
        const QJsonObject quars = gram.value("quartiles").toObject();
        const QJsonObject storage = quars.value("storageQuar").toObject();
        const QJsonObject reaction = quars.value("reactionQuar").toObject();
        security.setRangePressureValves(storage.value("v_pressureRange").toString(), "storageQuar",
                                        storage.value("cond_pressureRange_open").toDouble(),
                                        storage.value("cond_pressureRange_close").toDouble());
        security.setRangePressureValves(reaction.value("v_pressureRange").toString(), "reactionQuar",
                                        reaction.value("cond_pressureRange_open").toDouble(),
                                        reaction.value("cond_pressureRange_close").toDouble());
        security.setSafeReleaseValves(quars.value("addRemoveQuar").toObject().value("v_gasRelease").toString(),
                                      "storageQuar", storage.value("cond_gasRelease").toDouble());
        security.setPressureStaleTicks(sec.value("pressureStaleTicks").toInt(4));
        for (const auto &s : input.pressureCard)   // как Grams::initSafeModule
            security.setSensorRange(s.name, 3.8 * s.A + s.B, 20.5 * s.A + s.B);
    }

    // Давления квартилей, бар, по датчикам: источник выбирается по клапану
    // диапазона, как StorageQuartile/ReactionQuartile::updateQuartileData.
    double dd311 = 1.0, dd312 = 1.0, dd331 = 1.0, dd332 = 1.0;
    quint64 seq = 0;

    // Такт softEvent: newSample == false — датчики не прислали новых отсчётов.
    void feedPressure(bool newSample = true)
    {
        if (newSample)
            ++seq;
        QMap<QString, PressureSample> m;
        m.insert("storageQuar", valves.valveState("S4") ? PressureSample{"DD312", dd312, seq}
                                                        : PressureSample{"DD311", dd311, seq});
        m.insert("reactionQuar", valves.valveState("R4") ? PressureSample{"DD332", dd332, seq}
                                                         : PressureSample{"DD331", dd331, seq});
        security.setPressureMap(m);
    }

    int indexOf(const QString &code) const
    {
        for (int i = 0; i < valveObjects.size(); ++i)
            if (valveObjects[i]->m_name == code)
                return i;
        return -1;
    }

    bool gui(const QString &code) const { return valves.getGuiValsValve().value(code).toBool(); }

    void startProfile(const char *file)
    {
        Profile profile;
        const auto v = parseProfile(readJson(profilePath(QString::fromLatin1(file))).object(),
                                    gram50().catalog, &profile);
        ASSERT_TRUE(v.ok());
        ASSERT_TRUE(controller.run(profile).ok());
        settle();
    }

    void simTick()
    {
        clock.advance(1.0);
        controller.tick();
        settle();
    }
};

// Крутит цикл событий, пока pred() не станет true или не выйдет таймаут;
// sample() вызывается каждые 50 мс.
bool spin(int timeoutMs, const std::function<bool()> &pred, const std::function<void()> &sample = {})
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        if (sample)
            sample();
        if (pred())
            return true;
    }
    return pred();
}

class NoopRegimeWorker final : public RegimeWorkerBase {
public:
    using RegimeWorkerBase::RegimeWorkerBase;
};

} // namespace

// ── a–c: ручная команда (мнемосхема) ─────────────────────────────────────────

TEST(SimValves, ManualClickOpensAR2AndSwitchesAccumulatorPhase)
{
    ValveRig rig;
    rig.startProfile("gas_to_accumulator.json");
    ASSERT_EQ(rig.controller.currentPhaseId(), "static_empty");

    rig.valves.setValveState(true, rig.indexOf("AR2"));

    EXPECT_TRUE(rig.valves.valveState("AR2"));
    EXPECT_TRUE(rig.gui("AR2"));
    EXPECT_GE(rig.guiChanged, 1);
    EXPECT_TRUE(rig.controller.isValveOpen("K109"));

    rig.simTick();
    EXPECT_EQ(rig.controller.currentPhaseId(), "gas_inlet");
    EXPECT_EQ(rig.controller.history().last().reason, "valve");
}

TEST(SimValves, ManualClickOpensR3AndSwitchesChamberPhase)
{
    ValveRig rig;
    rig.startProfile("h2_to_chamber.json");

    rig.valves.setValveState(true, rig.indexOf("R3"));

    EXPECT_TRUE(rig.valves.valveState("R3"));
    EXPECT_TRUE(rig.gui("R3"));
    EXPECT_TRUE(rig.controller.isValveOpen("K151"));

    rig.simTick();
    EXPECT_EQ(rig.controller.currentPhaseId(), "h2_inlet");
    EXPECT_EQ(rig.controller.history().last().reason, "valve");
}

TEST(SimValves, InterlockRefusesAR2WhileAR1Open)
{
    ValveRig rig;
    rig.valves.setValveState(true, rig.indexOf("AR1"));
    ASSERT_TRUE(rig.valves.valveState("AR1"));

    rig.valves.setValveState(true, rig.indexOf("AR2"));

    EXPECT_FALSE(rig.valves.valveState("AR2"));
    EXPECT_FALSE(rig.gui("AR2"));
    EXPECT_FALSE(rig.controller.isValveOpen("K109"));
}

// D3: карта Security менялась до записи, откат записи её не возвращал.
TEST(SimValves, FailedWriteDoesNotPoisonInterlock)
{
    auto port = std::make_unique<FakeDoPort>(QVector<bool>(16, false));
    FakeDoPort *raw = port.get();
    ValveRig rig(std::move(port));

    raw->writeOk = false;
    rig.valves.setValveState(true, rig.indexOf("AR1"));
    ASSERT_FALSE(rig.valves.valveState("AR1"));   // запись не прошла — клапан закрыт

    raw->writeOk = true;
    rig.valves.setValveState(true, rig.indexOf("AR2"));
    EXPECT_TRUE(rig.valves.valveState("AR2")) << "AR1 закрыт физически, но Security считает его открытым";
}

// D4: состояние платы при старте не доходило до Security — интерлок обходился.
TEST(SimValves, InterlockSeesValveOpenAtStartup)
{
    QVector<bool> board(16, false);
    board[0] = true;   // AR1 открыт с прошлого запуска
    ValveRig rig(std::make_unique<FakeDoPort>(board));
    ASSERT_TRUE(rig.valves.valveState("AR1"));

    EXPECT_FALSE(rig.valves.setValveFromAction(true, "AR2")) << "AR1 открыт на плате — AR2 обязан быть запрещён";
    EXPECT_FALSE(rig.valves.valveState("AR2"));
}

// D5: readback (confirmValve) приводил модель к плате, но интерлок продолжал
// смотреть на свою устаревшую копию.
TEST(SimValves, InterlockSeesReadbackCorrection)
{
    auto port = std::make_unique<FakeDoPort>(QVector<bool>(16, false));
    FakeDoPort *raw = port.get();
    ValveRig rig(std::move(port));

    raw->setBit(0, true);   // на плате AR1 открыт, модель думает — закрыт
    EXPECT_FALSE(rig.valves.confirmValve("AR1", false));
    ASSERT_TRUE(rig.valves.valveState("AR1"));
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "AR2"));

    raw->setBit(0, false);
    EXPECT_TRUE(rig.valves.confirmValve("AR1", false));
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "AR2"));
}
// D8: без порта valveNameList пуст — setValveState падал на QList::at и обрывал
// процесс целиком. При регрессии запускать отдельно:
// SimIntegrationTests.exe --gtest_filter=SimValves.ManualClickWithoutPort*
TEST(SimValves, ManualClickWithoutPortDoesNotCrash)
{
    ValveRig rig(nullptr, false);
    rig.valves.setValveState(true, rig.indexOf("AR2"));
    EXPECT_FALSE(rig.valves.valveState("AR2"));
    EXPECT_GE(rig.guiChanged, 1);

    rig.valves.setValveState(true, -1);
    rig.valves.setValveState(true, 16);
}

// ── d: «Тест клапанов» ───────────────────────────────────────────────────────

// D1: checkValvePressure отдавал карту состояний, любой закрытый клапан был
// «нарушением», шаг закрывался на первом такте выдержки.
TEST(SimValves, ValveTestHoldsValveForWholeDwell)
{
    ValveRig rig;
    ValveTestConfig cfg;
    cfg.steps = {ValveStepConfig{{"AR2"}, 0, 3, 0}};
    cfg.valveControl = &rig.valves;
    cfg.security = &rig.security;

    ValveTestWorker worker;
    worker.setConfig(cfg);
    int doneCount = 0;
    bool success = false;
    QObject::connect(&worker, &ValveTestWorker::done, &worker, [&](bool ok) { ++doneCount; success = ok; });

    QElapsedTimer t;
    t.start();
    qint64 openMs = 0, last = 0;
    worker.start();
    spin(8000, [&] { return doneCount > 0; }, [&] {
        const qint64 now = t.elapsed();
        if (rig.valves.valveState("AR2"))
            openMs += now - last;
        last = now;
    });

    EXPECT_EQ(doneCount, 1);
    EXPECT_TRUE(success);
    EXPECT_GE(openMs, 2500) << "AR2 был открыт только " << openMs << " мс из 3000";
    EXPECT_FALSE(rig.valves.valveState("AR2"));
}

// D2: после нарушения автомат продолжал шаги (открывал следующий) и
// перезапускал повтор; done приходил повторно, клапан следующего шага
// оставался открытым. Нарушение — настоящее: S4 открыт, давление на DD312
// поднимается выше порога закрытия 1,8 бар посреди выдержки.
TEST(SimValves, ValveTestViolationStopsWithoutOpeningNextStep)
{
    ValveRig rig;
    ValveTestConfig cfg;
    cfg.steps = {ValveStepConfig{{"S4"}, 0, 3, 0}, ValveStepConfig{{"R3"}, 0, 3, 0}};
    cfg.valveControl = &rig.valves;
    cfg.security = &rig.security;

    QTimer softEvent;   // такт Grams::softEvent
    QObject::connect(&softEvent, &QTimer::timeout, &softEvent, [&] { rig.feedPressure(); });
    softEvent.start(100);
    rig.feedPressure();

    ValveTestWorker worker;
    worker.setConfig(cfg);
    int doneCount = 0;
    bool success = true;
    QStringList openAtDone;
    QObject::connect(&worker, &ValveTestWorker::done, &worker, [&](bool ok) {
        if (++doneCount == 1) {
            success = ok;
            for (const auto &v : rig.valveObjects)
                if (v->getState())
                    openAtDone << v->m_name;
        }
    });

    bool s4Opened = false, r3Opened = false;
    QElapsedTimer t;
    t.start();
    worker.start();
    spin(4500, [] { return false; }, [&] {
        s4Opened = s4Opened || rig.valves.valveState("S4");
        r3Opened = r3Opened || rig.valves.valveState("R3");
        if (t.elapsed() > 1300)
            rig.dd312 = 1.9;
    });

    EXPECT_TRUE(s4Opened);
    EXPECT_FALSE(r3Opened) << "после аварии шага 0 открыт клапан шага 1";
    EXPECT_EQ(doneCount, 1);
    EXPECT_FALSE(success);
    EXPECT_TRUE(openAtDone.isEmpty()) << openAtDone.join(',').toStdString();
    for (const auto &v : rig.valveObjects)
        EXPECT_FALSE(v->getState()) << v->m_name.toStdString();
}

// ── D6: давления в Security ──────────────────────────────────────────────────

TEST(SimValves, PressureRuleValveRefusedWithoutPressure)
{
    ValveRig rig;   // setPressureMap ещё не вызывался
    for (const char *valve : {"S4", "R4", "AR4"}) {
        EXPECT_FALSE(rig.valves.setValveFromAction(true, valve)) << valve;
        EXPECT_EQ(rig.valves.lastRefusal(), "pressure_invalid") << valve;
    }
    // Клапаны без правила по давлению не затронуты.
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "R3"));
    EXPECT_TRUE(rig.valves.lastRefusal().isEmpty());
}

TEST(SimValves, RangeValveOpensOnlyBelowClosePressure)
{
    ValveRig rig;
    rig.dd311 = 1.9;   // S4 закрыт — давление квартиля по DD311
    rig.feedPressure();
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "S4"));
    EXPECT_EQ(rig.valves.lastRefusal(), "pressure_range");

    rig.dd311 = 1.0;
    rig.feedPressure();
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "S4"));
    EXPECT_TRUE(rig.valves.setValveFromAction(false, "S4"));   // закрыть можно всегда
}

TEST(SimValves, StalePressureBecomesInvalidAfterConfiguredTicks)
{
    ValveRig rig;
    rig.feedPressure();
    for (int i = 0; i < 4; ++i)
        rig.feedPressure(false);   // 4 такта без нового отсчёта — ещё достоверно
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "R4"));
    EXPECT_TRUE(rig.valves.setValveFromAction(false, "R4"));

    rig.feedPressure(false);       // 5-й — нет
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "R4"));
    EXPECT_EQ(rig.valves.lastRefusal(), "pressure_invalid");

    rig.feedPressure();            // отсчёт пришёл — снова достоверно
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "R4"));
}

TEST(SimValves, NanOrOutOfSensorRangeIsInvalid)
{
    ValveRig rig;
    rig.dd331 = std::nan("");
    rig.feedPressure();
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "R4"));
    EXPECT_EQ(rig.valves.lastRefusal(), "pressure_invalid");

    rig.dd331 = 60.0;   // DD331: 0…50 бар (3,8…20,5 мА → −0,6…51,6 бар)
    rig.feedPressure();
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "R4"));
    EXPECT_EQ(rig.valves.lastRefusal(), "pressure_invalid");

    rig.dd331 = -0.02;  // шум у нуля — в пределах NE43
    rig.feedPressure();
    EXPECT_TRUE(rig.valves.setValveFromAction(true, "R4"));
}

TEST(SimValves, OpenValveWithInvalidPressureWarnsOnceAndStaysOpen)
{
    ValveRig rig;
    rig.feedPressure();
    ASSERT_TRUE(rig.valves.setValveFromAction(true, "S4"));

    rig.dd312 = std::nan("");
    rig.feedPressure();
    PressureCheck first = rig.security.checkPressure(rig.valves.valveStates());
    EXPECT_TRUE(first.ok());
    ASSERT_EQ(first.warnings.size(), 1);
    EXPECT_EQ(first.warnings.first().valve, "S4");
    EXPECT_EQ(first.warnings.first().reason, "pressure_invalid");
    EXPECT_TRUE(rig.security.checkPressure(rig.valves.valveStates()).warnings.isEmpty()) << "повтор предупреждения";
    EXPECT_TRUE(rig.valves.valveState("S4"));

    rig.dd312 = 1.9;    // снова достоверно, но выше порога — нарушение
    rig.feedPressure();
    PressureCheck violated = rig.security.checkPressure(rig.valves.valveStates());
    ASSERT_EQ(violated.violations.size(), 1);
    EXPECT_EQ(violated.violations.first().reason, "pressure_range");
}
// ── e: RegimeWorkerBase (Режим в / г) ────────────────────────────────────────

TEST(SimValves, RegimeExecutionNotAbortedByClosedValves)
{
    ValveRig rig;
    RegimeWorkerConfig cfg;
    cfg.maxTimeSec = 3;
    cfg.tickIntervalMs = 20;
    cfg.security = &rig.security;
    cfg.valveControl = &rig.valves;

    NoopRegimeWorker worker;
    worker.setConfig(cfg);
    int doneCount = 0;
    bool success = false;
    QObject::connect(&worker, &RegimeWorkerBase::done, &worker, [&](bool ok) { ++doneCount; success = ok; });
    worker.start();
    spin(3000, [&] { return doneCount > 0; });

    EXPECT_EQ(doneCount, 1);
    EXPECT_TRUE(success) << "Execution прерван «нарушением давления» при закрытых клапанах";
}

// D1 без таймеров: checkValvePressure отдавал карту состояний, и при всех
// закрытых клапанах «нарушения» были у 14 клапанов. Закрытый клапан — не нарушение.
TEST(SimValves, PressureCheckReportsNoViolationWhenIdle)
{
    ValveRig rig;
    const PressureCheck idle = rig.security.checkPressure(rig.valves.valveStates());
    EXPECT_TRUE(idle.ok());
    EXPECT_TRUE(idle.violations.isEmpty());

    rig.valves.setValveFromAction(true, "AR2");
    rig.valves.setValveFromAction(true, "R3");
    EXPECT_TRUE(rig.security.checkPressure(rig.valves.valveStates()).ok());
}

// ── f: контракт демо «фронт, не уровень» (S1) ────────────────────────────────

TEST(SimValves, ValveOpenedBeforeRunDoesNotSwitchPhase)
{
    ValveRig rig;
    rig.valves.setValveState(true, rig.indexOf("AR2"));
    ASSERT_TRUE(rig.controller.isValveOpen("K109"));

    rig.startProfile("gas_to_accumulator.json");
    rig.simTick();
    rig.simTick();
    EXPECT_EQ(rig.controller.currentPhaseId(), "static_empty") << "уровень клапана переход не запускает";

    rig.valves.setValveState(false, rig.indexOf("AR2"));
    rig.valves.setValveState(true, rig.indexOf("AR2"));
    rig.simTick();
    EXPECT_EQ(rig.controller.currentPhaseId(), "gas_inlet");
}
