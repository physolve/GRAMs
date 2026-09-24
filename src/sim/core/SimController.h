#pragma once

// Контроллер демо-данных: владеет значениями каналов, состояниями клапанов
// и машиной фаз текущего прогона.
//
// Машина QStateMachine строится заново на каждый sim.run: граф фаз задаёт
// профиль, а менять набор состояний у запущенной машины небезопасно.
// «Idle» — машины нет. Внутри прогона:
//
//   Active{ фазы + QHistoryState } ⇄ Paused;  Active|Paused → Fault;
//   Active|Paused|Fault → Stopped (QFinalState) → машина удаляется → Idle.
//
// Команды RPC приходят сюда методами run/goto/pause/resume/stop, проверяются
// (код ошибки контракта) и уходят в машину только через postEvent.

#include "SimClock.h"
#include "SimEvents.h"
#include "SimProfile.h"
#include "SimTypes.h"
#include "TrackEngine.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>

#include <memory>

class QStateMachine;

namespace sim {

class SimPhaseState;

class SimController : public QObject {
    Q_OBJECT
public:
    enum class Machine { Idle, Active, Paused, Fault };
    Q_ENUM(Machine)

    struct Result {
        int code = 0;          // 0 — успех, иначе sim::RpcError
        QString message;
        QString runId;         // для run
        bool ok() const { return code == 0; }
    };

    struct HistoryEntry {
        QString phaseId;
        QDateTime enteredAt;
        QString reason;        // start | timeout | valve | manual
    };

    SimController(Catalog catalog, ISimClock *clock, QObject *parent = nullptr);
    ~SimController() override;

    const Catalog &catalog() const { return m_catalog; }

    // ── значения каналов (Па / °C) и клапаны ────────────────────────────────
    double value(const QString &channelId) const;
    const QHash<QString, double> &values() const { return m_values; }
    void setValue(const QString &channelId, double value);   // с ограничением диапазоном
    bool isValveOpen(const QString &valveId) const { return m_valves.value(valveId); }
    const QHash<QString, bool> &valveStates() const { return m_valves; }

    // ── путь данных ─────────────────────────────────────────────────────────
    // Такт опроса: двигает время фазы, пересчитывает треки, ставит SimTickEvent.
    void tick();
    // Эхо клапана: фронт уходит в машину как SimValveEvent.
    void notifyValve(const QString &valveId, bool open);

    // ── команды ─────────────────────────────────────────────────────────────
    Result run(const Profile &profile, const QString &startPhaseId = {});
    Result gotoPhase(const QString &runId, const QString &phaseId);
    Result pause(const QString &runId);
    Result resume(const QString &runId);
    Result stop(const QString &runId);
    void raiseFault(const QString &reason);

    // ── состояние ───────────────────────────────────────────────────────────
    Machine machine() const { return m_machineState; }
    bool isDemoActive() const { return m_machineState != Machine::Idle; }
    QString runId() const { return m_runId; }
    QString profileName() const { return m_profile.name; }
    QString currentPhaseId() const;
    QString currentPhaseLabel() const;
    double currentPhaseElapsed() const;
    const QList<HistoryEntry> &history() const { return m_history; }
    QString lastError() const { return m_lastError; }

    QJsonObject statusJson(const QJsonValue &regime = QJsonValue::Null) const;
    static QString toWire(Machine m);

    // ── для состояний машины (SimStates.cpp) ───────────────────────────────
    void phaseEntered(SimPhaseState *phase, bool fresh);
    void setNextReason(const QString &reason) { m_nextReason = reason; }
    void latchValveEdge(const QString &valveId, bool open);
    bool consumeResume();

signals:
    void machineChanged();
    void phaseChanged();
    void valuesUpdated();

protected:
    bool event(QEvent *event) override;

private:
    Result checkRun(const QString &runId) const;
    void buildMachine();
    void postToMachine(QEvent *event);
    void setMachineState(Machine m);
    void onMachineFinished();
    void updateTracks();

    Catalog m_catalog;
    ISimClock *m_clock;
    QHash<QString, double> m_values;
    QHash<QString, bool> m_valves;

    Profile m_profile;
    QString m_startPhaseId;
    QString m_runId;
    Machine m_machineState = Machine::Idle;
    QString m_lastError;
    QList<HistoryEntry> m_history;

    QPointer<QStateMachine> m_machine;
    QList<QEvent *> m_pendingEvents;   // до старта машины
    SimPhaseState *m_phase = nullptr;
    QString m_nextReason;
    bool m_resuming = false;
    double m_lastTick = 0.0;
    SimNoise m_noise;
};

} // namespace sim
