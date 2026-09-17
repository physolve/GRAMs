// Клапаны на демо-данных: щелчок, интерлоки, «Тест клапанов», проверка давления.
//
// Путь тот же, что в Grams, и в том же порядке инициализации:
//   ValveControl::setValvePointers → setSafeModuleInitialValveState → setDoPort
//   → Grams::initSafeModule (constructValveMap, интерлоки, пороги из профиля).
//
// Тесты с префиксом DISABLED_ воспроизводят найденные дефекты и падают на
// текущем коде; каждое исправление снимает префикс со своего теста. Прогнать
// их сейчас: SimIntegrationTests.exe --gtest_also_run_disabled_tests
// --gtest_filter=SimValves*

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
        // Grams::advDoController: начальные состояния — до установки порта.
        valves.setSafeModuleInitialValveState();
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
        security.constructValveMap(input.valveCodes);
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

// D3: карта Security меняется до записи, откат записи её не возвращает.
TEST(SimValves, DISABLED_FailedWriteDoesNotPoisonInterlock)
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

// D4: состояние платы при старте не доходит до Security — интерлок обходится.
TEST(SimValves, DISABLED_InterlockSeesValveOpenAtStartup)
{
    QVector<bool> board(16, false);
    board[0] = true;   // AR1 открыт с прошлого запуска
    ValveRig rig(std::make_unique<FakeDoPort>(board));
    ASSERT_TRUE(rig.valves.valveState("AR1"));

    EXPECT_FALSE(rig.valves.setValveFromAction(true, "AR2")) << "AR1 открыт на плате — AR2 обязан быть запрещён";
    EXPECT_FALSE(rig.valves.valveState("AR2"));
}

// D8: без порта valveNameList пуст, setValveState падает на QList::at.
// Тест обрывает процесс целиком — запускать отдельно.
TEST(SimValves, DISABLED_ManualClickWithoutPortDoesNotCrash)
{
    ValveRig rig(nullptr, false);
    rig.valves.setValveState(true, rig.indexOf("AR2"));
    EXPECT_FALSE(rig.valves.valveState("AR2"));
}

// ── d: «Тест клапанов» ───────────────────────────────────────────────────────

// D1: checkValvePressure отдаёт карту состояний, любой закрытый клапан —
// «нарушение», шаг закрывается на первом такте выдержки.
TEST(SimValves, DISABLED_ValveTestHoldsValveForWholeDwell)
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

// D2: после нарушения автомат продолжает шаги (открывает следующий) и
// перезапускает повтор; done приходит повторно.
TEST(SimValves, DISABLED_ValveTestViolationStopsWithoutOpeningNextStep)
{
    ValveRig rig;
    ValveTestConfig cfg;
    cfg.steps = {ValveStepConfig{{"AR2"}, 0, 3, 0}, ValveStepConfig{{"R3"}, 0, 3, 0}};
    cfg.valveControl = &rig.valves;
    cfg.security = &rig.security;   // на текущем коде нарушение наступает всегда

    ValveTestWorker worker;
    worker.setConfig(cfg);
    int doneCount = 0;
    QObject::connect(&worker, &ValveTestWorker::done, &worker, [&](bool) { ++doneCount; });

    bool r3Opened = false;
    QStringList openAtDone;
    QObject::connect(&worker, &ValveTestWorker::done, &worker, [&](bool) {
        if (doneCount == 1)
            for (const auto &v : rig.valveObjects)
                if (v->getState())
                    openAtDone << v->m_name;
    });
    worker.start();
    spin(4000, [] { return false; }, [&] { r3Opened = r3Opened || rig.valves.valveState("R3"); });

    EXPECT_FALSE(r3Opened) << "после аварии шага 0 открыт клапан шага 1";
    EXPECT_EQ(doneCount, 1);
    EXPECT_TRUE(openAtDone.isEmpty()) << openAtDone.join(',').toStdString();
    for (const auto &v : rig.valveObjects)
        EXPECT_FALSE(v->getState()) << v->m_name.toStdString();
}

// ── e: RegimeWorkerBase (Режим в / г) ────────────────────────────────────────

TEST(SimValves, DISABLED_RegimeExecutionNotAbortedByClosedValves)
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

// Прямое доказательство D1 без таймеров: при всех закрытых клапанах и без
// давлений «нарушений» больше десяти.
TEST(SimValves, DISABLED_PressureCheckReportsNoViolationWhenIdle)
{
    ValveRig rig;
    const QMap<QString, bool> map = rig.security.checkValvePressure();
    QStringList falseKeys;
    for (auto it = map.cbegin(); it != map.cend(); ++it)
        if (!it.value())
            falseKeys << it.key();
    EXPECT_TRUE(falseKeys.isEmpty()) << falseKeys.join(',').toStdString();
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
