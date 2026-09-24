#pragma once

// Каталог каналов и клапанов (sim.catalog) из данных профиля установки.
// Чистая функция: входом служат уже разобранные Initialize параметры,
// чтобы тесты строили тот же каталог из profile/GRAMsPfp.json.

#include "SimTypes.h"

#include <QHash>
#include <QJsonObject>
#include <QStringList>

namespace sim {

// Канал USB-4716: токовая петля 4–20 мА на шунте R, value = (A/R·1000)·V + B.
struct PressureCardSensor {
    QString name;   // "DD311", "DT341"
    double A = 0.0;
    double B = 0.0;
    double R = 0.0;
};

struct CatalogInput {
    QList<PressureCardSensor> pressureCard;   // 8 каналов, порядок = индекс AI
    QStringList temperatureCard;              // 8 термопар USB-4718, порядок = индекс AI
    QStringList valveCodes;                   // valveMap: порядок = бит DO
};

// Где физически живёт канал — нужно SimSensorSource для обратного перевода.
struct ChannelBinding {
    enum class Card { Pressure, Temperature, Gauge };
    QString channelId;
    Card card = Card::Pressure;
    int index = 0;                // индекс AI; для Gauge: 0 — ДВ301, 1 — ДВ302
    double A = 0.0, B = 0.0, R = 0.0;
};

struct CatalogBuild {
    Catalog catalog;
    QList<ChannelBinding> bindings;
};

CatalogBuild buildCatalog(const CatalogInput &input);

// "AR6" → "K176"; пусто, если код неизвестен.
QString valveNumberForCode(const QString &code);

QJsonObject catalogToJson(const Catalog &catalog,
                          const QHash<QString, double> &values,
                          const QHash<QString, bool> &valveOpen);

// Имена вакуумметров и их диапазон (Торр прибора → Па).
inline constexpr auto kGaugeFore = "DV301";
inline constexpr auto kGaugeTurbo = "DV302";
inline constexpr double kTorrToPa = 133.322;
inline constexpr double kGaugeMaxTorr = 761.0;    // выше — OverRange (SerialCtrl)
inline constexpr double kGaugeMinTorr = 1e-8;     // UnderRange
inline constexpr double kAtmospherePa = 101325.0;

} // namespace sim
