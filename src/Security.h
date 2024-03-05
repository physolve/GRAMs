#pragma once

#include <QVariant>

class ValveGraph{
public:
    ValveGraph(const QString & selfName);
    void addEachInList(const QStringList &nodeValveList);
    void addRuleOfThree(const QString &threeNodeOne,const QString &threeNodeTwo);
    
    bool applyGraphMask(const QMap<QString, bool> &valveMap);
    
private:
    const QString m_selfName;
    bool checkEachInList = false;
    QStringList nodeValves;
    bool maskEachInList(const QMap<QString, bool> &valveMap){
        for(auto nodeValve : nodeValves){
            if(valveMap[nodeValve]){
                return false;
            }
        }
        return true;
    }
    bool checkRuleOfThree = false;
    QString m_threeNodeOne;
    QString m_threeNodeTwo;
    bool maskRuleOfThree(const QMap<QString, bool> &valveMap){
        if(valveMap[m_threeNodeOne]&&valveMap[m_threeNodeTwo])
            return false;
        return true;
    }
};

class ValveToRangePressure{
public:
    ValveToRangePressure(const QString &selfName, double pressureOpen, double pressureClose);
    bool applyPressureMask(bool state, double currentPressure, double incomingPressure);
private:
    const QString m_selfName;
    //const QString m_quartile;
    const double m_pressureOpen;
    const double m_pressureClose;
};

class ValveToSafePressure{
public:
    ValveToSafePressure(const QString &selfName, double pressureOpen);
    bool applyPressureMask(bool state, double currentPressure);
private:
    const QString m_selfName;
    //const QString m_quartile;
    const double m_pressureOpen;
};

class Security : public QObject
{
    Q_OBJECT
    Q_INVOKABLE void setContradictionValves(const QVariantMap &contradictionValves, const QVariantList &ruleOfThreeList);
    Q_INVOKABLE void setRangePressureValves(const QVariantMap &rangePressureValves);
    Q_INVOKABLE void setSafePressureValves(const QVariantMap &safePressureValves);
public:
    Security(QObject *parent = 0);
    void checkValveAction(const QMap<QString, bool> &valveMap);
    void checkValvePressure(const QMap<QString, bool> &valveMap, const QMap<QString, double> &pressureMap);

private:
    // current states (valves)
    // current pressure
    // incoming states (valves)
    // incoming pressure
    // filter incoming states to current states
    // filter incoming states to current/incoming pressure
    QMap<QString, ValveGraph> m_contradictionValves;
    QMap<QString, ValveToRangePressure> m_rangePressureValves;
    QMap<QString, ValveToSafePressure> m_safePressureValves;
};