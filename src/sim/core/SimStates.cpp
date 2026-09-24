#include "SimStates.h"

#include "SimController.h"

namespace sim {

namespace {
QString edgeKey(const QString &valveId, bool open)
{
    return valveId + (open ? u":open" : u":closed");
}
} // namespace

SimPhaseState::SimPhaseState(const PhaseSpec &spec, SimController *controller, QState *parent)
    : QState(parent), m_spec(spec), m_controller(controller)
{
    setObjectName(spec.id);
}

void SimPhaseState::latchValveEdge(const QString &valveId, bool open)
{
    m_edges.insert(edgeKey(valveId, open));
}

bool SimPhaseState::hasValveEdge(const QString &valveId, bool open) const
{
    return m_edges.contains(edgeKey(valveId, open));
}

void SimPhaseState::onEntry(QEvent *event)
{
    QState::onEntry(event);
    // Возврат с паузы через QHistoryState — та же фаза продолжается.
    if (m_controller->consumeResume()) {
        m_controller->phaseEntered(this, false);
        return;
    }
    m_elapsed = 0.0;
    m_edges.clear();
    m_resolved.clear();
    for (const auto &track : m_spec.tracks)
        m_resolved.append(TrackEngine::resolve(track.keypoints, m_controller->value(track.channel)));
    m_controller->phaseEntered(this, true);
}

ProfileTransition::ProfileTransition(const TransitionSpec &spec, SimPhaseState *source, SimController *controller)
    : QAbstractTransition(source), m_spec(spec), m_source(source), m_controller(controller)
{
}

bool ProfileTransition::eventTest(QEvent *event)
{
    if (event->type() != SimTickEvent::staticType())
        return false;
    switch (m_spec.type) {
    case TransitionSpec::Type::Timeout:
        return m_source->timedOut();
    case TransitionSpec::Type::Valve:
        return m_source->hasValveEdge(m_spec.valve, m_spec.open);
    case TransitionSpec::Type::Manual:
        return false;   // только sim.goto
    }
    return false;
}

void ProfileTransition::onTransition(QEvent *)
{
    m_controller->setNextReason(m_spec.type == TransitionSpec::Type::Timeout
                                    ? QStringLiteral("timeout") : QStringLiteral("valve"));
}

EventTypeTransition::EventTypeTransition(QEvent::Type type, QState *source, std::function<void(QEvent *)> action)
    : QAbstractTransition(source), m_type(type), m_action(std::move(action))
{
}

GotoTransition::GotoTransition(const QString &phaseId, QState *source, SimController *controller)
    : QAbstractTransition(source), m_phaseId(phaseId), m_controller(controller)
{
}

bool GotoTransition::eventTest(QEvent *event)
{
    return event->type() == SimGotoEvent::staticType()
        && static_cast<SimGotoEvent *>(event)->phaseId == m_phaseId;
}

void GotoTransition::onTransition(QEvent *)
{
    m_controller->setNextReason(QStringLiteral("manual"));
}

ValveLatchTransition::ValveLatchTransition(QState *source, SimController *controller)
    : QAbstractTransition(source), m_controller(controller)
{
}

void ValveLatchTransition::onTransition(QEvent *event)
{
    const auto *e = static_cast<SimValveEvent *>(event);
    m_controller->latchValveEdge(e->valveId, e->open);
}

} // namespace sim
