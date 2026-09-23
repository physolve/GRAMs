#pragma once

#include <QVariant>
#include <QLoggingCategory>
#include <QSet>

// grams.security — ответы интерлоков и проверки давления
// (QT_LOGGING_RULES="grams.security.debug=true").
Q_DECLARE_LOGGING_CATEGORY(lcSecurity)

// Кто держит клапан открытым. Ведёт ValveControl рядом с состоянием клапана
// (Security состояний не хранит, решения D3–D5) и отдаёт Security снимком.
// Источник задаёт последняя команда открытия; любое закрытие сбрасывает его.
//   None   — клапан закрыт
//   Manual — открыт оператором (щелчок на мнемосхеме)
//   Regime — открыт режимом (ValveControl::setValveFromAction)
//   Board  — открыт на плате без команды программы: так было при старте
//            или так показал readback (confirmValve)
enum class ValveSource { None, Manual, Regime, Board };
QString toString(ValveSource source);

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

// Отсчёт давления квартиля за такт softEvent: датчик-источник, который квартиль
// выбрал по клапану диапазона, его значение (бар) и счётчик отсчётов датчика
// (DataCollection::sampleCount) — по нему видно, что показание обновляется.
struct PressureSample {
    QString sensor;
    double bar = 0.0;
    quint64 seq = 0;
};

// Замечание проверки давления: какой клапан, почему, по какому квартилю.
// reason — машинный код для журнала режима:
//   pressure_range   — клапан диапазона открыт (или открывается) при давлении
//                      выше порога закрытия
//   pressure_release — давление достигло порога сброса
//   pressure_invalid — давление квартиля недостоверно: нет данных, нет нового
//                      отсчёта дольше N тактов, NaN или вне диапазона датчика
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
    
    // Диапазон датчика, бар. Вне диапазона показание недостоверно.
    void setSensorRange(const QString &sensor, double minBar, double maxBar);
    // Сколько тактов setPressureMap подряд показание может не обновляться.
    void setPressureStaleTicks(int ticks);
    // Давления квартилей; один вызов — один такт softEvent. Квартиль, которого
    // нет в вызове, считается не обновлённым на этом такте.
    void setPressureMap(const QMap<QString, PressureSample> &pressureMap);

    // Интерлоки и правила давления для команды sender → state. valveStates —
    // фактические состояния клапанов (ValveControl::valveStates); Security их
    // не хранит, поэтому не расходится с платой после старта, отказа записи
    // или readback. Открытие клапана с правилом по давлению (S4/R4 — диапазон,
    // AR4 — сброс) при недостоверном давлении запрещено; S4/R4 выше порога
    // закрытия тоже. reason — код причины отказа (пусто при разрешении).
    bool checkValveAction(const QString &sender, const bool &state,
                          const QMap<QString, bool> &valveStates,
                          QString *reason = nullptr) const;
    // Проверка давления для идущего режима. Возвращает только нарушения и
    // предупреждения; закрытый клапан нарушением не является. Недостоверное
    // давление у уже открытого клапана — предупреждение (однократно, до
    // восстановления), клапан не закрывается.
    PressureCheck checkPressure(const QMap<QString, bool> &valveStates) const;

private:
    struct QuartilePressure {
        PressureSample sample;
        int staleTicks = 0;
    };
    // Достоверно ли давление квартиля; why — пояснение для лога.
    bool pressureOf(const QString &quartile, double *bar, QString *why) const;
    QString watchedQuartile(const QString &valve) const;

    QMap<QString, QuartilePressure> m_pressure;
    QMap<QString, QPair<double, double>> m_sensorRange;
    int m_pressureStaleTicks = 4;
    mutable QSet<QString> m_invalidReported;   // клапаны, о недостоверности которых уже сообщено
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

