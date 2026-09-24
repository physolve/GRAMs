#include "SimSensorSource.h"

#include "SensorInverse.h"
#include "core/SimController.h"

#include <QDebug>
#include <QThread>

namespace sim {

SimSensorSource::SimSensorSource(SimController *controller, QList<ChannelBinding> bindings,
                                 int pressureChannels, int temperatureChannels, int sectionLength)
    : m_controller(controller), m_bindings(std::move(bindings)), m_sectionLength(sectionLength),
      m_pressureVolts(pressureChannels, 0.0), m_temperatures(temperatureChannels, 0.0)
{
    readPressure();
    readTemperature();
}

bool SimSensorSource::onOwnerThread() const
{
    // Всё в главном потоке. Устаревший InletAction зовёт fastBufferRead из
    // QtConcurrent — там демо-данные не обновляются, а не гоняются без защиты.
    if (m_controller && QThread::currentThread() == m_controller->thread())
        return true;
    qWarning() << "SimSensorSource: вызов не из потока SimController — пропущен";
    return false;
}

const ChannelDesc *SimSensorSource::channel(const ChannelBinding &b) const
{
    return m_controller ? m_controller->catalog().channel(b.channelId) : nullptr;
}

void SimSensorSource::readTemperature()
{
    if (!onOwnerThread())
        return;
    m_controller->tick();
    for (const auto &b : std::as_const(m_bindings))
        if (b.card == ChannelBinding::Card::Temperature && b.index < m_temperatures.size())
            m_temperatures[b.index] = m_controller->value(b.channelId);
}

void SimSensorSource::readPressure()
{
    if (!onOwnerThread())
        return;
    for (const auto &b : std::as_const(m_bindings)) {
        if (b.card != ChannelBinding::Card::Pressure || b.index >= m_pressureVolts.size())
            continue;
        if (const ChannelDesc *ch = channel(b))
            m_pressureVolts[b.index] =
                SensorInverse::pressureCardVolts(b, ch->kind, m_controller->value(b.channelId));
    }
}

QVector<double> SimSensorSource::pressureBuffer(int channel)
{
    const double v = (channel >= 0 && channel < m_pressureVolts.size()) ? m_pressureVolts[channel] : 0.0;
    return QVector<double>(m_sectionLength, v);
}

const ChannelBinding *SimSensorSource::gaugeBinding(Gauge gauge) const
{
    for (const auto &b : m_bindings)
        if (b.card == ChannelBinding::Card::Gauge && b.index == int(gauge))
            return &b;
    return nullptr;
}

double SimSensorSource::gaugeTorr(Gauge gauge) const
{
    const ChannelBinding *b = gaugeBinding(gauge);
    if (!b || !m_controller)
        return 0.0;
    return SensorInverse::gauge(m_controller->value(b->channelId)).torr;
}

Quality SimSensorSource::gaugeQuality(Gauge gauge) const
{
    const ChannelBinding *b = gaugeBinding(gauge);
    if (!b || !m_controller)
        return Quality::NoResponse;
    return SensorInverse::gauge(m_controller->value(b->channelId)).quality;
}

} // namespace sim
