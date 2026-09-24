#pragma once

// Общие типы подсистемы демо-данных (контракт grams.sim/1, раздел 2).
// Только Qt Core: ни железа, ни GUI — используется и в GRAMs, и в тестах.

#include <QList>
#include <QString>

namespace sim {

enum class ChannelKind { Pressure, Temperature, Vacuum };
enum class Scale { Linear, Log };

// Канал на проводе: давление — Па абс., температура — °C (раздел 2.1).
struct ChannelDesc {
    QString id;          // "P.DD311", "T.DT358", "VAC.DV301"
    ChannelKind kind = ChannelKind::Pressure;
    QString zone;        // supply | accumulator | chamber | vacuum
    QString label;
    double min = 0.0;
    double max = 0.0;
    Scale scale = Scale::Linear;
    double defaultValue = 0.0;   // значение до первого прогона
};

struct ValveDesc {
    QString id;          // "K176" — К-номер латиницей
    QString code;        // "AR6"  — имя клапана в ValveControl
    QString label;
};

struct ZoneDesc {
    QString id;
    QString label;
};

// Ошибка или предупреждение валидации профиля (раздел 2.2, Issue).
struct Issue {
    QString path;
    QString code;
    QString message;
};

struct Catalog {
    QList<ChannelDesc> channels;
    QList<ValveDesc> valves;
    QList<ZoneDesc> zones;

    const ChannelDesc *channel(const QString &id) const {
        for (const auto &c : channels)
            if (c.id == id) return &c;
        return nullptr;
    }
    const ValveDesc *valve(const QString &id) const {
        for (const auto &v : valves)
            if (v.id == id) return &v;
        return nullptr;
    }
    const ValveDesc *valveByCode(const QString &code) const {
        for (const auto &v : valves)
            if (v.code == code) return &v;
        return nullptr;
    }
    bool hasZone(const QString &id) const {
        for (const auto &z : zones)
            if (z.id == id) return true;
        return false;
    }
};

inline QString toWire(ChannelKind k) {
    switch (k) {
    case ChannelKind::Pressure:    return QStringLiteral("pressure");
    case ChannelKind::Temperature: return QStringLiteral("temperature");
    case ChannelKind::Vacuum:      return QStringLiteral("vacuum");
    }
    return {};
}

inline QString unitOf(ChannelKind k) {
    return k == ChannelKind::Temperature ? QStringLiteral("degC") : QStringLiteral("Pa");
}

inline bool isPressureLike(ChannelKind k) {
    return k == ChannelKind::Pressure || k == ChannelKind::Vacuum;
}

} // namespace sim
