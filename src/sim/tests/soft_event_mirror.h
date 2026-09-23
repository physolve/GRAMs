#pragma once

// Зеркало части Grams::softEvent, которая относится к Security: какие
// давления уходят в Security на такте и что Security делает с клапанами.
//
// Grams — это QApplication с платами, графиками и QML; в тест его не поднять.
// Поэтому интеграционные риги вызывают эту функцию, а она повторяет
// Grams::softEvent строка в строку. МЕНЯТЬ ТОЛЬКО ВМЕСТЕ С Grams::softEvent
// (src/Grams.cpp) и в том же коммите — иначе тесты проверяют не то, что
// работает в приложении.

#include "Security.h"
#include "ValveControl.h"
#include "core/SimCatalog.h"

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

// Grams::softEvent: квартиль берёт узкодиапазонный датчик, пока открыт его
// клапан диапазона (StorageQuartile/ReactionQuartile::updateQuartileData),
// и отдаёт Security один отсчёт на такт.
inline void softEventSecurity(Security &security, ValveControl &valves, const SoftEventInput &in)
{
    QMap<QString, PressureSample> pressures;
    pressures.insert(QStringLiteral("storageQuar"),
                     toSample(valves.valveState(QStringLiteral("S4")) ? in.dd312 : in.dd311));
    pressures.insert(QStringLiteral("reactionQuar"),
                     toSample(valves.valveState(QStringLiteral("R4")) ? in.dd332 : in.dd331));
    security.setPressureMap(pressures);
}

// Хвост конструктора Grams после initSafeModule: сейчас там ничего нет —
// первое, что Security увидит, это первый такт softEvent.
inline void gramsStartup(Security &security, ValveControl &valves)
{
    Q_UNUSED(security);
    Q_UNUSED(valves);
}

} // namespace simtest
