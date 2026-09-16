#include "SimValveEcho.h"

#include "core/SimController.h"

namespace sim {

SimValveEcho::SimValveEcho(QStringList valveIds, SimController *controller, QObject *parent)
    : QObject(parent), m_valveIds(std::move(valveIds)), m_controller(controller),
      m_states(m_valveIds.size(), false)
{
}

bool SimValveEcho::write(const QVector<bool> &states)
{
    const QVector<bool> previous = std::exchange(m_states, states);
    m_states.resize(m_valveIds.size(), false);
    for (int i = 0; i < m_valveIds.size(); ++i) {
        const bool was = i < previous.size() && previous[i];
        if (m_states[i] == was)
            continue;
        if (m_controller)
            m_controller->notifyValve(m_valveIds[i], m_states[i]);
        emit valveChanged(m_valveIds[i], m_states[i]);
    }
    return true;
}

} // namespace sim
