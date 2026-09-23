#include "Security.h"

#include <QDebug>

#include <cmath>

Q_LOGGING_CATEGORY(lcSecurity, "grams.security")

namespace {
QStringList openValves(const QMap<QString, bool> &valveMap)
{
    QStringList open;
    for (auto it = valveMap.cbegin(); it != valveMap.cend(); ++it)
        if (it.value())
            open << it.key();
    return open;
}
} // namespace

ValveGraph::ValveGraph(const QString &selfName): m_selfName(selfName), checkEachInList(false), checkRuleOfThree(false) {}
void ValveGraph::addEachInList(const QStringList &nodeValveList){
    checkEachInList = true;
    exclusionValves = nodeValveList;
}
void ValveGraph::addRuleOfThree(const QString &threeNodeOne,const QString &threeNodeTwo){
    checkRuleOfThree = true;
    m_threeNodeOne = threeNodeOne;
    m_threeNodeTwo = threeNodeTwo;
}
bool ValveGraph::isExclusion() const{
    return checkEachInList||checkRuleOfThree;
}

bool ValveGraph::maskEachInList(const QMap<QString, bool> &valveMap) const{
    for(const auto& exclusionValve : exclusionValves){
        if(valveMap[exclusionValve]){
            return false;
        }
    }
    return true;
}
bool ValveGraph::maskRuleOfThree(const QMap<QString, bool> &valveMap) const{
    if(valveMap[m_threeNodeOne]&&valveMap[m_threeNodeTwo])
        return false;
    return true;
}
bool ValveGraph::applyGraphMask(const QMap<QString, bool> &valveMap) const{
    if(!valveMap[m_selfName]) 
        return false;
    bool resultEach = true;
    if(checkEachInList)
        resultEach = maskEachInList(valveMap);
    
    bool resultRuleOfThree = true;
    if(checkRuleOfThree)
        resultRuleOfThree = maskRuleOfThree(valveMap);
    bool result = resultEach*resultRuleOfThree;
    return result;
}

bool ReactionToSupply::applyPressureMask(bool &rangePressureState, double &safeReleaseState, double incomingPressure) const{
    // three step check
    // incoming - node value
    rangePressureState = m_rangePressure.applyPressureMask(rangePressureState, incomingPressure);
    safeReleaseState = m_safeRelease.applyPressureMask(incomingPressure);
    return !safeReleaseState;
}

bool ReactionToLeakage::applyPressureMask(bool &rangePressureState, double incomingPressure) const{
    rangePressureState = m_rangePressure.applyPressureMask(rangePressureState, incomingPressure);
    return incomingPressure<chamberMax&&incomingPressure<gasMax;
}

bool ValveToRangePressure::applyPressureMask(bool currentState, double incomingPressure) const{
    if(incomingPressure < m_pressureOpen)
        return true;
    else if (incomingPressure > m_pressureClose)
        return false;
    else
        return currentState;
}

bool ValveToSafeRelease::applyPressureMask(double currentPressure) const{
    return currentPressure>=m_gasMax;
}

// security module should be running in timer (move to thread)
// also methods from Security to check valves

Security::Security(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Security class is created";

}

void Security::setContradictionValves(const QMap<QString, QStringList> &contradictionValves){
    for(const auto& [name, list] : contradictionValves.asKeyValueRange()){
        m_contradictionValves.insert(name, ValveGraph(name));
        m_contradictionValves[name].addEachInList(list);
    }
}

void Security::setRuleOfThreeValves(const QStringList &ruleOfThreeList){
    if(ruleOfThreeList.size() < 3){
        qWarning() << "Security: twoOfThree требует ровно три клапана, получено"
                   << ruleOfThreeList;
        return;
    }
    // ВАЖНО: имя клапана обязано попасть в ValveGraph.
    //
    // Раньше здесь стоял голый operator[], который для отсутствующего ключа
    // default-конструирует ValveGraph с m_selfName == "unknown". Дальше
    // applyGraphMask делает `if(!valveMap[m_selfName]) return false;`, а
    // valveMap["unknown"] не существует и всегда false — то есть открыть такой
    // клапан было НЕВОЗМОЖНО. Под это попадали SL2 (К179, турбонасос) и SL1
    // (К192): в contradictionValves профиля их нет, они приходят только через
    // twoOfThree. Ф3 «второй тракт» из-за этого не могла открыть К179.
    for(const QString& name : ruleOfThreeList){
        if(!m_contradictionValves.contains(name))
            m_contradictionValves.insert(name, ValveGraph(name));
    }
    m_contradictionValves[ruleOfThreeList[0]].addRuleOfThree(ruleOfThreeList[1],ruleOfThreeList[2]);
    m_contradictionValves[ruleOfThreeList[1]].addRuleOfThree(ruleOfThreeList[0],ruleOfThreeList[2]);
    m_contradictionValves[ruleOfThreeList[2]].addRuleOfThree(ruleOfThreeList[0],ruleOfThreeList[1]);
}

void Security::setGasSupplyValves(const QStringList &gasSupplyList){
    QString v_pressureRange = "S4"; // from profile
    QString v_gasRelease = "AR4"; // from profile, only one?
    for(const auto& valve : gasSupplyList){
        // should be quartile m_rangePressureValves and one? m_safeReleaseValves
        m_supplyValves.insert(valve, {valve,m_rangePressureValves[v_pressureRange],m_safeReleaseValves[v_gasRelease]});
    }
}

void Security::setGasLeakageValves(const QStringList &gasLeakageList){
    QString v_pressureRange = "R4"; // from profile
    double gasMax = 50; // from profile
    double chamberMax = 50; // Chamber object
    for(const auto& valve : gasLeakageList){
        m_leakageValves.insert(valve, {valve,m_rangePressureValves[v_pressureRange],gasMax,chamberMax});
    }
}

void Security::setRangePressureValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen, const double &pressureClose){
    m_rangePressureValves.insert(valve, {valve, watchQuartile, pressureOpen, pressureClose});
}

void Security::setTransferValves(const QStringList &valves, const QString &quartileA, const QString &quartileB){
    m_transferValves = valves;
    m_transferA = quartileA;
    m_transferB = quartileB;
}

void Security::setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen){
    m_safeReleaseValves.insert(valve, {valve, watchQuartile, pressureOpen});
}

void Security::setSensorRange(const QString &sensor, double minBar, double maxBar){
    m_sensorRange.insert(sensor, {minBar, maxBar});
}

void Security::setPressureStaleTicks(int ticks){
    m_pressureStaleTicks = ticks;
}

void Security::setQuartilePressures(const QMap<QString, QuartileSnapshot> &quartiles){
    QSet<QString> updated;
    for(auto q = quartiles.cbegin(); q != quartiles.cend(); ++q){
        QStringList names;
        for(const PressureSample &sample : q->sensors){
            names << sample.sensor;
            updated << sample.sensor;
            const auto known = m_sensors.find(sample.sensor);
            if(known == m_sensors.end()){
                m_sensors.insert(sample.sensor, {sample, 0});
                continue;
            }
            const bool sameReading = known->sample.seq == sample.seq;
            known->staleTicks = sameReading ? known->staleTicks + 1 : 0;
            known->sample = sample;
        }
        m_quartileSensors.insert(q.key(), names);
        m_quartileVolume.insert(q.key(), q->volumeCm3);
    }
    // Датчик без отсчёта в этом такте — не обновлён.
    for(auto it = m_sensors.begin(); it != m_sensors.end(); ++it)
        if(!updated.contains(it.key()))
            ++it->staleTicks;
}

bool Security::sensorValid(const SensorState &state, QString *why) const{
    const PressureSample &s = state.sample;
    if(std::isnan(s.bar)){
        *why = QStringLiteral("NaN");
        return false;
    }
    if(state.staleTicks > m_pressureStaleTicks){
        *why = QStringLiteral("нет нового отсчёта %1 тактов (допустимо %2)")
                   .arg(state.staleTicks).arg(m_pressureStaleTicks);
        return false;
    }
    const auto range = m_sensorRange.constFind(s.sensor);
    if(range != m_sensorRange.cend() && (s.bar < range->first || s.bar > range->second)){
        *why = QStringLiteral("%1 бар вне диапазона датчика [%2; %3]")
                   .arg(s.bar).arg(range->first).arg(range->second);
        return false;
    }
    return true;
}

QuartileReading Security::quartilePressure(const QString &quartile) const{
    QuartileReading r;
    const QStringList names = m_quartileSensors.value(quartile);
    if(names.isEmpty()){
        r.invalid << qMakePair(QString(), QStringLiteral("нет данных"));
        return r;
    }
    for(const QString &name : names){
        const SensorState state = m_sensors.value(name);
        QString why;
        if(!sensorValid(state, &why)){
            r.invalid << qMakePair(name, why);
            continue;
        }
        if(!r.valid || state.sample.bar > r.bar){
            r.valid = true;
            r.bar = state.sample.bar;
            r.sensor = name;
        }
    }
    return r;
}

void Security::reportInvalid(const QString &valve, const QString &quartile, const QuartileReading &reading,
                             double limit, QList<SecurityIssue> *out) const{
    // Одно предупреждение на клапан за эпизод: пока хоть один датчик его
    // резервуара недостоверен. Все датчики восстановились — эпизод закончен.
    if(reading.invalid.isEmpty()){
        m_invalidReported.remove(valve);
        return;
    }
    if(m_invalidReported.contains(valve))
        return;
    m_invalidReported.insert(valve);
    QStringList sensors, whys;
    for(const auto &[sensor, why] : reading.invalid){
        sensors << sensor;
        whys << (sensor.isEmpty() ? why : sensor + u": " + why);
    }
    qCWarning(lcSecurity) << valve << "открыт, показание" << quartile << "недостоверно:"
                          << whys.join(u"; ") << "— по нему клапан не закрывается";
    if(out)
        *out << SecurityIssue{valve, QStringLiteral("pressure_invalid"), quartile, reading.bar, limit,
                              sensors.join(u','), {}};
}

void Security::forgetInvalid(const QString &valve) const{
    m_invalidReported.remove(valve);
}

QString Security::watchedQuartile(const QString &valve) const{
    if(const auto r = m_rangePressureValves.constFind(valve); r != m_rangePressureValves.cend())
        return r->m_watchQuartile;
    if(const auto r = m_safeReleaseValves.constFind(valve); r != m_safeReleaseValves.cend())
        return r->m_watchQuartile;
    return {};
}

bool Security::checkValveAction(const QString &sender, const bool &state,
                                const QMap<QString, bool> &valveStates, QString *reason) const{
    auto refuse = [&](const QString &code, const QString &detail){
        qCWarning(lcSecurity) << "checkValveAction" << sender << "открытие → ОТКАЗ" << code << detail;
        if(reason)
            *reason = code;
        return false;
    };
    if(reason)
        reason->clear();

    // Правила по давлению — только для открытия; закрыть можно всегда.
    const QString quartile = watchedQuartile(sender);
    if(state && !quartile.isEmpty()){
        const QuartileReading r = quartilePressure(quartile);
        if(!r.valid){
            QStringList whys;
            for(const auto &[sensor, why] : r.invalid)
                whys << (sensor.isEmpty() ? why : sensor + u": " + why);
            return refuse(QStringLiteral("pressure_invalid"), quartile + u": " + whys.join(u"; "));
        }
        // Гистерезис 1,6–1,8: клапан диапазона открывается только ниже порога
        // открытия — иначе он закрылся бы автоматически от малейшего роста.
        const auto range = m_rangePressureValves.constFind(sender);
        if(range != m_rangePressureValves.cend() && r.bar >= range->m_pressureOpen)
            return refuse(QStringLiteral("pressure_range"),
                          QStringLiteral("%1 %2 = %3 бар ≥ порога открытия %4")
                              .arg(quartile, r.sensor).arg(r.bar).arg(range->m_pressureOpen));
    }

    const auto graph = m_contradictionValves.constFind(sender);
    if(graph == m_contradictionValves.cend()){
        qCDebug(lcSecurity) << "checkValveAction" << sender << "запрос" << state
                            << "→" << state << "(нет в интерлоках)";
        return state;
    }
    // Проверяется состояние «после команды»: факт остальных клапанов (его
    // ведёт ValveControl — плата при старте, откат записи, readback) плюс
    // запрошенное состояние отправителя. Security своей копии не держит.
    QMap<QString, bool> image = valveStates;
    image[sender] = state;
    const bool imageState = graph->applyGraphMask(image);
    if(state && !imageState)
        return refuse(QStringLiteral("interlock"),
                      QStringLiteral("открыты: %1").arg(openValves(valveStates).join(u',')));
    qCDebug(lcSecurity) << "checkValveAction" << sender << "запрос" << state << "→" << imageState;
    return imageState;
}

QString toString(ValveSource source){
    switch(source){
    case ValveSource::None:   return QStringLiteral("none");
    case ValveSource::Manual: return QStringLiteral("manual");
    case ValveSource::Regime: return QStringLiteral("regime");
    case ValveSource::Board:  return QStringLiteral("board");
    }
    return QStringLiteral("?");
}

QString SecurityIssue::toString() const{
    const QString where = sensor.isEmpty() ? quartile : quartile + u' ' + sensor;
    QString text = QStringLiteral("%1: %2 (%3 = %4 бар, порог %5)")
                       .arg(valve, reason, where).arg(pressure).arg(limit);
    if(!detail.isEmpty())
        text += QStringLiteral("; ") + detail;
    return text;
}

PressureCheck Security::checkPressure(const QMap<QString, bool> &valveStates) const{
    PressureCheck check;
    for(const auto &rule : m_rangePressureValves){
        if(!valveStates.value(rule.m_selfName)){
            forgetInvalid(rule.m_selfName);
            continue;
        }
        const QuartileReading r = quartilePressure(rule.m_watchQuartile);
        reportInvalid(rule.m_selfName, rule.m_watchQuartile, r, rule.m_pressureClose, &check.warnings);
        // Клапан диапазона открыт: выше порога закрытия он обязан быть закрыт.
        if(r.valid && r.bar > rule.m_pressureClose)
            check.violations << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_range"),
                                              rule.m_watchQuartile, r.bar, rule.m_pressureClose, r.sensor, {}};
    }
    for(const auto &rule : m_safeReleaseValves){
        const QuartileReading r = quartilePressure(rule.m_watchQuartile);
        if(valveStates.value(rule.m_selfName))
            reportInvalid(rule.m_selfName, rule.m_watchQuartile, r, rule.m_gasMax, &check.warnings);
        else
            forgetInvalid(rule.m_selfName);
        if(r.valid && r.bar >= rule.m_gasMax)
            check.violations << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_release"),
                                              rule.m_watchQuartile, r.bar, rule.m_gasMax, r.sensor, {}};
    }

    for(const auto &issue : check.violations)
        qCWarning(lcSecurity) << "checkPressure: нарушение" << issue.toString();
    qCDebug(lcSecurity) << "checkPressure: нарушений" << check.violations.size()
                        << "предупреждений" << check.warnings.size();
    return check;
}

QList<SecurityIssue> Security::rangeValveClosures(const QMap<QString, bool> &valveStates,
                                                  const QMap<QString, ValveSource> &sources,
                                                  bool regimeActive,
                                                  const QString &origin,
                                                  QList<SecurityIssue> *warnings) const{
    QList<SecurityIssue> closures;
    for(const auto &rule : m_rangePressureValves){
        if(!valveStates.value(rule.m_selfName)){
            forgetInvalid(rule.m_selfName);
            continue;
        }
        // Открытым клапан диапазона держат только оператор и идущий режим.
        const ValveSource source = sources.value(rule.m_selfName, ValveSource::None);
        const bool authorized = source == ValveSource::Manual
                                || (source == ValveSource::Regime && regimeActive);
        if(!authorized){
            const QString why = source == ValveSource::Regime
                                    ? QStringLiteral("открыт режимом, режим не идёт")
                                    : QStringLiteral("открыт без команды (источник %1)").arg(toString(source));
            closures << SecurityIssue{rule.m_selfName, QStringLiteral("unauthorized_open"),
                                      rule.m_watchQuartile, 0.0, rule.m_pressureClose, {}, origin, why};
            forgetInvalid(rule.m_selfName);
            continue;
        }
        const QuartileReading r = quartilePressure(rule.m_watchQuartile);
        reportInvalid(rule.m_selfName, rule.m_watchQuartile, r, rule.m_pressureClose, warnings);
        if(r.valid && r.bar > rule.m_pressureClose)
            closures << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_range_autoclose"),
                                      rule.m_watchQuartile, r.bar, rule.m_pressureClose, r.sensor, origin};
    }
    return closures;
}

QList<SecurityIssue> Security::transferClosures(const QMap<QString, bool> &valveStates,
                                                const QString &origin,
                                                QList<SecurityIssue> *warnings) const{
    QList<SecurityIssue> closures;
    QStringList openTransfer;
    for(const QString &valve : m_transferValves)
        if(valveStates.value(valve))
            openTransfer << valve;

    for(const auto &rule : m_rangePressureValves){
        const QString key = rule.m_selfName + QStringLiteral("/transfer");
        if(openTransfer.isEmpty() || !valveStates.value(rule.m_selfName)){
            m_invalidReported.remove(key);
            continue;
        }
        const QString own = rule.m_watchQuartile;
        const QString other = own == m_transferA ? m_transferB
                            : own == m_transferB ? m_transferA : QString();
        if(other.isEmpty())
            continue;
        const QuartileReading pOwn = quartilePressure(own);
        const QuartileReading pOther = quartilePressure(other);
        const double vOwn = m_quartileVolume.value(own);
        const double vOther = m_quartileVolume.value(other);
        if(!pOwn.valid || !pOther.valid || vOwn <= 0.0 || vOther <= 0.0){
            // Прогноз невозможен — по одной недостоверности не закрываем (D6).
            if(!m_invalidReported.contains(key)){
                m_invalidReported.insert(key);
                qCWarning(lcSecurity) << rule.m_selfName << "и" << openTransfer.join(u',')
                                      << "открыты, прогноз равновесия невозможен: давление"
                                      << own << (pOwn.valid ? "есть" : "недостоверно") << "," << other
                                      << (pOther.valid ? "есть" : "недостоверно") << ", объёмы" << vOwn << vOther
                                      << "— клапан не закрывается";
                if(warnings)
                    *warnings << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_invalid"), other,
                                               0.0, rule.m_pressureClose, {}, origin,
                                               QStringLiteral("прогноз через %1").arg(openTransfer.join(u','))};
            }
            continue;
        }
        m_invalidReported.remove(key);
        const double pEq = (pOwn.bar * vOwn + pOther.bar * vOther) / (vOwn + vOther);
        const QString detail = QStringLiteral("через %1: %2 %3 = %4 бар × %5 см³, %6 %7 = %8 бар × %9 см³")
                                   .arg(openTransfer.join(u','), own, pOwn.sensor).arg(pOwn.bar).arg(vOwn)
                                   .arg(other, pOther.sensor).arg(pOther.bar).arg(vOther);
        qCDebug(lcSecurity) << "прогноз равновесия" << rule.m_selfName << pEq << "бар," << detail;
        if(pEq > rule.m_pressureClose)
            closures << SecurityIssue{rule.m_selfName, QStringLiteral("transfer_equilibrium"), own, pEq,
                                      rule.m_pressureClose, pOwn.sensor + u'+' + pOther.sensor, origin, detail};
    }
    return closures;
}
