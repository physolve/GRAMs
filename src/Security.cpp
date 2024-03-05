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
        return;
    bool resultEach = true;
    if(checkEachInList)
        resultEach = maskEachInList(valveMap);
    bool resultRuleOfThree = true;
    if(checkRuleOfThree)
        resultRuleOfThree = maskRuleOfThree(valveMap);
    bool result = resultEach*resultRuleOfThree;
    return result;
}

ValveToRangePressure::ValveToRangePressure(const QString &selfName, double pressureOpen, double pressureClose)
: m_selfName(selfName), m_pressureOpen(pressureOpen), m_pressureClose(pressureClose) {}
bool ValveToRangePressure::applyPressureMask(bool state, double currentPressure, double incomingPressure){
    bool m_state = state;
    if(currentPressure>=m_pressureClose&&incomingPressure>=m_pressureClose){
        m_state = false;
    }
    else if(currentPressure<=m_pressureOpen&&incomingPressure<=m_pressureOpen)
        m_state = true;
    return state;
}

ValveToSafePressure::ValveToSafePressure(const QString &selfName, double pressureOpen)
: m_selfName(selfName), m_pressureOpen(pressureOpen) {}
bool ValveToSafePressure::applyPressureMask(bool state, double currentPressure){
    bool m_state = state;
    if(currentPressure>=m_pressureOpen)
        m_state = true;
    return m_state;
}

Security::Security(QObject *parent) :
    QObject(parent)
{

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

void Security::setRangePressureValves(const QVariantMap &rangePressureValves){
    QStringList valves = rangePressureValves.keys();
    for(auto valve : valves){
        auto pressureRange = rangePressureValves[valve].toMap();
        double pressureOpen = pressureRange["pressureOpen"].toDouble();
        double pressureClose = pressureRange["pressureClose"].toDouble();
        m_rangePressureValves.insert(valve, ValveToRangePressure(valve, pressureOpen, pressureClose));
    }
}
void Security::setSafePressureValves(const QVariantMap &safePressureValves){
    QStringList valves = safePressureValves.keys();
    for(auto valve : valves){
        auto pressureRange = safePressureValves[valve].toMap();
        double pressureOpen = pressureRange["pressureOpen"].toDouble();
        m_safePressureValves.insert(valve, ValveToSafePressure(valve, pressureOpen));
    }
}

void Security::checkValveAction(const QMap<QString, bool> &valveMap){
    
}