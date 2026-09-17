#include "Security.h"

#include <QDebug>

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
bool ValveGraph::applyGraphMask(const QMap<QString, bool> &valveMap){
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

void Security::constructValveMap(const QStringList &valveList){
    QMap<QString, bool> buffValveMap;
    for(const auto& valve : valveList){
        buffValveMap.insert(valve, false);
    }
    m_valveMap = buffValveMap;
    qCDebug(lcSecurity) << "constructValveMap: карта из" << valveList.size() << "клапанов, все закрыты";
}

void Security::setInitialState(const QString &sender, const bool &state){
    m_valveMap[sender] = state;
    qCDebug(lcSecurity) << "setInitialState" << sender << state;
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

bool Security::checkValveAction(const QString &sender, const bool &state){
    if(m_supplyValves.contains(sender)){
        // checks incoming pressure pre-open

    }
    if(m_leakageValves.contains(sender)){
        // checks incoming pressure pre-open
    }
    // if(m_storageValves.contains(sender)){
    // checks incoming pressure pre-open
    // }
    // if(m_pressureRange.contains(sender)){
    // checks opening pressure-range valve
    //}

    if(!m_contradictionValves.contains(sender)){
        qCDebug(lcSecurity) << "checkValveAction" << sender << "запрос" << state
                            << "→" << state << "(нет в интерлоках, карта не обновлена)";
        return state;
    }
    m_valveMap[sender] = state;
    bool imageState = m_valveMap[sender];
    imageState = m_contradictionValves[sender].applyGraphMask(m_valveMap);
    m_valveMap[sender] = imageState;
    if(state && !imageState)
        qCDebug(lcSecurity) << "checkValveAction" << sender << "запрос" << state
                            << "→ ОТКАЗ, по карте открыты:" << openValves(m_valveMap);
    else
        qCDebug(lcSecurity) << "checkValveAction" << sender << "запрос" << state
                            << "→" << imageState;
    return imageState;
}

QMap<QString, bool> Security::checkValvePressure(){
    auto buffValveMap = m_valveMap; 
    for(const auto& valveToRangePressure : m_rangePressureValves.values()){
        const auto& valveName = valveToRangePressure.m_selfName;
        const auto& pressureQuartile = valveToRangePressure.m_watchQuartile;
        buffValveMap[valveName] = valveToRangePressure.applyPressureMask(m_valveMap[valveName], m_pressureQuarMap[pressureQuartile]); // quartileNode! -> upcoming
    }
    for(const auto& valveToSafeRelease : m_safeReleaseValves.values()){
        auto valveName = valveToSafeRelease.m_selfName;
        auto pressureQuartile = valveToSafeRelease.m_watchQuartile;
        buffValveMap[valveName] = valveToSafeRelease.applyPressureMask(m_pressureQuarMap[pressureQuartile]); // quartileNode!
    }
    QStringList falseKeys;
    for (auto it = buffValveMap.cbegin(); it != buffValveMap.cend(); ++it)
        if (!it.value())
            falseKeys << it.key();
    qCDebug(lcSecurity) << "checkValvePressure: давления" << m_pressureQuarMap
                        << "false у" << falseKeys;
    return buffValveMap;
}