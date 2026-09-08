#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QVariantMap>
#include <QString>

#include "../recipes/VacuumTaskTree.h"   // VacuumNode, NodeState

// ─────────────────────────────────────────────────────────────────────────────
// VacuumRunMonitor — наблюдаемый мост состояния рецепта «Вакуум» в QML.
//
// Питается из VacuumRegimeWorker::makeContext() (швы VacuumTreeContext) и из
// прогресса внутреннего QTaskTree. Ничего не решает — только отражает состояние.
// В дальнейшем логика переедет в RegimeManager; пока это standalone-объект,
// доступный из QML как свойство RegimeTaskTree.vacuumMonitor.
//
// Все приёмники вызываются из главного потока (рецепт крутится в главном event
// loop через QTimer), поэтому прямые вызовы безопасны.
// ─────────────────────────────────────────────────────────────────────────────

// Статичная развёртка рецепта: одна строка на VacuumNode (порядок = обход дерева).
class VacuumStepModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        NodeIdRole = Qt::UserRole + 1,
        LabelRole,
        PhaseRole,
        SRangeRole,
        DepthRole,
        StateRole
    };

    explicit VacuumStepModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void resetStates();                                   // все узлы → Initial
    void setNodeState(VacuumNode node, NodeState state);  // обновляет одну строку

private:
    struct Row {
        VacuumNode node;
        QString    label;
        QString    phase;
        QString    sRange;
        int        depth;
        NodeState  state = NodeState::Initial;
    };
    QVector<Row> m_rows;
    int indexOf(VacuumNode node) const;
};

class VacuumRunMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* steps READ steps CONSTANT)
    Q_PROPERTY(bool        running       READ running       NOTIFY runningChanged)
    Q_PROPERTY(bool        paused        READ paused        NOTIFY pausedChanged)
    Q_PROPERTY(int         currentRepeat READ currentRepeat NOTIFY progressChanged)
    Q_PROPERTY(int         totalRepeats  READ totalRepeats  NOTIFY progressChanged)
    Q_PROPERTY(int         elapsedSec    READ elapsedSec    NOTIFY progressChanged)
    Q_PROPERTY(int         progress      READ progress      NOTIFY progressChanged)
    Q_PROPERTY(int         progressMax   READ progressMax   NOTIFY progressChanged)
    Q_PROPERTY(QString     currentLabel  READ currentLabel  NOTIFY currentLabelChanged)
    Q_PROPERTY(int         repeatsDone   READ repeatsDone   NOTIFY progressChanged)
    Q_PROPERTY(int         repeatsError  READ repeatsError  NOTIFY progressChanged)
    Q_PROPERTY(QVariantMap valveStates   READ valveStates   NOTIFY valveStatesChanged)

    // ── Форвакуум 11.5–11.7: цель и live-прогресс её достижения ───────────────
    // forevacNode — int(VacuumNode) качающего сейчас узла, либо -1. QML сопоставляет
    // его с model.nodeId строки развёртки, чтобы нарисовать прогресс ПОД этой строкой.
    Q_PROPERTY(double  forevacTargetPa  READ forevacTargetPa  NOTIFY forevacTargetChanged)
    Q_PROPERTY(int     forevacHoldSec   READ forevacHoldSec   NOTIFY forevacTargetChanged)
    Q_PROPERTY(int     forevacTimeoutSec READ forevacTimeoutSec NOTIFY forevacTargetChanged)
    Q_PROPERTY(int     forevacNode      READ forevacNode      NOTIFY forevacProgressChanged)
    Q_PROPERTY(bool    forevacActive    READ forevacActive    NOTIFY forevacProgressChanged)
    Q_PROPERTY(double  forevacCurrentPa READ forevacCurrentPa NOTIFY forevacProgressChanged)
    Q_PROPERTY(bool    forevacHasReading READ forevacHasReading NOTIFY forevacProgressChanged)
    Q_PROPERTY(int     forevacHeldSec   READ forevacHeldSec   NOTIFY forevacProgressChanged)
    Q_PROPERTY(int     forevacElapsedSec READ forevacElapsedSec NOTIFY forevacProgressChanged)

    // ── Турбо-этап 12.2: активный насос и live-показания обоих датчиков ───────
    // activePump: "" — оба закрыты, "fore" — К176, "turbo" — К179. Это
    // единственный видимый оператору признак того, на каком насосе идёт
    // откачка (REQ-008/082), поэтому свойство обязательное, а не украшение.
    Q_PROPERTY(QString activePump    READ activePump    NOTIFY turboProgressChanged)
    Q_PROPERTY(int     turboNode     READ turboNode     NOTIFY turboProgressChanged)
    Q_PROPERTY(bool    turboActive   READ turboActive   NOTIFY turboProgressChanged)
    Q_PROPERTY(double  dv301Pa       READ dv301Pa       NOTIFY turboProgressChanged)
    Q_PROPERTY(QString dv301Quality  READ dv301Quality  NOTIFY turboProgressChanged)
    Q_PROPERTY(double  dv302Pa       READ dv302Pa       NOTIFY turboProgressChanged)
    Q_PROPERTY(QString dv302Quality  READ dv302Quality  NOTIFY turboProgressChanged)
    Q_PROPERTY(int     turboHeldSec  READ turboHeldSec  NOTIFY turboProgressChanged)
    Q_PROPERTY(int     turboElapsedSec READ turboElapsedSec NOTIFY turboProgressChanged)
    Q_PROPERTY(double  turboGatePa   READ turboGatePa   NOTIFY turboTargetChanged)
    Q_PROPERTY(int     turboGateHoldSec READ turboGateHoldSec NOTIFY turboTargetChanged)
    Q_PROPERTY(int     turboTimeoutSec  READ turboTimeoutSec  NOTIFY turboTargetChanged)

    // ── Прогресс турбо-этапа: одна пара «значение / максимум» на обе стадии ───
    //
    // До переключения (TurboGate) набирается непрерывное удержание ДВ301 ≤ гейта,
    // после переключения (TurboPumping) идёт сама откачка турбонасосом, и её
    // критерий завершения — отведённое время. Это РАЗНЫЕ величины, но оператору
    // нужна одна полоса, которая не замирает при переходе, поэтому монитор
    // отдаёт готовые value/max и имя стадии, а QML не разбирает VacuumNode.
    //   turboStage: "" — этапа нет, "gate" — набор гейта, "pumping" — откачка.
    Q_PROPERTY(QString turboStage        READ turboStage        NOTIFY turboProgressChanged)
    Q_PROPERTY(int     turboProgressSec  READ turboProgressSec  NOTIFY turboProgressChanged)
    Q_PROPERTY(int     turboProgressMaxSec READ turboProgressMaxSec NOTIFY turboProgressChanged)

    // ── Бюджет времени прогона (T_total из строки RunTable) ───────────────────
    //
    // Оператору нужно видеть суммарное время прогона и остаток, а не только
    // прогресс текущего этапа: на финальной откачке важно именно «сколько
    // осталось до конца строки», и это же число показывает RunTable.
    Q_PROPERTY(int budgetTotalSec     READ budgetTotalSec     NOTIFY budgetChanged)
    Q_PROPERTY(int budgetElapsedSec   READ budgetElapsedSec   NOTIFY budgetChanged)
    Q_PROPERTY(int budgetRemainingSec READ budgetRemainingSec NOTIFY budgetChanged)
    Q_PROPERTY(bool budgetLimited     READ budgetLimited      NOTIFY budgetChanged)

    // ── Причина завершения прогона (пусто, пока прогон не закончился) ─────────
    Q_PROPERTY(QString finishReason READ finishReason NOTIFY finishReasonChanged)
    Q_PROPERTY(int     finishState  READ finishState  NOTIFY finishReasonChanged)

    // ── Пропущенные этапы и предупреждения прогона ────────────────────────────
    // «Этап не выполнялся» и «этап пройден» обязаны выглядеть в интерфейсе
    // по-разному, поэтому причины пропуска живут отдельным списком, а не в
    // общей строке статуса.
    Q_PROPERTY(QStringList skippedStages READ skippedStages NOTIFY noticesChanged)
    Q_PROPERTY(QStringList warnings      READ warnings      NOTIFY noticesChanged)

public:
    explicit VacuumRunMonitor(QObject* parent = nullptr);

    QAbstractItemModel* steps() const;
    bool        running()       const { return m_running; }
    bool        paused()        const { return m_paused; }
    int         currentRepeat() const { return m_currentRepeat; }
    int         totalRepeats()  const { return m_totalRepeats; }
    int         elapsedSec()    const { return m_elapsedSec; }
    int         progress()      const { return m_progress; }
    int         progressMax()   const { return m_progressMax; }
    QString     currentLabel()  const { return m_currentLabel; }
    int         repeatsDone()   const { return m_repeatsDone; }
    int         repeatsError()  const { return m_repeatsError; }
    QVariantMap valveStates()   const { return m_valveStates; }
    int  budgetTotalSec()     const { return m_budgetTotalSec; }
    int  budgetElapsedSec()   const { return m_budgetElapsedSec; }
    int  budgetRemainingSec() const { return m_budgetRemainingSec; }
    bool budgetLimited()      const { return m_budgetTotalSec > 0; }
    QStringList skippedStages() const { return m_skippedStages; }
    QStringList warnings()      const { return m_warnings; }

    double  forevacTargetPa()   const { return m_forevacTargetPa; }
    int     forevacHoldSec()    const { return m_forevacHoldSec; }
    int     forevacTimeoutSec() const { return m_forevacTimeoutSec; }
    int     forevacNode()       const { return m_forevacNode; }
    bool    forevacActive()     const { return m_forevacNode >= 0; }
    double  forevacCurrentPa()  const { return m_forevacCurrentPa; }
    bool    forevacHasReading() const { return m_forevacHasReading; }
    int     forevacHeldSec()    const { return m_forevacHeldSec; }
    int     forevacElapsedSec() const { return m_forevacElapsedSec; }

    QString activePump()      const { return m_activePump; }
    int     turboNode()       const { return m_turboNode; }
    bool    turboActive()     const { return m_turboNode >= 0; }
    double  dv301Pa()         const { return m_dv301Pa; }
    QString dv301Quality()    const { return m_dv301Quality; }
    double  dv302Pa()         const { return m_dv302Pa; }
    QString dv302Quality()    const { return m_dv302Quality; }
    int     turboHeldSec()    const { return m_turboHeldSec; }
    int     turboElapsedSec() const { return m_turboElapsedSec; }
    double  turboGatePa()     const { return m_turboGatePa; }
    int     turboGateHoldSec() const { return m_turboGateHoldSec; }
    int     turboTimeoutSec() const { return m_turboTimeoutSec; }
    QString turboStage()       const;
    int     turboProgressSec() const;
    int     turboProgressMaxSec() const;

    void setTurboGate(double gatePa, int holdSec, int timeoutSec);
    void onTurboProgress(VacuumNode node, Reading p301, Reading p302,
                         int heldSec, int elapsedSec);
    void onTurboSwitched(bool toTurbo);

    QString finishReason() const { return m_finishReason; }
    int     finishState()  const { return m_finishState; }

    // ── Приёмники состояния (из воркера / рецепта) ────────────────────────────
    void beginRun(int totalRepeats);
    void onNode(VacuumNode node, NodeState state);
    void onLabel(const QString& label);
    void onValve(const QString& name, bool open);
    void onProgress(int elapsedSec, int repeat);
    void onConditionProgress(int elapsedSec, int repeat);
    void onRepeatDone(bool success, int repeat);
    void onRunFinished(int repeatsDone, int repeatsError);
    // Цель форвакуума — выставляется до старта, чтобы UI показывал «должно быть»
    // ещё до входа в 11.5.
    void setForevacTarget(double targetPa, int holdSec, int timeoutSec);
    void onForevacProgress(VacuumNode node, double currentPa, int heldSec, int elapsedSec);
    void onForevacDone(VacuumNode node, bool success);
    // Причина отказа из рецепта (детальная). Первая за прогон побеждает —
    // последующие каскадные отказы её не затирают.
    void onFailure(const QString& reason);
    // Этап не выполнялся и почему (node = int(VacuumNode)). Копится списком:
    // за прогон может быть пропущено несколько этапов, и оператор обязан
    // увидеть их все, а не только первый.
    void onStageSkipped(int node, const QString& reason);
    // Расход бюджета времени прогона: сколько прошло и сколько осталось.
    void onBudget(int elapsedSec, int remainingSec);
    // T_total прогона; вызывается до старта, из RegimeTaskTree.
    void setBudgetTotal(int totalSec);
    // Предупреждение без остановки режима (REQ-062). Тоже списком.
    void onWarning(const QString& message);
    // Итог прогона: state — RegimeEnums::State (Done/Error/Stopped), reason —
    // готовый текст либо пусто (тогда берётся накопленная причина отказа).
    void setFinish(int state, const QString& reason);
    void setProgress(int value);
    void setProgressMax(int max);
    void setRunning(bool running);
    void setPaused(bool paused);

    // ── Управление из QML ─────────────────────────────────────────────────────
    Q_INVOKABLE void reset();

signals:
    void runningChanged();
    void pausedChanged();
    void progressChanged();
    void currentLabelChanged();
    void valveStatesChanged();
    void forevacTargetChanged();
    void forevacProgressChanged();
    void finishReasonChanged();
    void turboProgressChanged();
    void turboTargetChanged();
    void noticesChanged();
    void budgetChanged();

private:
    void initValves();

    VacuumStepModel* m_steps;
    bool        m_running       = false;
    bool        m_paused        = false;
    int         m_currentRepeat = 0;
    int         m_totalRepeats  = 1;
    int         m_elapsedSec    = 0;
    int         m_progress      = 0;
    int         m_progressMax   = 0;
    QString     m_currentLabel;
    int         m_repeatsDone    = 0;
    int         m_repeatsError   = 0;
    QVariantMap m_valveStates;

    double m_forevacTargetPa   = 40.0;
    int    m_forevacHoldSec    = 60;
    int    m_forevacTimeoutSec = 300;
    int    m_forevacNode       = -1;      // int(VacuumNode) активного этапа, -1 = нет
    double m_forevacCurrentPa  = 0.0;
    bool   m_forevacHasReading = false;   // false, когда датчик ДВ301 не задан
    int    m_forevacHeldSec    = 0;
    int    m_forevacElapsedSec = 0;

    int         m_budgetTotalSec     = 0;
    int         m_budgetElapsedSec   = 0;
    int         m_budgetRemainingSec = 0;
    QString     m_finishReason;
    QStringList m_skippedStages;          // причины пропуска этапов за прогон
    QStringList m_warnings;               // предупреждения без остановки режима
    QString m_failureReason;              // первая причина отказа из рецепта
    int     m_finishState = -1;           // RegimeEnums::State, -1 = не завершён

    // ── Турбо-этап 12.2 ──────────────────────────────────────────────────────
    QString m_activePump;                 // "", "fore" (К176), "turbo" (К179)
    int     m_turboNode        = -1;      // int(VacuumNode) активного узла либо -1
    double  m_dv301Pa          = 0.0;
    QString m_dv301Quality;
    double  m_dv302Pa          = 0.0;
    QString m_dv302Quality;
    int     m_turboHeldSec     = 0;
    int     m_turboElapsedSec  = 0;
    double  m_turboGatePa      = 10.0;    // порог перехода (REQ-078/080)
    int     m_turboGateHoldSec = 60;      // удержание порога (REQ-080)
    int     m_turboTimeoutSec  = 600;     // отведённое время турбо-откачки
};
