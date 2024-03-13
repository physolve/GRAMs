#pragma once

#include <QVariant>

class ValveGraph{
public:
    ValveGraph(const QString &selfName = "unknown");
    void addEachInList(const QStringList &nodeValveList);
    void addRuleOfThree(const QString &threeNodeOne,const QString &threeNodeTwo);
    
    bool applyGraphMask(const QMap<QString, bool> &valveMap);
    
    QString m_selfName;
private:
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

struct ValveToRangePressure{
    QString m_selfName;
    QString m_watchQuartile;
    double m_pressureOpen;
    double m_pressureClose;
    bool applyPressureMask(bool state, double currentPressure, double incomingPressure);
};

struct ValveToSafeRelease{
    QString m_selfName;
    QString m_watchQuartile;
    double m_pressureOpen;
    bool applyPressureMask(bool state, double currentPressure);
};


class Security : public QObject
{
    Q_OBJECT
public:
    Security(QObject *parent = 0);
    Q_INVOKABLE void setContradictionValves(const QVariantMap &contradictionValves, const QVariantList &ruleOfThreeList);
    Q_INVOKABLE void setRangePressureValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen, const double &pressureClose);
    Q_INVOKABLE void setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen);
    bool checkValveAction(const QMap<QString, bool> &valveMap, const QString &sender, const bool &state);
    QMap<QString, bool> checkValvePressure(const QMap<QString, bool> &valveMap, const QMap<QString, double> &pressureMap);

private:
    QMap<QString, bool> m_valveMap;
    // current states (valves)
    // current pressure
    // incoming states (valves)
    // incoming pressure
    // filter incoming states to current states
    // filter incoming states to current/incoming pressure
    QMap<QString, ValveGraph> m_contradictionValves;
    QMap<QString, ValveToRangePressure> m_rangePressureValves;
    QMap<QString, ValveToSafeRelease> m_safeReleaseValves;
};

