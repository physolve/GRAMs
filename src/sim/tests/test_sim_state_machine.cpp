#include "test_support.h"

#include "core/SimController.h"
#include "core/SimErrors.h"

#include <QCoreApplication>
#include <QJsonArray>

using namespace sim;
using namespace simtest;

namespace {

Profile makeProfile(const char *json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json), &err);
    EXPECT_EQ(err.error, QJsonParseError::NoError) << err.errorString().toStdString();
    Profile p;
    const auto r = parseProfile(doc.object(), gram50().catalog, &p);
    EXPECT_TRUE(r.ok()) << (r.errors.isEmpty() ? "" : r.errors.first().code.toStdString() + " @ "
                                                        + r.errors.first().path.toStdString());
    return p;
}

// Машина обрабатывает события в цикле событий — прокручиваем его.
void settle()
{
    for (int i = 0; i < 20; ++i)
        QCoreApplication::processEvents(QEventLoop::AllEvents);
}

class SimMachine : public ::testing::Test {
protected:
    ManualClock clock;
    SimController ctl{gram50().catalog, &clock};

    void step(double sec, int ticks = 1)
    {
        for (int i = 0; i < ticks; ++i) {
            clock.advance(sec);
            ctl.tick();
            settle();
        }
    }

    QString startRun(const Profile &p, const QString &start = {})
    {
        const auto r = ctl.run(p, start);
        EXPECT_TRUE(r.ok()) << r.message.toStdString();
        settle();
        return r.runId;
    }

    QStringList reasons() const
    {
        QStringList out;
        for (const auto &h : ctl.history())
            out << h.phaseId + u'/' + h.reason;
        return out;
    }
};

// Две фазы: a (линейный рост DD331 от текущего до 2 бар за 10 с) → по таймауту b.
const char *kTimeoutProfile = R"({
  "schema": "grams.sim.profile/1", "name": "timeout", "seed": 1,
  "initial": {"P.DD331": 100000},
  "phases": [
    {"id": "a", "kind": "gasInlet", "zone": "chamber", "durationSec": 10,
     "tracks": [{"channel": "P.DD331", "interp": "linear", "keypoints": [[0, "current"], [10, 200000]]}],
     "transitions": [{"to": "b", "on": {"type": "timeout"}}]},
    {"id": "b", "kind": "static", "zone": "chamber", "durationSec": null, "tracks": [], "transitions": []}
  ]})";

} // namespace

TEST_F(SimMachine, IdleByDefault)
{
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
    const QJsonObject s = ctl.statusJson();
    EXPECT_EQ(s.value("machine").toString(), "idle");
    EXPECT_TRUE(s.value("runId").isNull());
    EXPECT_TRUE(s.value("phase").isNull());
    EXPECT_DOUBLE_EQ(s.value("channels").toObject().value("VAC.DV301").toDouble(), 101325.0);
    EXPECT_EQ(s.value("valves").toObject().value("K176").toString(), "closed");
}

TEST_F(SimMachine, RunEntersFirstPhaseAndAppliesInitial)
{
    startRun(makeProfile(kTimeoutProfile));
    EXPECT_EQ(ctl.machine(), SimController::Machine::Active);
    EXPECT_EQ(ctl.currentPhaseId(), "a");
    EXPECT_EQ(reasons(), QStringList{"a/start"});
    EXPECT_DOUBLE_EQ(ctl.value("P.DD331"), 100000);
}

TEST_F(SimMachine, TimeoutAndTrackInterpolation)
{
    startRun(makeProfile(kTimeoutProfile));
    step(1.0, 5);
    EXPECT_NEAR(ctl.value("P.DD331"), 150000, 1e-6);
    EXPECT_NEAR(ctl.currentPhaseElapsed(), 5.0, 1e-9);
    step(1.0, 4);
    EXPECT_EQ(ctl.currentPhaseId(), "a");
    step(1.0);
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    EXPECT_EQ(reasons(), (QStringList{"a/start", "b/timeout"}));
    // b без треков — значение удерживается.
    step(1.0, 3);
    EXPECT_NEAR(ctl.value("P.DD331"), 200000, 1e-6);
}

TEST_F(SimMachine, StartPhaseIdAndErrors)
{
    const Profile p = makeProfile(kTimeoutProfile);
    EXPECT_EQ(ctl.run(p, "nope").code, RpcError::UnknownPhase);
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
    startRun(p, "b");
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    EXPECT_EQ(ctl.run(p).code, RpcError::Busy);
    EXPECT_EQ(ctl.pause("other-run").code, RpcError::NoActiveRun);
    EXPECT_EQ(ctl.gotoPhase(ctl.runId(), "zzz").code, RpcError::UnknownPhase);
}

namespace {
const char *kValveProfile = R"({
  "schema": "grams.sim.profile/1", "name": "valve",
  "phases": [
    {"id": "atm", "kind": "static", "zone": "chamber", "durationSec": null, "tracks": [],
     "transitions": [{"to": "pump", "on": {"type": "valve", "valve": "K176", "state": "open"}}]},
    {"id": "pump", "kind": "pumping", "zone": "vacuum", "durationSec": 100,
     "tracks": [{"channel": "VAC.DV301", "interp": "log", "keypoints": [[0, "current"], [100, 10]]}],
     "transitions": [{"to": "atm", "on": {"type": "valve", "valve": "K176", "state": "closed"}}]}
  ]})";
} // namespace

TEST_F(SimMachine, ValveLevelDoesNotFireOnlyEdge)
{
    ctl.notifyValve("K176", true);   // открыт до прогона — уровень, не фронт
    startRun(makeProfile(kValveProfile));
    step(1.0, 3);
    EXPECT_EQ(ctl.currentPhaseId(), "atm");

    ctl.notifyValve("K176", false);
    step(1.0);
    EXPECT_EQ(ctl.currentPhaseId(), "atm");
    ctl.notifyValve("K176", true);
    step(1.0);
    EXPECT_EQ(ctl.currentPhaseId(), "pump");
    EXPECT_EQ(ctl.history().last().reason, "valve");

    step(1.0, 50);
    EXPECT_NEAR(ctl.value("VAC.DV301"), std::pow(10.0, (std::log10(101325.0) + 1.0) / 2.0), 1e-6);
    ctl.notifyValve("K176", false);
    step(1.0);
    EXPECT_EQ(ctl.currentPhaseId(), "atm");
}

TEST_F(SimMachine, EdgeBeforePhaseEntryIsIgnored)
{
    const Profile p = makeProfile(R"({
      "schema": "grams.sim.profile/1", "name": "edge",
      "phases": [
        {"id": "a", "kind": "static", "zone": "vacuum", "durationSec": 5, "tracks": [],
         "transitions": [{"to": "b", "on": {"type": "timeout"}}]},
        {"id": "b", "kind": "static", "zone": "vacuum", "durationSec": null, "tracks": [],
         "transitions": [{"to": "c", "on": {"type": "valve", "valve": "K179", "state": "open"}}]},
        {"id": "c", "kind": "static", "zone": "vacuum", "durationSec": null, "tracks": [], "transitions": []}
      ]})");
    startRun(p);
    step(1.0, 2);
    ctl.notifyValve("K179", true);   // фронт в фазе a
    step(1.0, 3);
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    step(1.0, 3);
    EXPECT_EQ(ctl.currentPhaseId(), "b");
}

TEST_F(SimMachine, FirstMatchingTransitionWins)
{
    auto profile = [](bool valveFirst) {
        const QString valve = R"({"to": "x", "on": {"type": "valve", "valve": "K176", "state": "open"}})";
        const QString timeout = R"({"to": "y", "on": {"type": "timeout"}})";
        const QString json = QStringLiteral(R"({
          "schema": "grams.sim.profile/1", "name": "order",
          "phases": [
            {"id": "a", "kind": "static", "zone": "vacuum", "durationSec": 1, "tracks": [],
             "transitions": [%1, %2]},
            {"id": "x", "kind": "static", "zone": "vacuum", "tracks": [], "transitions": []},
            {"id": "y", "kind": "static", "zone": "vacuum", "tracks": [], "transitions": []}
          ]})").arg(valveFirst ? valve : timeout, valveFirst ? timeout : valve);
        return makeProfile(json.toUtf8().constData());
    };

    startRun(profile(true));
    ctl.notifyValve("K176", true);
    step(2.0);   // в одном такте и фронт, и таймаут
    EXPECT_EQ(ctl.currentPhaseId(), "x");
    ctl.stop(ctl.runId());
    settle();

    ctl.notifyValve("K176", false);
    startRun(profile(false));
    ctl.notifyValve("K176", true);
    step(2.0);
    EXPECT_EQ(ctl.currentPhaseId(), "y");
}

TEST_F(SimMachine, ManualTransitionNeverFiresByItself)
{
    const Profile p = makeProfile(R"({
      "schema": "grams.sim.profile/1", "name": "manual",
      "phases": [
        {"id": "a", "kind": "static", "zone": "vacuum", "durationSec": 1, "tracks": [],
         "transitions": [{"to": "b", "on": {"type": "manual"}}]},
        {"id": "b", "kind": "static", "zone": "vacuum", "tracks": [], "transitions": []}
      ]})");
    const QString run = startRun(p);
    step(1.0, 5);
    EXPECT_EQ(ctl.currentPhaseId(), "a");
    EXPECT_TRUE(ctl.gotoPhase(run, "b").ok());
    settle();
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    EXPECT_EQ(ctl.history().last().reason, "manual");
}

TEST_F(SimMachine, GotoResetsPhaseTime)
{
    const QString run = startRun(makeProfile(kTimeoutProfile));
    step(1.0, 4);
    ASSERT_TRUE(ctl.gotoPhase(run, "a").ok());
    settle();
    EXPECT_EQ(ctl.currentPhaseId(), "a");
    EXPECT_DOUBLE_EQ(ctl.currentPhaseElapsed(), 0.0);
    EXPECT_EQ(reasons(), (QStringList{"a/start", "a/manual"}));
}

TEST_F(SimMachine, PauseFreezesTimeAndValuesResumeContinues)
{
    const QString run = startRun(makeProfile(kTimeoutProfile));
    step(1.0, 3);
    const double frozen = ctl.value("P.DD331");
    ASSERT_TRUE(ctl.pause(run).ok());
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Paused);
    EXPECT_EQ(ctl.statusJson().value("machine").toString(), "paused");

    step(1.0, 100);   // часы идут, фаза — нет
    EXPECT_DOUBLE_EQ(ctl.currentPhaseElapsed(), 3.0);
    EXPECT_DOUBLE_EQ(ctl.value("P.DD331"), frozen);
    EXPECT_EQ(ctl.currentPhaseId(), "a");

    ASSERT_TRUE(ctl.resume(run).ok());
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Active);
    EXPECT_EQ(ctl.currentPhaseId(), "a");
    EXPECT_DOUBLE_EQ(ctl.currentPhaseElapsed(), 3.0);
    EXPECT_EQ(ctl.history().size(), 1);   // возврат из истории — не новый вход

    step(1.0, 7);
    EXPECT_EQ(ctl.currentPhaseId(), "b");
}

TEST_F(SimMachine, GotoWhilePausedStaysPaused)
{
    const QString run = startRun(makeProfile(kTimeoutProfile));
    step(1.0, 2);
    ctl.pause(run);
    settle();
    ASSERT_TRUE(ctl.gotoPhase(run, "b").ok());
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Paused);
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    ctl.resume(run);
    settle();
    EXPECT_EQ(ctl.currentPhaseId(), "b");
    EXPECT_EQ(ctl.machine(), SimController::Machine::Active);
}

TEST_F(SimMachine, StopHoldsValuesAndAllowsNewRun)
{
    const Profile p = makeProfile(kTimeoutProfile);
    const QString run = startRun(p);
    step(1.0, 5);
    const double held = ctl.value("P.DD331");
    ASSERT_TRUE(ctl.stop(run).ok());
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
    EXPECT_DOUBLE_EQ(ctl.value("P.DD331"), held);
    step(1.0, 5);
    EXPECT_DOUBLE_EQ(ctl.value("P.DD331"), held);

    const QJsonObject s = ctl.statusJson();
    EXPECT_TRUE(s.value("runId").isNull());
    EXPECT_TRUE(s.value("phase").isNull());
    EXPECT_EQ(s.value("history").toArray().size(), 1);
    EXPECT_EQ(ctl.stop(run).code, RpcError::NoActiveRun);

    const QString second = startRun(p);
    EXPECT_NE(second, run);
}

TEST_F(SimMachine, StopDuringStartupIsHonoured)
{
    const auto r = ctl.run(makeProfile(kTimeoutProfile));
    ASSERT_TRUE(r.ok());
    EXPECT_TRUE(ctl.stop(r.runId).ok());   // машина ещё не стартовала
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
}

TEST_F(SimMachine, FaultThenStop)
{
    const QString run = startRun(makeProfile(kTimeoutProfile));
    ctl.raiseFault("тест");
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Fault);
    EXPECT_EQ(ctl.statusJson().value("lastError").toString(), "тест");
    EXPECT_EQ(ctl.run(makeProfile(kTimeoutProfile)).code, RpcError::Busy);
    ctl.stop(run);
    settle();
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
}

TEST_F(SimMachine, StatusShape)
{
    startRun(makeProfile(kTimeoutProfile));
    step(2.5);
    const QJsonObject s = ctl.statusJson(QJsonObject{{"name", "Вакуум"}, {"step", "11.7б"}});
    EXPECT_EQ(s.value("machine").toString(), "active");
    EXPECT_EQ(s.value("profileName").toString(), "timeout");
    const QJsonObject phase = s.value("phase").toObject();
    EXPECT_EQ(phase.value("id").toString(), "a");
    EXPECT_EQ(phase.value("kind").toString(), "gasInlet");
    EXPECT_EQ(phase.value("zone").toString(), "chamber");
    EXPECT_DOUBLE_EQ(phase.value("elapsedSec").toDouble(), 2.5);
    EXPECT_DOUBLE_EQ(phase.value("durationSec").toDouble(), 10.0);
    EXPECT_EQ(s.value("regime").toObject().value("step").toString(), "11.7б");
    const QJsonObject h = s.value("history").toArray().first().toObject();
    EXPECT_EQ(h.value("reason").toString(), "start");
    EXPECT_TRUE(h.value("enteredAt").toString().endsWith('Z'));
    EXPECT_TRUE(s.value("lastError").isNull());
}

TEST_F(SimMachine, NoiseIsReproducibleBySeed)
{
    const Profile p = makeProfile(R"({
      "schema": "grams.sim.profile/1", "name": "noise", "seed": 99,
      "initial": {"P.DD331": 100000},
      "phases": [{"id": "a", "kind": "static", "zone": "chamber", "durationSec": null,
        "tracks": [{"channel": "P.DD331", "interp": "step", "keypoints": [[0, 100000]], "noise": {"sigmaAbs": 50}}],
        "transitions": []}]})");
    auto sample = [&] {
        QList<double> out;
        const QString run = startRun(p);
        for (int i = 0; i < 10; ++i) {
            step(0.5);
            out << ctl.value("P.DD331");
        }
        ctl.stop(run);
        settle();
        return out;
    };
    const auto first = sample();
    const auto second = sample();
    EXPECT_EQ(first, second);
    EXPECT_NE(first[0], first[1]);
}
