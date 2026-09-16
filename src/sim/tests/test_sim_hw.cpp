#include "test_support.h"

#include "core/SimController.h"
#include "hw/SensorInverse.h"
#include "hw/SimSensorSource.h"
#include "hw/SimValveEcho.h"

#include "DataCollection.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QtConcurrent/QtConcurrentRun>

using namespace sim;
using namespace simtest;

namespace {

const ChannelBinding &binding(const QString &id)
{
    for (const auto &b : gram50().bindings)
        if (b.channelId == id) return b;
    ADD_FAILURE() << "нет привязки " << id.toStdString();
    static ChannelBinding none;
    return none;
}

// Тот же расчёт, что Grams::initAnalogData + ControllerData::addValue.
double viaControllerData(const ChannelBinding &b, double volts)
{
    ControllerData cd;
    cd.setCoeffs(b.A / b.R * 1000.0, b.B);
    cd.addValue(volts);
    return cd.getCurValue();
}

} // namespace

TEST(SensorInverse, RoundTripThroughControllerDataAllPressureCardChannels)
{
    const Catalog &c = gram50().catalog;
    int checked = 0;
    for (const auto &b : gram50().bindings) {
        if (b.card != ChannelBinding::Card::Pressure)
            continue;
        const ChannelDesc *ch = c.channel(b.channelId);
        ASSERT_NE(ch, nullptr);
        for (int k = 0; k <= 10; ++k) {
            const double wire = ch->min + (ch->max - ch->min) * k / 10.0;
            const double volts = SensorInverse::pressureCardVolts(b, ch->kind, wire);
            const double engineering = viaControllerData(b, volts);   // бар или °C
            const double back = ch->kind == ChannelKind::Temperature ? engineering : engineering * 1e5;
            EXPECT_NEAR(back, wire, std::max(1e-9 * std::abs(wire), 1e-6)) << b.channelId.toStdString() << " k=" << k;
            EXPECT_NEAR(SensorInverse::pressureCardWire(b, ch->kind, volts), wire, std::max(1e-9 * std::abs(wire), 1e-6));
        }
        ++checked;
    }
    EXPECT_EQ(checked, 8);
}

TEST(SensorInverse, CurrentLoopEndsMatchChannelRange)
{
    // 20 мА на шунте 240 Ом = 4.8 В — верх диапазона DD311.
    const ChannelBinding &b = binding("P.DD311");
    const ChannelDesc *ch = gram50().catalog.channel("P.DD311");
    EXPECT_NEAR(SensorInverse::pressureCardVolts(b, ch->kind, ch->max), 0.020 * 240, 1e-9);
    // Атмосфера на DD311: (1.01325 + 12.494)·240 / 3124.3.
    EXPECT_NEAR(SensorInverse::pressureCardVolts(b, ch->kind, 101325.0), (1.01325 + 12.494) * 240 / 3124.3, 1e-9);
    const ChannelBinding &t = binding("T.DT314");
    EXPECT_NEAR(SensorInverse::pressureCardVolts(t, ChannelKind::Temperature, -50.0), 0.004 * 240, 1e-9);
}

TEST(SensorInverse, Gauge)
{
    auto r = SensorInverse::gauge(133.322);
    EXPECT_NEAR(r.torr, 1.0, 1e-12);
    EXPECT_EQ(r.quality, Quality::Valid);
    r = SensorInverse::gauge(101325.0);
    EXPECT_EQ(r.quality, Quality::Valid);
    r = SensorInverse::gauge(200000.0);
    EXPECT_EQ(r.quality, Quality::OverRange);
    EXPECT_DOUBLE_EQ(r.torr, 761.0);
    const ChannelDesc *ch = gram50().catalog.channel("VAC.DV301");
    EXPECT_EQ(SensorInverse::gauge(ch->min).quality, Quality::Valid);
}

class SimHw : public ::testing::Test {
protected:
    ManualClock clock;
    SimController ctl{gram50().catalog, &clock};
};

TEST_F(SimHw, SensorSourceFeedsDriverUnits)
{
    SimSensorSource src(&ctl, gram50().bindings);
    ctl.setValue("P.DD331", 250000.0);   // 2.5 бар
    ctl.setValue("T.DT358", 80.0);
    ctl.setValue("T.DT341", 30.0);
    ctl.setValue("VAC.DV302", 40.0);

    // Как processEvents: температура (и такт), затем показания, затем новая секция.
    src.readTemperature();
    const double stale = src.pressureVolts()[2];
    src.readPressure();
    const double fresh = src.pressureVolts()[2];
    EXPECT_NE(stale, fresh);   // давление видно со следующего опроса

    EXPECT_NEAR(viaControllerData(binding("P.DD331"), fresh), 2.5, 1e-12);
    const QVector<double> buffer = src.pressureBuffer(2);
    ASSERT_EQ(buffer.size(), 128);
    EXPECT_DOUBLE_EQ(buffer.first(), fresh);
    EXPECT_DOUBLE_EQ(buffer.last(), fresh);
    EXPECT_NEAR(viaControllerData(binding("T.DT341"), src.pressureVolts()[6]), 30.0, 1e-12);
    EXPECT_DOUBLE_EQ(src.temperatures()[6], 80.0);   // DT358 — индекс 6 USB-4718

    EXPECT_NEAR(src.gaugeTorr(ISensorSource::Gauge::Turbo) * 133.322, 40.0, 1e-9);
    EXPECT_EQ(src.gaugeQuality(ISensorSource::Gauge::Turbo), Quality::Valid);
    EXPECT_NEAR(src.gaugeTorr(ISensorSource::Gauge::Fore), 101325.0 / 133.322, 1e-9);

    // DataCollection хранит Торр, рецепт переводит в Па.
    DataCollection gauge;
    gauge.addPoint(src.gaugeTorr(ISensorSource::Gauge::Turbo), src.gaugeQuality(ISensorSource::Gauge::Turbo));
    EXPECT_NEAR(gauge.getCurValue() * 133.322, 40.0, 1e-9);
}

TEST_F(SimHw, ReadTemperatureTicksMachine)
{
    SimSensorSource src(&ctl, gram50().bindings);
    Profile p;
    const auto doc = QJsonDocument::fromJson(R"({"schema":"grams.sim.profile/1","name":"t",
      "phases":[{"id":"a","kind":"static","zone":"chamber","durationSec":2,"tracks":[],
      "transitions":[{"to":"b","on":{"type":"timeout"}}]},
      {"id":"b","kind":"static","zone":"chamber","tracks":[],"transitions":[]}]})");
    ASSERT_TRUE(parseProfile(doc.object(), gram50().catalog, &p).ok());
    ASSERT_TRUE(ctl.run(p).ok());
    for (int i = 0; i < 5; ++i) {
        clock.advance(0.5);
        src.readTemperature();
        for (int k = 0; k < 20; ++k) QCoreApplication::processEvents();
    }
    EXPECT_EQ(ctl.currentPhaseId(), "b");
}

TEST_F(SimHw, CallFromForeignThreadIsIgnored)
{
    SimSensorSource src(&ctl, gram50().bindings);
    ctl.setValue("P.DD311", 300000.0);
    QtConcurrent::run([&src] { src.readPressure(); }).waitForFinished();
    EXPECT_NE(viaControllerData(binding("P.DD311"), src.pressureVolts()[0]), 3.0);
}

TEST_F(SimHw, ValveEchoReadsBackAndEmitsEdges)
{
    QStringList ids;
    for (const auto &v : gram50().catalog.valves)
        ids << v.id;
    SimValveEcho echo(ids, &ctl);
    QSignalSpy spy(&echo, &SimValveEcho::valveChanged);

    QVector<bool> mask(16, false);
    EXPECT_TRUE(echo.write(mask));
    EXPECT_EQ(spy.count(), 0);   // ничего не изменилось

    const int ar6 = ids.indexOf("K176");
    ASSERT_GE(ar6, 0);
    mask[ar6] = true;
    EXPECT_TRUE(echo.write(mask));
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), "K176");
    EXPECT_TRUE(spy.at(0).at(1).toBool());
    EXPECT_TRUE(ctl.isValveOpen("K176"));

    EXPECT_TRUE(echo.refresh());
    EXPECT_EQ(echo.data(), mask);

    echo.write(mask);   // та же маска — фронта нет
    EXPECT_EQ(spy.count(), 1);
    mask[ar6] = false;
    echo.write(mask);
    EXPECT_EQ(spy.count(), 2);
    EXPECT_FALSE(ctl.isValveOpen("K176"));
}
