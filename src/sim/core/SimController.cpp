#include "SimController.h"

#include "SimErrors.h"
#include "SimStates.h"

#include <QFinalState>
#include <QHistoryState>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QStateMachine>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace sim {

SimController::SimController(Catalog catalog, ISimClock *clock, QObject *parent)
    : QObject(parent), m_catalog(std::move(catalog)), m_clock(clock)
{
    for (const auto &ch : m_catalog.channels)
        m_values.insert(ch.id, ch.defaultValue);
    for (const auto &v : m_catalog.valves)
        m_valves.insert(v.id, false);
    m_lastTick = m_clock->nowSec();
}

SimController::~SimController()
{
    qDeleteAll(m_pendingEvents);
}

double SimController::value(const QString &channelId) const
{
    return m_values.value(channelId, std::nan(""));
}

void SimController::setValue(const QString &channelId, double value)
{
    const ChannelDesc *ch = m_catalog.channel(channelId);
    if (!ch)
        return;
    m_values[channelId] = TrackEngine::clampToRange(value, ch->min, ch->max);
    emit valuesUpdated();
}

QString SimController::toWire(Machine m)
{
    switch (m) {
    case Machine::Idle:   return QStringLiteral("idle");
    case Machine::Active: return QStringLiteral("active");
    case Machine::Paused: return QStringLiteral("paused");
    case Machine::Fault:  return QStringLiteral("fault");
    }
    return {};
}

// ── путь данных ───────────────────────────────────────────────────────────────

void SimController::tick()
{
    const double now = m_clock->nowSec();
    const double dt = std::max(0.0, now - m_lastTick);
    m_lastTick = now;
    // На паузе время фазы стоит, значения удерживаются.
    if (m_machineState != Machine::Active || !m_phase)
        return;
    m_phase->advance(dt);
    updateTracks();
    postToMachine(new SimTickEvent);
}

void SimController::notifyValve(const QString &valveId, bool open)
{
    if (!m_valves.contains(valveId) || m_valves.value(valveId) == open)
        return;
    m_valves[valveId] = open;
    if (m_machineState != Machine::Idle)
        postToMachine(new SimValveEvent(valveId, open));
}

void SimController::updateTracks()
{
    if (!m_phase)
        return;
    const PhaseSpec &spec = m_phase->spec();
    for (int i = 0; i < spec.tracks.size(); ++i) {
        const TrackSpec &track = spec.tracks[i];
        const ChannelDesc *ch = m_catalog.channel(track.channel);
        if (!ch)
            continue;
        double v = TrackEngine::evaluate(track.interp, track.tau, m_phase->resolvedKeypoints(i),
                                         m_phase->elapsedSec());
        if (!std::isfinite(v)) {
            raiseFault(QStringLiteral("фаза %1, канал %2: значение не число").arg(spec.id, track.channel));
            return;
        }
        v = m_noise.apply(v, track.sigmaRel, track.sigmaAbs);
        m_values[track.channel] = TrackEngine::clampToRange(v, ch->min, ch->max);
    }
    emit valuesUpdated();
}

// ── команды ───────────────────────────────────────────────────────────────────

SimController::Result SimController::run(const Profile &profile, const QString &startPhaseId)
{
    if (m_machineState != Machine::Idle)
        return {RpcError::Busy, QStringLiteral("прогон %1 уже идёт").arg(m_runId), {}};
    if (profile.phases.isEmpty())
        return {RpcError::InvalidProfile, QStringLiteral("в профиле нет фаз"), {}};
    const QString start = startPhaseId.isEmpty() ? profile.phases.first().id : startPhaseId;
    if (profile.phaseIndex(start) < 0)
        return {RpcError::UnknownPhase, QStringLiteral("нет фазы %1").arg(start), {}};

    m_profile = profile;
    m_startPhaseId = start;
    m_runId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_history.clear();
    m_lastError.clear();
    m_nextReason = QStringLiteral("start");
    m_noise.reseed(profile.seed ? *profile.seed : QRandomGenerator::global()->generate64());
    for (auto it = profile.initial.begin(); it != profile.initial.end(); ++it)
        setValue(it.key(), it.value());
    m_lastTick = m_clock->nowSec();
    setMachineState(Machine::Active);

    QCoreApplication::postEvent(this, new SimRunEvent);
    return {0, {}, m_runId};
}

SimController::Result SimController::checkRun(const QString &runId) const
{
    if (m_machineState == Machine::Idle || runId != m_runId)
        return {RpcError::NoActiveRun, QStringLiteral("нет активного прогона %1").arg(runId), {}};
    return {};
}

SimController::Result SimController::gotoPhase(const QString &runId, const QString &phaseId)
{
    if (auto r = checkRun(runId); !r.ok())
        return r;
    if (m_profile.phaseIndex(phaseId) < 0)
        return {RpcError::UnknownPhase, QStringLiteral("нет фазы %1").arg(phaseId), {}};
    const bool wasPaused = m_machineState == Machine::Paused;
    postToMachine(new SimGotoEvent(phaseId));
    // На паузе переход выполняется, но прогон остаётся на паузе.
    if (wasPaused)
        postToMachine(new SimPauseEvent);
    return {};
}

SimController::Result SimController::pause(const QString &runId)
{
    if (auto r = checkRun(runId); !r.ok())
        return r;
    postToMachine(new SimPauseEvent);
    return {};
}

SimController::Result SimController::resume(const QString &runId)
{
    if (auto r = checkRun(runId); !r.ok())
        return r;
    postToMachine(new SimResumeEvent);
    return {};
}

SimController::Result SimController::stop(const QString &runId)
{
    if (auto r = checkRun(runId); !r.ok())
        return r;
    postToMachine(new SimStopEvent);
    return {};
}

void SimController::raiseFault(const QString &reason)
{
    if (m_machineState == Machine::Idle)
        return;
    m_lastError = reason;
    postToMachine(new SimFaultEvent(reason));
}

// ── машина ────────────────────────────────────────────────────────────────────

bool SimController::event(QEvent *event)
{
    if (event->type() == SimRunEvent::staticType()) {
        buildMachine();
        return true;
    }
    return QObject::event(event);
}

void SimController::postToMachine(QEvent *event)
{
    if (m_machine && m_machine->isRunning())
        m_machine->postEvent(event);
    else if (m_machineState != Machine::Idle)
        m_pendingEvents.append(event);   // машина ещё стартует
    else
        delete event;
}

void SimController::buildMachine()
{
    auto *machine = new QStateMachine(this);
    auto *active = new QState(machine);
    auto *paused = new QState(machine);
    auto *fault = new QState(machine);
    auto *stopped = new QFinalState(machine);
    active->setObjectName(QStringLiteral("active"));
    paused->setObjectName(QStringLiteral("paused"));
    fault->setObjectName(QStringLiteral("fault"));

    QHash<QString, SimPhaseState *> phases;
    for (const auto &spec : m_profile.phases)
        phases.insert(spec.id, new SimPhaseState(spec, this, active));

    // Переходы профиля — в порядке перечисления: первый подходящий побеждает.
    for (const auto &spec : m_profile.phases) {
        for (const auto &t : spec.transitions) {
            auto *tr = new ProfileTransition(t, phases.value(spec.id), this);
            tr->setTargetState(phases.value(t.to));
        }
    }

    auto *history = new QHistoryState(QHistoryState::DeepHistory, active);
    history->setDefaultState(phases.value(m_startPhaseId));
    active->setInitialState(phases.value(m_startPhaseId));
    machine->setInitialState(active);

    for (auto it = phases.begin(); it != phases.end(); ++it) {
        auto *fromActive = new GotoTransition(it.key(), active, this);
        fromActive->setTargetState(it.value());
        fromActive->setTransitionType(QAbstractTransition::InternalTransition);
        auto *fromPaused = new GotoTransition(it.key(), paused, this);
        fromPaused->setTargetState(it.value());
    }

    new ValveLatchTransition(active, this);
    new ValveLatchTransition(paused, this);

    (new EventTypeTransition(SimPauseEvent::staticType(), active))->setTargetState(paused);
    (new EventTypeTransition(SimResumeEvent::staticType(), paused,
                             [this](QEvent *) { m_resuming = true; }))->setTargetState(history);
    for (QState *s : {active, paused})
        (new EventTypeTransition(SimFaultEvent::staticType(), s))->setTargetState(fault);
    for (QState *s : {active, paused, fault})
        (new EventTypeTransition(SimStopEvent::staticType(), s))->setTargetState(stopped);

    connect(active, &QState::entered, this, [this] { setMachineState(Machine::Active); });
    connect(paused, &QState::entered, this, [this] { setMachineState(Machine::Paused); });
    connect(fault, &QState::entered, this, [this] { setMachineState(Machine::Fault); });
    connect(machine, &QStateMachine::started, this, [this] {
        const auto pending = std::exchange(m_pendingEvents, {});
        for (QEvent *e : pending)
            m_machine->postEvent(e);
    });
    connect(machine, &QStateMachine::finished, this, &SimController::onMachineFinished);

    m_machine = machine;
    machine->start();
}

void SimController::onMachineFinished()
{
    if (m_machine)
        m_machine->deleteLater();
    m_machine = nullptr;
    m_phase = nullptr;
    m_runId.clear();
    qDeleteAll(std::exchange(m_pendingEvents, {}));
    setMachineState(Machine::Idle);
    emit phaseChanged();
}

void SimController::setMachineState(Machine m)
{
    if (m_machineState == m)
        return;
    m_machineState = m;
    emit machineChanged();
}

void SimController::phaseEntered(SimPhaseState *phase, bool fresh)
{
    m_phase = phase;
    if (fresh) {
        m_history.append({phase->spec().id, QDateTime::currentDateTimeUtc(),
                          m_nextReason.isEmpty() ? QStringLiteral("start") : m_nextReason});
        m_nextReason.clear();
        updateTracks();
    }
    emit phaseChanged();
}

void SimController::latchValveEdge(const QString &valveId, bool open)
{
    if (m_phase)
        m_phase->latchValveEdge(valveId, open);
}

bool SimController::consumeResume()
{
    return std::exchange(m_resuming, false);
}

// ── статус ────────────────────────────────────────────────────────────────────

QString SimController::currentPhaseId() const
{
    return (m_phase && m_machineState != Machine::Idle) ? m_phase->spec().id : QString();
}

QString SimController::currentPhaseLabel() const
{
    if (!m_phase || m_machineState == Machine::Idle)
        return {};
    return m_phase->spec().label.isEmpty() ? m_phase->spec().id : m_phase->spec().label;
}

double SimController::currentPhaseElapsed() const
{
    return (m_phase && m_machineState != Machine::Idle) ? m_phase->elapsedSec() : 0.0;
}

QJsonObject SimController::statusJson(const QJsonValue &regime) const
{
    const bool idle = m_machineState == Machine::Idle;

    QJsonValue phase = QJsonValue::Null;
    if (!idle && m_phase) {
        const PhaseSpec &s = m_phase->spec();
        phase = QJsonObject{
            {"id", s.id},
            {"kind", sim::toWire(s.kind)},
            {"zone", s.zone},
            {"elapsedSec", m_phase->elapsedSec()},
            {"durationSec", s.durationSec ? QJsonValue(*s.durationSec) : QJsonValue(QJsonValue::Null)},
        };
    }

    QJsonObject channels;
    for (const auto &ch : m_catalog.channels)
        channels.insert(ch.id, m_values.value(ch.id));
    QJsonObject valves;
    for (const auto &v : m_catalog.valves)
        valves.insert(v.id, m_valves.value(v.id) ? "open" : "closed");

    QJsonArray history;
    for (const auto &h : m_history)
        history.append(QJsonObject{{"phaseId", h.phaseId},
                                   {"enteredAt", h.enteredAt.toString(Qt::ISODate)},
                                   {"reason", h.reason}});

    return QJsonObject{
        {"machine", toWire(m_machineState)},
        {"runId", idle ? QJsonValue(QJsonValue::Null) : QJsonValue(m_runId)},
        {"profileName", idle ? QJsonValue(QJsonValue::Null) : QJsonValue(m_profile.name)},
        {"phase", phase},
        {"channels", channels},
        {"valves", valves},
        {"regime", regime},
        {"history", history},
        {"lastError", m_lastError.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(m_lastError)},
    };
}

} // namespace sim
