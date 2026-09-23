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

// Отсчёт датчика давления за такт softEvent: имя, значение (бар) и счётчик
// отсчётов датчика (DataCollection::sampleCount) — по нему видно, что
// показание обновляется.
struct PressureSample {
    QString sensor;
    double bar = 0.0;
    quint64 seq = 0;
};

// Такт softEvent для квартиля: датчики, которые СЕЙЧАС видят его резервуар.
// Широкодиапазонный датчик (DD311, DD331) видит всегда, узкодиапазонный
// (DD312, DD332) — только при открытом клапане диапазона: за закрытым он
// заперт в своём объёме (D1, D2) и показывает не резервуар.
// volumeCm3 — объём, подключённый к резервуару сейчас (основной и открытые
// присоединённые объёмы квартиля), см³; 0 — неизвестен. Нужен прогнозу
// равновесия через перепускные клапаны.
struct QuartileSnapshot {
    QList<PressureSample> sensors;
    double volumeCm3 = 0.0;
};

// Давление квартиля: максимум из достоверных показаний датчиков, видящих
// резервуар. invalid — недостоверные датчики и почему.
struct QuartileReading {
    bool valid = false;
    double bar = 0.0;
    QString sensor;                              // датчик максимума
    QList<QPair<QString, QString>> invalid;      // {датчик, почему}
};

// Замечание проверки давления: какой клапан, почему, по какому квартилю.
// reason — машинный код для журнала режима и лога grams.security:
//   pressure_range           — клапан диапазона открыт (или открывается) при
//                              давлении выше порога
//   pressure_range_autoclose — Security сам закрыл клапан диапазона: давление
//                              резервуара выше порога закрытия
//   transfer_equilibrium     — Security закрыл клапан диапазона: давление
//                              равновесия через перепускной клапан (К151/К153/
//                              К155) выше порога закрытия
//   pressure_release         — давление достигло порога сброса
//   pressure_invalid         — показание датчика недостоверно: нет данных, нет
//                              нового отсчёта дольше N тактов, NaN или вне
//                              диапазона датчика
// origin — где обнаружено: tick (такт softEvent), startup, confirmValve,
// regime_end, command.
struct SecurityIssue {
    QString valve;
    QString reason;
    QString quartile;
    double pressure = 0.0;
    double limit = 0.0;
    QString sensor;
    QString origin;
    QString detail;
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
    // Перепускные клапаны между двумя квартилями (R1–R3 = К153/К155/К151 между
    // накопителем и реакционной областью): по ним Security прогнозирует
    // давление равновесия для клапанов диапазона обеих сторон.
    void setTransferValves(const QStringList &valves, const QString &quartileA, const QString &quartileB);
    bool isTransferValve(const QString &valve) const { return m_transferValves.contains(valve); }
    void setSafeReleaseValves(const QString &valve, const QString &watchQuartile, const double &pressureOpen);
    
    // Диапазон датчика, бар. Вне диапазона показание недостоверно.
    void setSensorRange(const QString &sensor, double minBar, double maxBar);
    // Сколько тактов softEvent подряд показание может не обновляться.
    void setPressureStaleTicks(int ticks);
    // Давления квартилей; один вызов — один такт softEvent. Квартиль, которого
    // нет в вызове, считается не обновлённым на этом такте: его датчики те же,
    // счётчик «нет нового отсчёта» растёт.
    void setQuartilePressures(const QMap<QString, QuartileSnapshot> &quartiles);
    QuartileReading quartilePressure(const QString &quartile) const;

    // Интерлоки и правила давления для команды sender → state. valveStates —
    // фактические состояния клапанов (ValveControl::valveStates); Security их
    // не хранит, поэтому не расходится с платой после старта, отказа записи
    // или readback. Открытие клапана с правилом по давлению (S4/R4 — диапазон,
    // AR4 — сброс) при недостоверном давлении запрещено; S4/R4 открываются
    // только ниже порога открытия (1,6 бар; зона 1,6–1,8 — гистерезис).
    // reason — код причины отказа (пусто при разрешении).
    bool checkValveAction(const QString &sender, const bool &state,
                          const QMap<QString, bool> &valveStates,
                          QString *reason = nullptr) const;
    // Проверка давления для идущего режима. Возвращает только нарушения и
    // предупреждения; закрытый клапан нарушением не является. Недостоверное
    // давление у уже открытого клапана — предупреждение (однократно, до
    // восстановления), клапан не закрывается.
    PressureCheck checkPressure(const QMap<QString, bool> &valveStates) const;

    // Какие клапаны диапазона (S4/R4) Security закрывает сам — на такте
    // softEvent, независимо от режима. Открытый клапан, давление резервуара
    // которого достоверно выше порога закрытия, — pressure_range_autoclose.
    // Недостоверное показание клапан не закрывает (D6): одно предупреждение в
    // warnings. Открывать Security не умеет — только закрывать.
    QList<SecurityIssue> rangeValveClosures(const QMap<QString, bool> &valveStates,
                                            const QString &origin,
                                            QList<SecurityIssue> *warnings = nullptr) const;
    // Прогноз равновесия: открыт клапан диапазона и открыт (в valveStates)
    // перепускной клапан — давление равновесия двух резервуаров по их
    // подключённым объёмам (изотермически: сумма p·V / сумма V) выше порога
    // закрытия → transfer_equilibrium. Для команды открыть перепускной клапан
    // вызывающий передаёт состояния «после команды». Недостоверное давление
    // или неизвестный объём — не закрывать, одно предупреждение (как D6).
    QList<SecurityIssue> transferClosures(const QMap<QString, bool> &valveStates,
                                          const QString &origin,
                                          QList<SecurityIssue> *warnings = nullptr) const;

private:
    struct SensorState {
        PressureSample sample;
        int staleTicks = 0;
    };
    // Достоверно ли показание датчика; why — пояснение для лога.
    bool sensorValid(const SensorState &state, QString *why) const;
    QString watchedQuartile(const QString &valve) const;
    // Предупредить о недостоверных датчиках клапана один раз за эпизод (до
    // восстановления всех датчиков); out — куда сложить замечание.
    void reportInvalid(const QString &valve, const QString &quartile, const QuartileReading &reading,
                       double limit, QList<SecurityIssue> *out) const;
    void forgetInvalid(const QString &valve) const;

    QMap<QString, SensorState> m_sensors;            // по имени датчика
    QMap<QString, QStringList> m_quartileSensors;    // датчики, видящие резервуар
    QMap<QString, double> m_quartileVolume;          // подключённый объём, см³
    QStringList m_transferValves;
    QString m_transferA, m_transferB;
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

