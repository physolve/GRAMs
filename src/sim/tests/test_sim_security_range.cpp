// Security по давлению для клапанов диапазона S4 (К171) и R4 (К173).
//
// Этап 5.5 проверки связки: без режима S4/R4, открытые щелчком на 1,01 бар,
// оставались открытыми на 1,95 бар — Security::checkPressure вызывали только
// воркеры режимов. Путь здесь тот же, что в GRAMs --sim:
//
//   SimController (профиль, Па) → SimSensorSource → DataAcquisition
//   → ControllerData (бар) → зеркало Grams::softEvent → Security
//   → ValveControl → порт (SimValveEcho или журнал записей)
//
// Режимы — через настоящий RegimeTaskTree, как из RunTable.
//
// Тесты с префиксом DISABLED_ воспроизводят дефект и падают до исправления
// (запуск: --gtest_also_run_disabled_tests); коммит исправления снимает
// префикс со своих тестов.

#include "soft_event_mirror.h"
#include "test_support.h"

#include "core/SimClock.h"
#include "core/SimController.h"
#include "core/SimProfile.h"
#include "hw/SimSensorSource.h"
#include "hw/SimValveEcho.h"

#include "DataAcquisition.h"
#include "DataCollection.h"
#include "Security.h"
#include "ValveControl.h"
#include "ValveModel.h"
#include "actions/RegimeTaskTree.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>

using namespace sim;
using namespace simtest;

namespace {

// ── Журнал grams.security ────────────────────────────────────────────────────
//
// Причины (pressure_range_autoclose, unauthorized_open, transfer_equilibrium)
// оператор и разработчик видят в логе grams.security — его и проверяем.

QStringList &securityLog()
{
    static QStringList lines;
    return lines;
}

QtMessageHandler g_previousHandler = nullptr;

void captureSecurity(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (ctx.category && qstrcmp(ctx.category, "grams.security") == 0)
        securityLog() << msg;
    if (g_previousHandler)
        g_previousHandler(type, ctx, msg);
}

int logCount(const QString &needle, const QString &also = {})
{
    return int(std::count_if(securityLog().cbegin(), securityLog().cend(), [&](const QString &l) {
        return l.contains(needle) && (also.isEmpty() || l.contains(also));
    }));
}

std::string dumpLog()
{
    return securityLog().join(u'\n').toStdString();
}

// ── Порт с журналом записей ──────────────────────────────────────────────────
//
// Порядок команд на плате (закрыть R4 ДО открытия R3) и клапан, открытый мимо
// программы (с прошлого запуска или вручную на стенде).
class RecordingPort final : public IDoPort {
public:
    explicit RecordingPort(QVector<bool> initial) : m_data(std::move(initial)) {}
    QList<QVector<bool>> writes;
    bool write(const QVector<bool> &states) override
    {
        writes << states;
        m_data = states;
        return true;
    }
    bool refresh() override { return true; }
    QVector<bool> data() override { return m_data; }
    void setBit(int i, bool v) { m_data[i] = v; }
    bool bit(int i) const { return m_data.value(i); }

private:
    QVector<bool> m_data;
};

void settle()
{
    for (int i = 0; i < 20; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents);
}

bool spin(int timeoutMs, const std::function<bool()> &pred, const std::function<void()> &sample = {})
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (sample)
            sample();
        if (pred())
            return true;
    }
    return pred();
}

// Объёмы GRAM50 (profile/addons.json), см³: накопитель B, за S4 — D1;
// реакционная область E, за R4 — D2.
struct Volumes {
    double B = 0, D1 = 0, E = 0, D2 = 0;
};

Volumes gram50Volumes()
{
    const QJsonObject addons = readJson(QStringLiteral(GRAMS_ADDONS_JSON)).object();
    const QJsonObject s = addons.value("storageQuar").toObject().value("volume").toObject();
    const QJsonObject r = addons.value("reactionQuar").toObject().value("volume").toObject();
    return {s.value("B").toDouble(), s.value("D1").toDouble(), r.value("E").toDouble(), r.value("D2").toDouble()};
}

// ── Установка GRAM50 на демо-данных ──────────────────────────────────────────

class RangeRig {
public:
    ManualClock clock;
    SimController controller{gram50().catalog, &clock};
    DataAcquisition daq;
    ControllerData time;
    QList<std::shared_ptr<ControllerData>> pressure;
    QList<std::shared_ptr<ControllerData>> temperature;
    QList<std::shared_ptr<FilterData>> filters;
    DataCollection dv301;
    DataCollection dv302;

    Security security;
    ValveControl valves;
    QList<std::shared_ptr<Valve>> valveObjects;
    Valve chamberValve;
    RecordingPort *port = nullptr;   // nullptr — эхо демо-режима
    int guiChanged = 0;

    // Показание поверх датчика, бар: недостоверный (отказ токовой петли) или
    // запертый узкодиапазонный датчик. Демо так не умеет: канал ограничен
    // шкалой датчика (SimCatalog), и выше 2 бар DD312 просто стоит на 2,0.
    QMap<QString, double> override;

    explicit RangeRig(std::unique_ptr<RecordingPort> recording = nullptr)
    {
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
        daq.setSensorSource(std::make_unique<SimSensorSource>(&controller, gram50().bindings));
        daq.setTempPointers(temp);
        daq.markControllersConnected();
        daq.setVacuumPointer(&dv301);
        daq.setTurboVacuumPointer(&dv302);

        // Grams::advDoController → Grams::initSafeModule → хвост конструктора.
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
        if (recording) {
            port = recording.get();
            valves.setDoPort(std::move(recording));
        } else {
            QStringList ids;
            for (const auto &code : input.valveCodes)
                ids << gram50().catalog.valveByCode(code)->id;
            valves.setDoPort(std::make_unique<SimValveEcho>(ids, &controller));
        }
        QObject::connect(&valves, &ValveControl::guiValsValveChanged, &valves, [this] { ++guiChanged; });
        initSafeModuleMirror(security, readJson(QStringLiteral(GRAMS_PROFILE_JSON)).object()
                                           .value("GRAM50").toObject(), input.pressureCard);
        gramsStartup(security, valves);
    }

    ControllerData *sensor(const QString &name)
    {
        for (const auto &p : pressure)
            if (p->m_name == name)
                return p.get();
        return nullptr;
    }

    SensorReading reading(const QString &name)
    {
        const ControllerData *s = sensor(name);
        return {name, override.value(name, s->getCurValue()), s->sampleCount()};
    }

    // Показание, которое видит Security (с учётом подмены).
    double seen(const QString &name) { return reading(name).bar; }

    void setBar(const QString &sensorName, double bar)
    {
        controller.setValue(QStringLiteral("P.") + sensorName, bar * 1e5);
    }

    // Опрос DataAcquisition и такт softEvent. Давление приходит в
    // ControllerData со следующего опроса, как у платы.
    void softEvent()
    {
        QMetaObject::invokeMethod(&daq, "processEvents", Qt::DirectConnection);
        softEventSecurity(security, valves, {reading("DD311"), reading("DD312"),
                                             reading("DD331"), reading("DD332")});
    }

    // Такт softEvent без новых отсчётов: плата не отвечает.
    void softEventWithoutSamples()
    {
        softEventSecurity(security, valves, {reading("DD311"), reading("DD312"),
                                             reading("DD331"), reading("DD332")});
    }

    // 500 мс установки: демо-время, профиль, опрос, softEvent.
    void tick(double sec = 0.5)
    {
        clock.advance(sec);
        controller.tick();
        settle();
        softEvent();
    }

    void ticks(int n)
    {
        for (int i = 0; i < n; ++i)
            tick();
    }

    void startProfile(const char *file)
    {
        Profile profile;
        const auto v = parseProfile(readJson(profilePath(QString::fromLatin1(file))).object(),
                                    gram50().catalog, &profile);
        ASSERT_TRUE(v.ok());
        ASSERT_TRUE(controller.run(profile).ok());
        settle();
    }

    void gotoPhase(const char *phase)
    {
        ASSERT_TRUE(controller.gotoPhase(controller.runId(), QString::fromLatin1(phase)).ok());
        settle();
    }

    int indexOf(const QString &code) const
    {
        for (int i = 0; i < valveObjects.size(); ++i)
            if (valveObjects[i]->m_name == code)
                return i;
        return -1;
    }

    // Щелчок на мнемосхеме (MnemoBase: setValveState(!open, индекс)).
    void click(const QString &code) { valves.setValveState(!valves.valveState(code), indexOf(code)); }

    bool open(const QString &code) const { return valves.valveState(code); }
    bool gui(const QString &code) const { return valves.getGuiValsValve().value(code).toBool(); }

    // Состояние на «плате»: эхо демо-режима (sim.status.valves) или журнал.
    bool board(const QString &code) const
    {
        if (port)
            return port->bit(indexOf(code));
        return controller.isValveOpen(gram50().catalog.valveByCode(code)->id);
    }
};

// ── RunTable → RegimeTaskTree → воркер ───────────────────────────────────────
//
// Журнал режимов пишется во временную папку: RegimeTaskTree открывает
// data/regime_log.db относительно текущей папки в конструкторе.
class RegimeHarness {
public:
    QTemporaryDir dir;
    RegimeManager manager;
    std::unique_ptr<RegimeTaskTree> tree;

    RegimeHarness(RangeRig &rig, const QVariantList &steps)
    {
        const QString cwd = QDir::currentPath();
        QDir::setCurrent(dir.path());
        tree = std::make_unique<RegimeTaskTree>();
        QDir::setCurrent(cwd);

        Regime regime;
        regime.m_name = QStringLiteral("Тест клапанов");
        regime.m_repeatCount = 1;
        regime.m_condition.type = QStringLiteral("time");
        regime.m_condition.time = 0;
        manager.model()->setRegimes({regime});

        tree->setRegimeManager(&manager);
        tree->setValveControl(&rig.valves);
        tree->setDataAcquisition(&rig.daq);
        tree->setSecurity(&rig.security);
        tree->setValveTestSteps(steps, 1, 0, 0);
    }

    ~RegimeHarness()
    {
        tree.reset();
        QSqlDatabase::removeDatabase(QStringLiteral("security_range_reader"));
    }

    // События журнала режима по типу (RegimeLogger::k*).
    QStringList events(const char *type)
    {
        QStringList out;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                        QStringLiteral("security_range_reader"));
            db.setDatabaseName(dir.filePath(QStringLiteral("data/regime_log.db")));
            if (!db.open())
                return out;
            QSqlQuery q(db);
            q.prepare(QStringLiteral("SELECT detail FROM regime_events WHERE event_type = ?"));
            q.addBindValue(QString::fromLatin1(type));
            q.exec();
            while (q.next())
                out << q.value(0).toString();
            db.close();
        }
        return out;
    }
};

QVariantMap step(const QStringList &valves, int dwellSec)
{
    return {{"valves", valves}, {"dwell", dwellSec}, {"pauseBefore", 0}, {"pauseAfter", 0}};
}

class SimSecurityRange : public ::testing::Test {
protected:
    void SetUp() override
    {
        securityLog().clear();
        g_previousHandler = qInstallMessageHandler(captureSecurity);
    }
    void TearDown() override { qInstallMessageHandler(g_previousHandler); }
};

// Щелчок S4, R4 на атмосфере и рост до 1,95 бар по профилю security_range.
// Возвращает такты: когда Security впервые увидел > 1,8 и когда клапан закрылся.
struct RiseResult {
    int crossS4 = -1, closedS4 = -1, crossR4 = -1, closedR4 = -1;
};

RiseResult openAtAtmosphereAndRise(RangeRig &rig)
{
    RiseResult r;
    rig.startProfile("security_range.json");
    rig.ticks(2);
    EXPECT_NEAR(rig.seen("DD311"), 1.01325, 1e-6);
    rig.click("S4");
    rig.click("R4");
    EXPECT_TRUE(rig.open("S4")) << dumpLog();
    EXPECT_TRUE(rig.open("R4")) << dumpLog();
    EXPECT_TRUE(rig.board("S4"));
    EXPECT_TRUE(rig.board("R4"));

    rig.gotoPhase("rise");
    for (int i = 0; i < 140; ++i) {   // 70 с: рост 60 с и фаза high
        rig.tick();
        const double storage = std::max(rig.seen("DD311"), rig.seen("DD312"));
        const double reaction = std::max(rig.seen("DD331"), rig.seen("DD332"));
        if (r.crossS4 < 0 && storage > 1.8) r.crossS4 = i;
        if (r.crossR4 < 0 && reaction > 1.8) r.crossR4 = i;
        if (r.closedS4 < 0 && !rig.open("S4")) r.closedS4 = i;
        if (r.closedR4 < 0 && !rig.open("R4")) r.closedR4 = i;
    }
    return r;
}

} // namespace

// ── a: без режима — автозакрытие выше 1,8 бар ────────────────────────────────

TEST_F(SimSecurityRange, ManualOpenClosedAboveThresholdWithoutRegime)
{
    RangeRig rig;
    const RiseResult r = openAtAtmosphereAndRise(rig);

    ASSERT_GE(r.crossS4, 0) << "давление накопителя не превысило 1,8 бар";
    ASSERT_GE(r.crossR4, 0) << "давление реакционной области не превысило 1,8 бар";
    EXPECT_GE(r.closedS4, 0) << "S4 открыт на " << rig.seen("DD312") << " бар\n" << dumpLog();
    EXPECT_GE(r.closedR4, 0) << "R4 открыт на " << rig.seen("DD332") << " бар\n" << dumpLog();
    // Не раньше пересечения (ложное закрытие) и не позже следующего такта.
    EXPECT_GE(r.closedS4, r.crossS4);
    EXPECT_LE(r.closedS4, r.crossS4 + 1);
    EXPECT_GE(r.closedR4, r.crossR4);
    EXPECT_LE(r.closedR4, r.crossR4 + 1);

    // Одно и то же состояние везде: модель, мнемосхема/Lumber, плата (эхо).
    for (const char *v : {"S4", "R4"}) {
        EXPECT_FALSE(rig.open(v)) << v;
        EXPECT_FALSE(rig.gui(v)) << v;
        EXPECT_FALSE(rig.board(v)) << v;
    }
    EXPECT_GE(logCount("pressure_range_autoclose", "S4"), 1) << dumpLog();
    EXPECT_GE(logCount("pressure_range_autoclose", "R4"), 1) << dumpLog();
}

// ── b: спуск после автозакрытия — сам не открывается ─────────────────────────

TEST_F(SimSecurityRange, AutoClosedValveStaysClosedWhenPressureFalls)
{
    RangeRig rig;
    const RiseResult r = openAtAtmosphereAndRise(rig);
    ASSERT_GE(r.closedS4, 0) << "предусловие: S4 закрыт автоматически";
    ASSERT_GE(r.closedR4, 0) << "предусловие: R4 закрыт автоматически";

    bool reopened = false;
    rig.gotoPhase("hyst");
    for (int i = 0; i < 70; ++i) {
        rig.tick();
        reopened = reopened || rig.open("S4") || rig.open("R4");
    }
    rig.gotoPhase("low");
    for (int i = 0; i < 70; ++i) {
        rig.tick();
        reopened = reopened || rig.open("S4") || rig.open("R4");
    }
    ASSERT_LT(rig.seen("DD311"), 1.6);
    EXPECT_FALSE(reopened) << "Security открыл клапан сам";
    EXPECT_FALSE(rig.board("S4"));
    EXPECT_FALSE(rig.board("R4"));

    // Новая команда оператора ниже порога открытия — открывает.
    rig.click("S4");
    EXPECT_TRUE(rig.open("S4")) << dumpLog();
}

// ── В3: зона гистерезиса 1,6–1,8 — открытие запрещено ───────────────────────

TEST_F(SimSecurityRange, RangeValveRefusedInHysteresisZone)
{
    RangeRig rig;
    for (const char *s : {"DD311", "DD312", "DD331", "DD332"})
        rig.setBar(s, 1.7);
    rig.ticks(2);
    ASSERT_NEAR(rig.seen("DD311"), 1.7, 1e-6);

    rig.click("S4");
    EXPECT_FALSE(rig.open("S4")) << "S4 открыт в зоне 1,6–1,8 бар";
    EXPECT_EQ(rig.valves.lastRefusal(), "pressure_range");
    EXPECT_FALSE(rig.valves.setValveFromAction(true, "R4")) << "R4 открыт в зоне 1,6–1,8 бар";

    rig.valves.setValveState(false, rig.indexOf("S4"));
    for (const char *s : {"DD311", "DD312", "DD331", "DD332"})
        rig.setBar(s, 1.55);
    rig.ticks(2);
    rig.valves.setValveState(true, rig.indexOf("S4"));
    EXPECT_TRUE(rig.open("S4"));
}

// ── c: «Тест клапанов» — закрывает Security, одно нарушение, шаг не идёт дальше

TEST_F(SimSecurityRange, ValveTestStepClosedBySecurityOnce)
{
    RangeRig rig;
    rig.ticks(2);
    RegimeHarness h(rig, {step({"S4"}, 5), step({"R3"}, 2)});

    QTimer softEvent;   // такт Grams::softEvent (ускоренный)
    QObject::connect(&softEvent, &QTimer::timeout, &softEvent, [&] { rig.tick(); });
    softEvent.start(100);

    h.tree->startAll();
    ASSERT_TRUE(spin(3000, [&] { return rig.open("S4"); })) << dumpLog();

    for (const char *s : {"DD311", "DD312", "DD331", "DD332"})
        rig.setBar(s, 1.95);
    bool r3Opened = false;
    spin(8000, [&] { return !h.tree->isRunning(); }, [&] { r3Opened = r3Opened || rig.open("R3"); });
    softEvent.stop();

    EXPECT_FALSE(h.tree->isRunning());
    EXPECT_FALSE(rig.open("S4"));
    EXPECT_FALSE(r3Opened) << "после нарушения открыт клапан следующего шага";
    EXPECT_EQ(h.events("security_violation").size(), 1) << h.events("security_violation").join('\n').toStdString();
    // Закрыл Security (на такте softEvent), а не воркер на своём такте.
    EXPECT_GE(logCount("pressure_range_autoclose", "S4"), 1) << dumpLog();
}

// ── d2: «Тест клапанов» остановлен «Стоп» во время выдержки ──────────────────

TEST_F(SimSecurityRange, DISABLED_StoppedValveTestLeavesRangeValveClosed)
{
    RangeRig rig;
    rig.ticks(2);
    RegimeHarness h(rig, {step({"S4"}, 30)});

    QTimer softEvent;
    QObject::connect(&softEvent, &QTimer::timeout, &softEvent, [&] { rig.tick(); });
    softEvent.start(100);

    h.tree->startAll();
    ASSERT_TRUE(spin(3000, [&] { return rig.open("S4"); })) << dumpLog();
    h.tree->stop();
    spin(600, [] { return false; });
    softEvent.stop();

    EXPECT_FALSE(rig.open("S4")) << "режим остановлен, S4 остался открытым";
    EXPECT_FALSE(rig.board("S4"));
    EXPECT_GE(logCount("unauthorized_open", "S4"), 1) << dumpLog();
}

// ── e: S4 открыт на плате при старте GRAMs ───────────────────────────────────

TEST_F(SimSecurityRange, DISABLED_RangeValveOpenOnBoardAtStartupIsClosed)
{
    const int s4 = gram50Input().valveCodes.indexOf("S4");
    QVector<bool> board(16, false);
    board[s4] = true;   // с прошлого запуска
    RangeRig rig(std::make_unique<RecordingPort>(board));

    EXPECT_FALSE(rig.open("S4")) << "после инициализации S4 открыт";
    EXPECT_FALSE(rig.board("S4"));
    EXPECT_GE(logCount("unauthorized_open", "startup"), 1) << dumpLog();
}

// ── e2: readback нашёл S4 открытым без команды ───────────────────────────────

TEST_F(SimSecurityRange, DISABLED_ReadbackFindsUncommandedRangeValveOpen)
{
    auto port = std::make_unique<RecordingPort>(QVector<bool>(16, false));
    RecordingPort *raw = port.get();
    RangeRig rig(std::move(port));
    rig.ticks(2);

    raw->setBit(rig.indexOf("S4"), true);   // переключён мимо программы
    // REQ-082: результат confirmValve — только про запрошенный клапан.
    EXPECT_TRUE(rig.valves.confirmValve("AR6", false));
    EXPECT_FALSE(rig.open("S4"));
    EXPECT_FALSE(rig.board("S4")) << "S4 открыт на плате без команды";
    EXPECT_GE(logCount("unauthorized_open", "confirmValve"), 1) << dumpLog();
}

// ── f: недостоверное давление у открытого S4 (D6) ────────────────────────────

TEST_F(SimSecurityRange, StalePressureDoesNotCloseOpenRangeValve)
{
    RangeRig rig;
    rig.ticks(2);
    rig.click("S4");
    ASSERT_TRUE(rig.open("S4"));

    for (int i = 0; i < 10; ++i)   // больше pressureStaleTicks (4)
        rig.softEventWithoutSamples();
    EXPECT_TRUE(rig.open("S4")) << "S4 закрыт по одной недостоверности давления";
    EXPECT_TRUE(rig.board("S4"));
}

TEST_F(SimSecurityRange, StalePressureWarnsOnceWithoutRegime)
{
    RangeRig rig;
    rig.ticks(2);
    rig.click("S4");
    ASSERT_TRUE(rig.open("S4"));

    for (int i = 0; i < 10; ++i)
        rig.softEventWithoutSamples();
    EXPECT_TRUE(rig.open("S4"));
    EXPECT_EQ(logCount("недостоверно", "S4"), 1) << dumpLog();
}

// ── h: прогноз равновесия через К151/К153/К155 ───────────────────────────────

// R4 открыт, накопитель заряжен: команда открыть R3 (К151) сначала закрывает
// R4 отдельной записью, затем открывает R3.
TEST_F(SimSecurityRange, DISABLED_TransferOpenClosesReactionRangeValveFirst)
{
    auto port = std::make_unique<RecordingPort>(QVector<bool>(16, false));
    RecordingPort *raw = port.get();
    RangeRig rig(std::move(port));
    rig.startProfile("security_transfer.json");
    rig.ticks(2);
    ASSERT_NEAR(rig.seen("DD311"), 30.0, 1e-6);
    ASSERT_NEAR(rig.seen("DD331"), 0.05, 1e-6);

    rig.click("R4");
    ASSERT_TRUE(rig.open("R4"));
    rig.ticks(2);

    const int r3 = rig.indexOf("R3"), r4 = rig.indexOf("R4");
    const int before = int(raw->writes.size());
    rig.click("R3");

    EXPECT_TRUE(rig.open("R3"));
    EXPECT_FALSE(rig.open("R4")) << "R4 открыт при перепуске 30 → 0,05 бар";
    ASSERT_GE(raw->writes.size(), before + 2) << "нет отдельной записи закрытия R4";
    const QVector<bool> &closeR4 = raw->writes.at(before);
    EXPECT_FALSE(closeR4[r4]);
    EXPECT_FALSE(closeR4[r3]) << "R3 открыт в одной записи с закрытием R4";
    EXPECT_TRUE(raw->writes.last()[r3]);
    EXPECT_FALSE(raw->writes.last()[r4]);
    EXPECT_GE(logCount("transfer_equilibrium", "R4"), 1) << dumpLog();
}

// pравн ниже порога — R4 остаётся открытым.
TEST_F(SimSecurityRange, TransferBelowLimitKeepsRangeValveOpen)
{
    const Volumes v = gram50Volumes();
    RangeRig rig;
    rig.setBar("DD311", 2.5);
    for (const char *s : {"DD331", "DD332"})
        rig.setBar(s, 0.05);
    rig.ticks(2);
    const double pEq = (2.5 * v.B + 0.05 * (v.E + v.D2)) / (v.B + v.E + v.D2);
    ASSERT_LT(pEq, 1.8);

    rig.click("R4");
    ASSERT_TRUE(rig.open("R4"));
    rig.ticks(2);
    rig.click("R3");
    rig.ticks(4);
    EXPECT_TRUE(rig.open("R3"));
    EXPECT_TRUE(rig.open("R4")) << "pравн = " << pEq << " бар";
}

// Натекатель R1 (К153) и R4 открыты; накопитель зарядили — R4 закрывается на
// такте, хотя реакционная область ещё не поднялась.
TEST_F(SimSecurityRange, DISABLED_ChargedStorageClosesReactionRangeValveOnTick)
{
    RangeRig rig;
    rig.setBar("DD311", 1.0);
    for (const char *s : {"DD331", "DD332"})
        rig.setBar(s, 0.05);
    rig.ticks(2);
    rig.click("R4");
    rig.click("R1");
    ASSERT_TRUE(rig.open("R4"));
    ASSERT_TRUE(rig.open("R1"));

    rig.setBar("DD311", 30.0);
    rig.ticks(3);
    EXPECT_LT(rig.seen("DD332"), 1.0);
    EXPECT_FALSE(rig.open("R4")) << "R4 открыт: накопитель 30 бар, натекатель открыт";
    EXPECT_TRUE(rig.open("R1"));
    EXPECT_GE(logCount("transfer_equilibrium", "R4"), 1) << dumpLog();
}

// Зеркально: S4 и R3 открыты, давление в реакционной области поднялось.
TEST_F(SimSecurityRange, DISABLED_HighReactionPressureClosesStorageRangeValveOnTick)
{
    RangeRig rig;
    for (const char *s : {"DD311", "DD312", "DD331"})
        rig.setBar(s, 1.0);
    rig.ticks(2);
    rig.click("S4");
    rig.click("R3");
    ASSERT_TRUE(rig.open("S4"));
    ASSERT_TRUE(rig.open("R3"));

    rig.setBar("DD331", 30.0);
    rig.ticks(3);
    EXPECT_FALSE(rig.open("S4")) << "S4 открыт: в реакционной области 30 бар, К151 открыт";
    EXPECT_GE(logCount("transfer_equilibrium", "S4"), 1) << dumpLog();
}

// ── i: оба датчика резервуара ────────────────────────────────────────────────

// DD312 вне диапазона (отказ петли), DD311 достоверен и выше порога — S4
// закрывается по DD311.
TEST_F(SimSecurityRange, InvalidLowRangeSensorDoesNotHideOverpressure)
{
    RangeRig rig;
    for (const char *s : {"DD311", "DD312"})
        rig.setBar(s, 1.0);
    rig.ticks(2);
    rig.click("S4");
    ASSERT_TRUE(rig.open("S4"));

    rig.setBar("DD311", 2.3);
    rig.override.insert("DD312", 2.3);   // > 2,0625 бар (20,5 мА) — недостоверно
    rig.ticks(2);
    EXPECT_FALSE(rig.open("S4")) << "S4 открыт: DD311 = 2,3 бар, DD312 недостоверен";
    EXPECT_GE(logCount("pressure_range_autoclose", "DD311"), 1) << dumpLog();
}

// S4 закрыт: DD312 заперт в D1 и показывает старое давление — открытию он не
// мешает, решает DD311.
TEST_F(SimSecurityRange, TrappedLowRangeSensorDoesNotBlockOpening)
{
    RangeRig rig;
    rig.setBar("DD311", 1.0);
    rig.setBar("DD312", 1.85);
    rig.ticks(2);
    rig.click("S4");
    EXPECT_TRUE(rig.open("S4")) << rig.valves.lastRefusal().toStdString();
}
