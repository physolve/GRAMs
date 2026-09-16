#pragma once

// Обратный перевод «значение на проводе → сырое показание драйвера».
//
// Прямой путь (ControllerData::addValue, Grams::initAnalogData):
//   USB-4716: value = (A/R·1000)·V + B  — бар для давления, °C для DT341/DT314;
//   USB-4718: °C как есть;
//   ДВ301/ДВ302: Торр, DataCollection хранит Торр, рецепт умножает на 133.322.
// Здесь — обратное, чтобы демо-значения шли по тому же пути, что и настоящие.

#include "core/SimCatalog.h"

#include "SensorQuality.h"

namespace sim::SensorInverse {

// Па (давление) или °C (температура на токовой петле) → В после фильтра.
double pressureCardVolts(const ChannelBinding &binding, ChannelKind kind, double wireValue);

// Прямой перевод по тем же коэффициентам — для проверки «туда-обратно».
double pressureCardWire(const ChannelBinding &binding, ChannelKind kind, double volts);

struct GaugeReading {
    double torr = 0.0;
    Quality quality = Quality::Valid;
};

// Па → Торр вакуумметра; выше 761 Торр — OverRange с 761, как SerialCtrl.
GaugeReading gauge(double pa);

} // namespace sim::SensorInverse
