#include "Security.h"

ValveGraph::ValveGraph(const QString &selfName): m_selfName(selfName) {}
void ValveGraph::addEachInList(const QStringList &nodeValveList){
    checkEachInList = true;
    nodeValves = nodeValveList;
}
void ValveGraph::addRuleOfThree(const QString &threeNodeOne,const QString &threeNodeTwo){
    checkRuleOfThree = true;
    m_threeNodeOne = threeNodeOne;
    m_threeNodeTwo = threeNodeTwo;
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

bool ValveToRangePressure::applyPressureMask(bool state, double currentPressure, double incomingPressure){
    bool m_state = state;
    if(currentPressure>=m_pressureClose&&incomingPressure>=m_pressureClose){
        m_state = false;
    }
    else if(currentPressure<=m_pressureOpen&&incomingPressure<=m_pressureOpen)
        m_state = true;
    return state;
}

bool ValveToSafeRelease::applyPressureMask(bool state, double currentPressure){
    bool m_state = state;
    if(currentPressure>=m_pressureOpen)
        m_state = true;
    return m_state;
}

Security::Security(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Security class is created";
}

void Security::setContradictionValves(const QVariantMap &contradictionValves, const QVariantList &ruleOfThreeList){
    // valve graph security option from QStringlist values of contradictionValves
    QStringList valves = contradictionValves.keys();
    for(auto valve : valves){
        m_contradictionValves.insert(valve, ValveGraph(valve));
        m_contradictionValves[valve].addEachInList(contradictionValves[valve].toStringList());
    }
    for(auto ruleOfThreeVariant : ruleOfThreeList){
        auto allThree = ruleOfThreeVariant.toStringList();
        for(auto oneOfThree: allThree){
            if(!m_contradictionValves.contains(oneOfThree))
                m_contradictionValves.insert(oneOfThree, ValveGraph(oneOfThree));
            auto twoOfThree = ruleOfThreeVariant.toStringList();
            twoOfThree.removeOne(oneOfThree);
            m_contradictionValves[oneOfThree].addRuleOfThree(twoOfThree.at(0),twoOfThree.at(1));
        }
    }
}

void Security::setRangePressureValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen, const double &pressureClose){
    m_rangePressureValves.insert(valve, ValveToRangePressure{valve, watchQuartile, pressureOpen, pressureClose});
}
void Security::setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen){
    m_safeReleaseValves.insert(valve, ValveToSafeRelease{valve, watchQuartile, pressureOpen});
}

bool Security::checkValveAction(const QMap<QString, bool> &valveMap, const QString &sender, const bool &state){
    auto imageValveMap = valveMap;
    imageValveMap[sender] = state;
    bool imageState = imageValveMap[sender];
    if(m_contradictionValves.contains(sender)){
        imageState = m_contradictionValves[sender].applyGraphMask(imageValveMap);
    }
    return imageState;
}

QMap<QString, bool> Security::checkValvePressure(const QMap<QString, bool> &valveMap, const QMap<QString, double> &pressureMap){
    auto cur_valveMap = valveMap;
    for(auto valveToRangePressure : m_rangePressureValves.values()){
        auto valveName = valveToRangePressure.m_selfName;
        auto pressureQuartile = valveToRangePressure.m_watchQuartile;
        auto result = valveToRangePressure.applyPressureMask(valveMap[valveName], pressureMap[pressureQuartile], pressureMap[pressureQuartile]); // quartileNode!
        cur_valveMap[valveName] = result;
    }
    for(auto valveToSafeRelease : m_safeReleaseValves.values()){
        auto valveName = valveToSafeRelease.m_selfName;
        auto pressureQuartile = valveToSafeRelease.m_watchQuartile;
        auto result = valveToSafeRelease.applyPressureMask(valveMap[valveName], pressureMap[pressureQuartile]); // quartileNode!
        cur_valveMap[valveName] = result;
    }
    return cur_valveMap;
}