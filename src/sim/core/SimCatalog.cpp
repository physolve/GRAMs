#include "SimCatalog.h"

#include <QJsonArray>

#include <algorithm>

namespace sim {

namespace {

struct ZoneInfo {
    const char *zone;
    const char *label;
};

// Принадлежность датчиков участкам — по квартилям GRAM50
// (Grams.cpp: initStorageQuartile / initReactionQuartile). Термопары
// DT350, DT351, DT357 ни в один квартиль не входят — отнесены к подаче.
ZoneInfo zoneForSensor(const QString &name)
{
    static const QHash<QString, ZoneInfo> table = {
        {"DD311", {"accumulator", "резервуар, высокий"}},
        {"DD312", {"accumulator", "резервуар, низкий"}},
        {"DD341", {"accumulator", "резервуар, 2-й высокий"}},
        {"DT341", {"accumulator", "резервуар"}},
        {"DT314", {"accumulator", "резервуар"}},
        {"DT352", {"accumulator", "ёмкость малая"}},
        {"DT354", {"accumulator", "ёмкость средняя"}},
        {"DT356", {"accumulator", "ёмкость большая"}},
        {"DD331", {"chamber", "камера, высокий"}},
        {"DD332", {"chamber", "камера, атм."}},
        {"DD334", {"chamber", "камера, низкий"}},
        {"DT358", {"chamber", "реакционная зона"}},
        {"DT359", {"chamber", "камера F"}},
        {"DT350", {"supply", "не назначен"}},
        {"DT351", {"supply", "не назначен"}},
        {"DT357", {"supply", "трубка"}},
    };
    return table.value(name, ZoneInfo{"supply", "не назначен"});
}

struct ValveInfo {
    const char *number;
    const char *label;
};

const QHash<QString, ValveInfo> &valveTable()
{
    // Grams.h: комментарии у vAR1…vR4; роли — docs/regimes/vacuum.md.
    static const QHash<QString, ValveInfo> table = {
        {"AR1", {"K104", "порт подачи 1"}},
        {"AR2", {"K109", "порт подачи 2"}},
        {"AR3", {"K114", "порт подачи 3"}},
        {"AR4", {"K118", "сброс в атмосферу"}},
        {"AR5", {"K178", "магистраль"}},
        {"SL2", {"K179", "турбомолекулярный насос"}},
        {"AR6", {"K176", "форвакуум"}},
        {"SL1", {"K192", "выход второго тракта"}},
        {"S4",  {"K171", "диапазон резервуара"}},
        {"S1",  {"K131", "ёмкость малая"}},
        {"S2",  {"K133", "ёмкость средняя"}},
        {"S3",  {"K135", "ёмкость большая"}},
        {"R1",  {"K153", "натекатель средний"}},
        {"R2",  {"K155", "натекатель медленный"}},
        {"R3",  {"K151", "линия E–F"}},
        {"R4",  {"K173", "диапазон камеры"}},
    };
    return table;
}

QString ruName(const QString &name)
{
    QString out = name;
    if (out.startsWith(u"DD")) out.replace(0, 2, QStringLiteral("ДД"));
    else if (out.startsWith(u"DT")) out.replace(0, 2, QStringLiteral("ДТ"));
    else if (out.startsWith(u"DV")) out.replace(0, 2, QStringLiteral("ДВ"));
    return out;
}

} // namespace

QString valveNumberForCode(const QString &code)
{
    const auto it = valveTable().constFind(code);
    return it == valveTable().constEnd() ? QString() : QString::fromLatin1(it->number);
}

CatalogBuild buildCatalog(const CatalogInput &input)
{
    CatalogBuild out;
    Catalog &c = out.catalog;

    c.zones = {
        {"supply", "Линия подачи"},
        {"accumulator", "Аккумуляционный резервуар"},
        {"chamber", "Камера / реакционная зона"},
        {"vacuum", "Вакуумная линия"},
    };

    for (int i = 0; i < input.pressureCard.size(); ++i) {
        const auto &s = input.pressureCard[i];
        const ZoneInfo zone = zoneForSensor(s.name);
        const bool temperature = s.name.startsWith(u"DT");
        // Диапазон датчика — токовая петля: 4 мА → 4A+B, 20 мА → 20A+B.
        const double lo = 4.0 * s.A + s.B;
        const double hi = 20.0 * s.A + s.B;

        ChannelDesc ch;
        ch.zone = QString::fromLatin1(zone.zone);
        if (temperature) {
            ch.id = QStringLiteral("T.") + s.name;
            ch.kind = ChannelKind::Temperature;
            ch.min = lo;
            ch.max = hi;
            ch.label = QStringLiteral("%1 (%2), %3…%4 °C").arg(ruName(s.name), QString::fromUtf8(zone.label))
                           .arg(lo, 0, 'g', 4).arg(hi, 0, 'g', 4);
            ch.defaultValue = std::clamp(25.0, ch.min, ch.max);
        } else {
            ch.id = QStringLiteral("P.") + s.name;
            ch.kind = ChannelKind::Pressure;
            // Давление абсолютное: отрицательных значений не бывает.
            ch.min = std::max(0.0, lo * 1e5);
            ch.max = hi * 1e5;
            ch.label = QStringLiteral("%1 (%2), до %3 бар").arg(ruName(s.name), QString::fromUtf8(zone.label))
                           .arg(hi, 0, 'g', 3);
            ch.defaultValue = std::clamp(kAtmospherePa, ch.min, ch.max);
        }
        c.channels.append(ch);
        out.bindings.append({ch.id, ChannelBinding::Card::Pressure, i, s.A, s.B, s.R});
    }

    for (int i = 0; i < input.temperatureCard.size(); ++i) {
        const QString &name = input.temperatureCard[i];
        const ZoneInfo zone = zoneForSensor(name);
        ChannelDesc ch;
        ch.id = QStringLiteral("T.") + name;
        ch.kind = ChannelKind::Temperature;
        ch.zone = QString::fromLatin1(zone.zone);
        ch.label = QStringLiteral("%1 (%2), термопара").arg(ruName(name), QString::fromUtf8(zone.label));
        ch.min = -200.0;
        ch.max = 1200.0;
        ch.defaultValue = 25.0;
        c.channels.append(ch);
        out.bindings.append({ch.id, ChannelBinding::Card::Temperature, i, 1.0, 0.0, 0.0});
    }

    const QList<std::pair<QString, QString>> gauges = {
        {QString::fromLatin1(kGaugeFore), QStringLiteral("форвакуум")},
        {QString::fromLatin1(kGaugeTurbo), QStringLiteral("турбо, второй тракт")},
    };
    for (int i = 0; i < gauges.size(); ++i) {
        ChannelDesc ch;
        ch.id = QStringLiteral("VAC.") + gauges[i].first;
        ch.kind = ChannelKind::Vacuum;
        ch.zone = QStringLiteral("vacuum");
        ch.label = QStringLiteral("%1 (%2)").arg(ruName(gauges[i].first), gauges[i].second);
        ch.min = kGaugeMinTorr * kTorrToPa;
        ch.max = kGaugeMaxTorr * kTorrToPa;
        ch.scale = Scale::Log;
        ch.defaultValue = kAtmospherePa;
        c.channels.append(ch);
        out.bindings.append({ch.id, ChannelBinding::Card::Gauge, i, 1.0, 0.0, 0.0});
    }

    for (const QString &code : input.valveCodes) {
        const auto it = valveTable().constFind(code);
        if (it == valveTable().constEnd())
            continue;
        const QString number = QString::fromLatin1(it->number);
        QString ru = number;
        ru.replace(0, 1, QStringLiteral("К"));
        c.valves.append({number, code, QStringLiteral("%1 (%2) %3").arg(ru, code, QString::fromUtf8(it->label))});
    }
    return out;
}

QJsonObject catalogToJson(const Catalog &catalog,
                          const QHash<QString, double> &values,
                          const QHash<QString, bool> &valveOpen)
{
    QJsonArray channels;
    for (const auto &ch : catalog.channels) {
        channels.append(QJsonObject{
            {"id", ch.id},
            {"kind", toWire(ch.kind)},
            {"zone", ch.zone},
            {"label", ch.label},
            {"unit", unitOf(ch.kind)},
            {"min", ch.min},
            {"max", ch.max},
            {"scale", ch.scale == Scale::Log ? "log" : "linear"},
            {"value", values.value(ch.id, ch.defaultValue)},
        });
    }
    QJsonArray valves;
    for (const auto &v : catalog.valves)
        valves.append(QJsonObject{{"id", v.id}, {"label", v.label},
                                  {"state", valveOpen.value(v.id) ? "open" : "closed"}});
    QJsonArray zones;
    for (const auto &z : catalog.zones)
        zones.append(QJsonObject{{"id", z.id}, {"label", z.label}});
    return QJsonObject{
        {"channels", channels},
        {"valves", valves},
        {"zones", zones},
        {"kinds", QJsonArray{"static", "pumping", "gasInlet", "h2Inlet"}},
    };
}

} // namespace sim
