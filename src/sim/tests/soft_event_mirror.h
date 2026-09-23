#pragma once

// Зеркало части Grams, которая относится к Security: какие давления уходят в
// Security на такте softEvent, что Security делает с клапанами, и как он
// настраивается в initSafeModule.
//
// Grams — это QApplication с платами, графиками и QML; в тест его не поднять.
// Поэтому интеграционные риги вызывают эти функции, а они повторяют Grams
// строка в строку. МЕНЯТЬ ТОЛЬКО ВМЕСТЕ С src/Grams.cpp (softEvent,
// initSafeModule, хвост конструктора) и в том же коммите — иначе тесты
// проверяют не то, что работает в приложении.

#include "Security.h"
#include "ValveControl.h"
#include "core/SimCatalog.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace simtest {

// Зеркало Grams::initSafeModule: те же поля профиля и в том же порядке, как
// их читает Initialize. В частности, клапан сброса берётся из
// storageQuar.v_gasRelease (Initialize::fillStorageQuar), а не из
// addRemoveQuar — как в приложении, даже если в профиле его там нет.
inline void initSafeModuleMirror(Security &security, const QJsonObject &gram,
                                 const QList<sim::PressureCardSensor> &pressureCard)
{
    const QJsonObject sec = gram.value("security").toObject();
    QMap<QString, QStringList> contradictions;
    const QJsonObject cv = sec.value("contradictionValves").toObject();
    for (auto it = cv.begin(); it != cv.end(); ++it)
        contradictions.insert(it.key(), it.value().toVariant().toStringList());
    security.setContradictionValves(contradictions);
    security.setPressureStaleTicks(sec.value("pressureStaleTicks").toInt(4));
    for (const auto &s : pressureCard)
        security.setSensorRange(s.name, 3.8 * s.A + s.B, 20.5 * s.A + s.B);
    security.setRuleOfThreeValves(sec.value("twoOfThree").toVariant().toStringList());

    const QJsonObject quars = gram.value("quartiles").toObject();
    const QJsonObject storage = quars.value("storageQuar").toObject();
    const QJsonObject reaction = quars.value("reactionQuar").toObject();
    security.setRangePressureValves(storage.value("v_pressureRange").toString(), "storageQuar",
                                    storage.value("cond_pressureRange_open").toDouble(),
                                    storage.value("cond_pressureRange_close").toDouble());
    security.setRangePressureValves(reaction.value("v_pressureRange").toString(), "reactionQuar",
                                    reaction.value("cond_pressureRange_open").toDouble(),
                                    reaction.value("cond_pressureRange_close").toDouble());
    security.setSafeReleaseValves(storage.value("v_gasRelease").toString(), "storageQuar",
                                  storage.value("cond_gasRelease").toDouble());
    security.setGasSupplyValves(quars.value("addRemoveQuar").toObject().value("v_gasSupply")
                                    .toVariant().toStringList());
    security.setGasLeakageValves(reaction.value("v_gasLeakage").toVariant().toStringList());
    security.setTransferValves(reaction.value("v_gasLeakage").toVariant().toStringList(),
                               QStringLiteral("storageQuar"), QStringLiteral("reactionQuar"));
}

// Подключённые объёмы, см³ — как StorageQuartile/ReactionQuartile::getUsedVolumes
// по profile/addons.json, плюс камера из Grams::chamberSetUp (подключена и
// открыта при старте): F = 25,405, EF = 26,1327 − 25,7941.
struct UsedVolumes {
    double storage = 0.0;
    double reaction = 0.0;
};

inline UsedVolumes usedVolumes(const ValveControl &valves)
{
    static const QJsonObject addons = [] {
        QFile f(QStringLiteral(GRAMS_ADDONS_JSON));
        f.open(QIODevice::ReadOnly);
        return QJsonDocument::fromJson(f.readAll()).object();
    }();
    auto quartile = [&](const char *name, double extra) {
        const QJsonObject q = addons.value(name).toObject();
        const QJsonObject volume = q.value("volume").toObject();
        double sum = volume.value(q.value("mainVolume").toString()).toDouble() + extra;
        const QJsonObject toValve = q.value("volumeToValve").toObject();
        for (auto it = toValve.begin(); it != toValve.end(); ++it)
            if (valves.valveState(it.value().toString()))
                sum += volume.value(it.key()).toDouble();
        return sum;
    };
    return {quartile("storageQuar", 0.0), quartile("reactionQuar", 25.405 + (26.1327 - 25.7941))};
}

// Отсчёт датчика давления на такте: имя, бар (ControllerData::getCurValue),
// счётчик отсчётов (ControllerData::sampleCount).
struct SensorReading {
    QString name;
    double bar = 0.0;
    quint64 seq = 0;
};

// Датчики квартилей GRAM50: prSH/prSA — накопитель, prRH/prRA — реакционная
// область (Grams::initStorageQuartile / initReactionQuartile).
struct SoftEventInput {
    SensorReading dd311, dd312, dd331, dd332;
};

inline PressureSample toSample(const SensorReading &r)
{
    return PressureSample{r.name, r.bar, r.seq};
}

// Grams::softEvent: резервуар видят широкодиапазонный датчик всегда и
// узкодиапазонный — при открытом клапане диапазона; затем Security сам
// закрывает S4/R4 на этом же такте.
inline void softEventSecurity(Security &security, ValveControl &valves, const SoftEventInput &in)
{
    QMap<QString, QuartileSnapshot> quartiles;
    quartiles[QStringLiteral("storageQuar")].sensors << toSample(in.dd311);
    if (valves.valveState(QStringLiteral("S4")))
        quartiles[QStringLiteral("storageQuar")].sensors << toSample(in.dd312);
    quartiles[QStringLiteral("reactionQuar")].sensors << toSample(in.dd331);
    if (valves.valveState(QStringLiteral("R4")))
        quartiles[QStringLiteral("reactionQuar")].sensors << toSample(in.dd332);
    const UsedVolumes v = usedVolumes(valves);
    quartiles[QStringLiteral("storageQuar")].volumeCm3 = v.storage;
    quartiles[QStringLiteral("reactionQuar")].volumeCm3 = v.reaction;
    security.setQuartilePressures(quartiles);
    valves.enforceSecurity(QStringLiteral("tick"));
}

// Хвост конструктора Grams после initSafeModule: сейчас там ничего нет —
// первое, что Security увидит, это первый такт softEvent.
inline void gramsStartup(Security &security, ValveControl &valves)
{
    Q_UNUSED(security);
    Q_UNUSED(valves);
}

} // namespace simtest
