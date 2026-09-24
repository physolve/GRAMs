#pragma once

// Демо-источник показаний: значения SimController (Па / °C) переводятся
// в то, что отдали бы драйверы, и уходят в DataAcquisition обычным путём.

#include "controllers/ISensorSource.h"
#include "core/SimCatalog.h"

#include <QList>
#include <QPointer>

namespace sim {

class SimController;

class SimSensorSource final : public ISensorSource {
public:
    SimSensorSource(SimController *controller, QList<ChannelBinding> bindings,
                    int pressureChannels = 8, int temperatureChannels = 8, int sectionLength = 128);

    bool isPressureConnected() const override { return true; }
    bool isTemperatureConnected() const override { return true; }

    // Такт машины — здесь: DataAcquisition::processEvents зовёт readTemperature
    // каждый опрос независимо от режима «быстрого» чтения давления.
    void readTemperature() override;
    // Как у платы: новая секция давления видна только на следующем опросе.
    void readPressure() override;

    QVector<double> pressureVolts() override { return m_pressureVolts; }
    QVector<double> pressureBuffer(int channel) override;
    QVector<double> temperatures() override { return m_temperatures; }

    double gaugeTorr(Gauge gauge) const override;
    Quality gaugeQuality(Gauge gauge) const override;

private:
    bool onOwnerThread() const;
    const ChannelBinding *gaugeBinding(Gauge gauge) const;
    const ChannelDesc *channel(const ChannelBinding &b) const;

    QPointer<SimController> m_controller;
    QList<ChannelBinding> m_bindings;
    int m_sectionLength;
    QVector<double> m_pressureVolts;
    QVector<double> m_temperatures;
};

} // namespace sim
