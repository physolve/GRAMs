#pragma once

// Состояния и переходы QStateMachine демо-данных.
//
// Фаза профиля — SimPhaseState. Виды фаз (static/pumping/gasInlet/h2Inlet)
// отличаются только данными: умолчанием интерполяции и правилами валидации
// (SimProfile), поведение в работе одинаковое, поэтому отдельных подклассов
// по kind нет.

#include "SimEvents.h"
#include "SimProfile.h"

#include <QAbstractTransition>
#include <QSet>
#include <QState>

#include <functional>

namespace sim {

class SimController;

class SimPhaseState final : public QState {
    Q_OBJECT
public:
    SimPhaseState(const PhaseSpec &spec, SimController *controller, QState *parent);

    const PhaseSpec &spec() const { return m_spec; }
    double elapsedSec() const { return m_elapsed; }
    void advance(double dt) { m_elapsed += dt; }

    // Ключевые точки с подставленным "current" — фиксируются при входе.
    const QVector<Keypoint> &resolvedKeypoints(int track) const { return m_resolved[track]; }

    void latchValveEdge(const QString &valveId, bool open);
    bool hasValveEdge(const QString &valveId, bool open) const;
    bool timedOut() const { return m_spec.durationSec && m_elapsed >= *m_spec.durationSec; }

protected:
    void onEntry(QEvent *event) override;

private:
    PhaseSpec m_spec;
    SimController *m_controller;
    double m_elapsed = 0.0;
    QVector<QVector<Keypoint>> m_resolved;
    QSet<QString> m_edges;   // "K176:open"
};

// Переход профиля: timeout / valve / manual. Проверяется на SimTickEvent;
// переходы одной фазы выбираются в порядке добавления — порядок профиля.
class ProfileTransition final : public QAbstractTransition {
    Q_OBJECT
public:
    ProfileTransition(const TransitionSpec &spec, SimPhaseState *source, SimController *controller);

protected:
    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *event) override;

private:
    TransitionSpec m_spec;
    SimPhaseState *m_source;
    SimController *m_controller;
};

// Переход по типу события без условий (pause/resume/stop/fault).
class EventTypeTransition final : public QAbstractTransition {
    Q_OBJECT
public:
    EventTypeTransition(QEvent::Type type, QState *source, std::function<void(QEvent *)> action = {});

protected:
    bool eventTest(QEvent *event) override { return event->type() == m_type; }
    void onTransition(QEvent *event) override { if (m_action) m_action(event); }

private:
    QEvent::Type m_type;
    std::function<void(QEvent *)> m_action;
};

// sim.goto в конкретную фазу.
class GotoTransition final : public QAbstractTransition {
    Q_OBJECT
public:
    GotoTransition(const QString &phaseId, QState *source, SimController *controller);

protected:
    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *event) override;

private:
    QString m_phaseId;
    SimController *m_controller;
};

// Без цели: фиксирует фронт клапана в текущей фазе, состояние не меняет.
class ValveLatchTransition final : public QAbstractTransition {
    Q_OBJECT
public:
    ValveLatchTransition(QState *source, SimController *controller);

protected:
    bool eventTest(QEvent *event) override { return event->type() == SimValveEvent::staticType(); }
    void onTransition(QEvent *event) override;

private:
    SimController *m_controller;
};

} // namespace sim
