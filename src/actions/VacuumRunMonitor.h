#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QVariantMap>
#include <QString>

#include "VacuumTaskTree.h"   // VacuumNode, NodeState

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

    // ── Приёмники состояния (из воркера / рецепта) ────────────────────────────
    void beginRun(int totalRepeats);
    void onNode(VacuumNode node, NodeState state);
    void onLabel(const QString& label);
    void onValve(const QString& name, bool open);
    void onProgress(int elapsedSec, int repeat);
    void onConditionProgress(int elapsedSec, int repeat);
    void onRepeatDone(bool success, int repeat);
    void onRunFinished(int repeatsDone, int repeatsError);
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
};
