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

void Security::setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen){
    m_safeReleaseValves.insert(valve, {valve, watchQuartile, pressureOpen});
}

void Security::setSensorRange(const QString &sensor, double minBar, double maxBar){
    m_sensorRange.insert(sensor, {minBar, maxBar});
}

void Security::setPressureStaleTicks(int ticks){
    m_pressureStaleTicks = ticks;
}

void Security::setPressureMap(const QMap<QString, PressureSample> &pressureMap){
    // Квартиль без отсчёта в этом такте — не обновлён.
    for(auto it = m_pressure.begin(); it != m_pressure.end(); ++it)
        if(!pressureMap.contains(it.key()))
            ++it->staleTicks;
    for(auto it = pressureMap.cbegin(); it != pressureMap.cend(); ++it){
        const auto known = m_pressure.find(it.key());
        if(known == m_pressure.end()){
            m_pressure.insert(it.key(), {it.value(), 0});
            continue;
        }
        const bool sameReading = known->sample.sensor == it.value().sensor
                                 && known->sample.seq == it.value().seq;
        known->staleTicks = sameReading ? known->staleTicks + 1 : 0;
        known->sample = it.value();
    }
}

bool Security::pressureOf(const QString &quartile, double *bar, QString *why) const{
    const auto it = m_pressure.constFind(quartile);
    if(it == m_pressure.cend()){
        *why = QStringLiteral("нет данных");
        return false;
    }
    const PressureSample &s = it->sample;
    *bar = s.bar;
    if(std::isnan(s.bar)){
        *why = QStringLiteral("%1: NaN").arg(s.sensor);
        return false;
    }
    if(it->staleTicks > m_pressureStaleTicks){
        *why = QStringLiteral("%1: нет нового отсчёта %2 тактов (допустимо %3)")
                   .arg(s.sensor).arg(it->staleTicks).arg(m_pressureStaleTicks);
        return false;
    }
    const auto range = m_sensorRange.constFind(s.sensor);
    if(range != m_sensorRange.cend() && (s.bar < range->first || s.bar > range->second)){
        *why = QStringLiteral("%1 = %2 бар вне диапазона датчика [%3; %4]")
                   .arg(s.sensor).arg(s.bar).arg(range->first).arg(range->second);
        return false;
    }
    return true;
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
        double p = 0.0;
        QString why;
        if(!pressureOf(quartile, &p, &why))
            return refuse(QStringLiteral("pressure_invalid"), quartile + u": " + why);
        const auto range = m_rangePressureValves.constFind(sender);
        if(range != m_rangePressureValves.cend() && p > range->m_pressureClose)
            return refuse(QStringLiteral("pressure_range"),
                          QStringLiteral("%1 = %2 бар > %3").arg(quartile).arg(p).arg(range->m_pressureClose));
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

QString SecurityIssue::toString() const{
    return QStringLiteral("%1: %2 (%3 = %4 бар, порог %5)")
        .arg(valve, reason, quartile).arg(pressure).arg(limit);
}

PressureCheck Security::checkPressure(const QMap<QString, bool> &valveStates) const{
    PressureCheck check;

    // Недостоверное давление у открытого клапана: не закрываем, сообщаем один
    // раз до восстановления достоверности.
    auto invalidOpen = [&](const QString &valve, const QString &quartile, double p, double limit,
                           const QString &why){
        if(m_invalidReported.contains(valve))
            return;
        m_invalidReported.insert(valve);
        qCWarning(lcSecurity) << "checkPressure:" << valve << "открыт, давление" << quartile
                              << "недостоверно:" << why << "— клапан не закрывается";
        check.warnings << SecurityIssue{valve, QStringLiteral("pressure_invalid"), quartile, p, limit};
    };

    for(const auto &rule : m_rangePressureValves){
        if(!valveStates.value(rule.m_selfName)){
            m_invalidReported.remove(rule.m_selfName);
            continue;
        }
        double p = 0.0;
        QString why;
        if(!pressureOf(rule.m_watchQuartile, &p, &why)){
            invalidOpen(rule.m_selfName, rule.m_watchQuartile, p, rule.m_pressureClose, why);
            continue;
        }
        m_invalidReported.remove(rule.m_selfName);
        // Клапан диапазона открыт: выше порога закрытия он обязан быть закрыт.
        if(p > rule.m_pressureClose)
            check.violations << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_range"),
                                              rule.m_watchQuartile, p, rule.m_pressureClose};
    }
    for(const auto &rule : m_safeReleaseValves){
        double p = 0.0;
        QString why;
        if(!pressureOf(rule.m_watchQuartile, &p, &why)){
            if(valveStates.value(rule.m_selfName))
                invalidOpen(rule.m_selfName, rule.m_watchQuartile, p, rule.m_gasMax, why);
            else
                m_invalidReported.remove(rule.m_selfName);
            continue;
        }
        m_invalidReported.remove(rule.m_selfName);
        if(p >= rule.m_gasMax)
            check.violations << SecurityIssue{rule.m_selfName, QStringLiteral("pressure_release"),
                                              rule.m_watchQuartile, p, rule.m_gasMax};
    }

    for(const auto &issue : check.violations)
        qCWarning(lcSecurity) << "checkPressure: нарушение" << issue.toString();
    qCDebug(lcSecurity) << "checkPressure: нарушений" << check.violations.size()
                        << "предупреждений" << check.warnings.size();
    return check;
}