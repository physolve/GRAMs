#pragma once

#include <QVariant>
#include <QLoggingCategory>

// grams.security — ответы интерлоков и проверки давления
// (QT_LOGGING_RULES="grams.security.debug=true").
Q_DECLARE_LOGGING_CATEGORY(lcSecurity)

class ValveGraph{
public:
    ValveGraph(const QString &selfName = "unknown");

    bool isExclusion() const;
    void addEachInList(const QStringList &exclusionValveList);
    void addRuleOfThree(const QString &threeNodeOne,const QString &threeNodeTwo);
    bool applyGraphMask(const QMap<QString, bool> &valveMap) const;
    QString m_selfName;

private:
    bool maskEachInList(const QMap<QString, bool> &valveMap) const;
    bool maskRuleOfThree(const QMap<QString, bool> &valveMap) const;

    bool checkEachInList;
    QStringList exclusionValves;

    bool checkRuleOfThree;
    QString m_threeNodeOne;
    QString m_threeNodeTwo;
};

struct ValveToRangePressure{
    QString m_selfName;
    QString m_watchQuartile;
    double m_pressureOpen;
    double m_pressureClose;
    // currentPressure - step, incoming - two step
    bool applyPressureMask(bool currentState, double incomingPressure) const;
};

struct ValveToSafeRelease{
    QString m_selfName;
    QString m_watchQuartile;
    double m_gasMax;
    bool applyPressureMask(double currentPressure) const;
};

struct ReactionToSupply{
    // this could be Supply or Leakage valves
    // start with Leakage
    QString m_selfName;
    ValveToRangePressure m_rangePressure;
    ValveToSafeRelease m_safeRelease;
    bool applyPressureMask(bool &rangePressureState, double &safeReleaseState, double incomingPressure) const;
};
struct ReactionToLeakage{
    // kind of three step check
    QString m_selfName;
    ValveToRangePressure m_rangePressure;
    double gasMax; // from profile
    double chamberMax; // chamber object
    bool applyPressureMask(bool &rangePressureState, double incomingPressure) const;
};

// Замечание проверки давления: какой клапан, почему, по какому квартилю.
// reason — машинный код для журнала режима:
//   pressure_range   — клапан диапазона открыт при давлении выше порога закрытия
//   pressure_release — давление достигло порога сброса
struct SecurityIssue {
    QString valve;
    QString reason;
    QString quartile;
    double pressure = 0.0;
    double limit = 0.0;
    QString toString() const;
};

// Результат проверки давления. violations — режим обязан прервать шаг;
// warnings — сообщить и продолжать.
struct PressureCheck {
    QList<SecurityIssue> violations;
    QList<SecurityIssue> warnings;
    bool ok() const { return violations.isEmpty(); }
};

class Security : public QObject
{
    Q_OBJECT
public:
    explicit Security(QObject *parent = 0); // ?
    void setContradictionValves(const QMap<QString, QStringList> &contradictionValves);
    void setRuleOfThreeValves(const QStringList &ruleOfThreeList);

    void setGasSupplyValves(const QStringList &gasSupplyList);
    void setGasLeakageValves(const QStringList &gasLeakageList);
    
    void setRangePressureValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen, const double &pressureClose);
    void setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen);
    
    // void setValveMap(const QMap<QString, bool> &valveMap);
    void setPressureMap(const QMap<QString, double> &pressureMap);
    // Интерлоки для команды sender → state. valveStates — фактические
    // состояния клапанов (ValveControl::valveStates); Security их не хранит,
    // поэтому не расходится с платой после старта, отказа записи или readback.
    bool checkValveAction(const QString &sender, const bool &state,
                          const QMap<QString, bool> &valveStates) const;
    // Проверка давления для идущего режима. valveStates — фактические
    // состояния клапанов (ValveControl::valveStates). Возвращает только
    // нарушения и предупреждения; закрытый клапан нарушением не является.
    PressureCheck checkPressure(const QMap<QString, bool> &valveStates) const;

private:
    // pointers 
    
    // current pressure
    QMap<QString, double> m_pressureQuarMap;
    mutable bool m_noPressureReported = false;
    // pointers
    
    // incoming states (valves)
    // incoming pressure
    // pressureNodes
    // filter incoming states to current states
    // filter incoming states to current/incoming pressure
    QMap<QString, ValveGraph> m_contradictionValves;
    QMap<QString, ValveToRangePressure> m_rangePressureValves;
    QMap<QString, ValveToSafeRelease> m_safeReleaseValves;

    QMap<QString, ReactionToSupply> m_supplyValves;
    QMap<QString, ReactionToLeakage> m_leakageValves;

    // QMap<QString, ControllerConnection> GRAMsIntegrity;
    
};

