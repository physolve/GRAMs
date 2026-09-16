#pragma once

#include "ISensorSource.h"
#include "AdvantechCtrl.h"
#include "SerialCtrl.h"

// Настоящие датчики: платы Advantech и вакуумметры по serial.
// Только делегирует — логика опроса та же, что жила в DataAcquisition.
//
// Создаётся лишь на пути с железом: конструктор AdvantechBuff обращается
// к biodaq (WaveformAiCtrl::Create), поэтому в демо-режиме его нет.
class RealSensorSource final : public ISensorSource
{
public:
    RealSensorSource() = default;

    AdvantechBuff &pressureCard() { return m_pressure; }
    AdvantechAI &temperatureCard() { return m_temperature; }
    VacuumController &foreGauge() { return m_fore; }
    TurboVacuumController &turboGauge() { return m_turbo; }

    bool isPressureConnected() const override { return m_pressure.isConnected(); }
    bool isTemperatureConnected() const override { return m_temperature.isConnected(); }

    void readPressure() override { m_pressure.readData(); }
    void readTemperature() override { m_temperature.readData(); }

    QVector<double> pressureVolts() override { return m_pressure.getData(); }
    QVector<double> pressureBuffer(int channel) override
    {
        return m_pressure.getBufferedData(static_cast<uint8_t>(channel));
    }
    QVector<double> temperatures() override { return m_temperature.getData(); }

    double gaugeTorr(Gauge gauge) const override
    {
        return gauge == Gauge::Fore ? m_fore.getData() : m_turbo.getData();
    }
    Quality gaugeQuality(Gauge gauge) const override
    {
        return gauge == Gauge::Fore ? m_fore.quality() : m_turbo.quality();
    }

private:
    AdvantechBuff m_pressure;
    AdvantechAI m_temperature;
    VacuumController m_fore;
    TurboVacuumController m_turbo;
};
