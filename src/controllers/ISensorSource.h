#pragma once

#include "../SensorQuality.h"

#include <QVector>

// Источник сырых показаний датчиков для DataAcquisition.
//
// Граница проходит сразу над драйверами: всё, что ниже (плата АЦП, фильтр
// Калмана, протокол вакуумметра), живёт в реализации, всё, что выше
// (калибровка ControllerData, буферы напуска/натекания, квартили, GUI,
// режимы), не знает, откуда пришли значения.
//
// Единицы — как у драйверов:
//   pressureVolts / pressureBuffer — В после фильтра Калмана, USB-4716;
//   temperatures                   — °C, USB-4718;
//   gaugeTorr                      — Торр, вакуумметры ДВ301 / ДВ302.
//
// Реализации: RealSensorSource (железо) и sim::SimSensorSource (демо-данные).
class ISensorSource
{
public:
    enum class Gauge { Fore = 0, Turbo = 1 };

    virtual ~ISensorSource() = default;

    virtual bool isPressureConnected() const = 0;
    virtual bool isTemperatureConnected() const = 0;

    // Запуск чтения очередной секции / опроса карты.
    virtual void readPressure() = 0;
    virtual void readTemperature() = 0;

    virtual QVector<double> pressureVolts() = 0;               // по одному на канал
    virtual QVector<double> pressureBuffer(int channel) = 0;   // секция канала
    virtual QVector<double> temperatures() = 0;                // по одному на канал

    virtual double gaugeTorr(Gauge gauge) const = 0;
    virtual Quality gaugeQuality(Gauge gauge) const = 0;
};
