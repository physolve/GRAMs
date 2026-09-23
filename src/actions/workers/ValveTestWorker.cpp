#include "ValveTestWorker.h"
#include <QDebug>
#include <algorithm>

ValveTestWorker::ValveTestWorker(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &ValveTestWorker::tick);
}

void ValveTestWorker::setConfig(const ValveTestConfig& cfg)
{
    m_cfg = cfg;
}

// ─── start ────────────────────────────────────────────────────────────────────

void ValveTestWorker::start()
{
    if (m_cfg.steps.isEmpty()) {
        qWarning() << "ValveTestWorker: no steps configured";
        emit done(false);
        return;
    }

    if (m_cfg.logger)
        m_runId = m_cfg.logger->openRun(m_cfg.regimeId, m_cfg.regimeName, m_cfg.totalRepeats);

    if (m_cfg.valveControl) {
        m_cfg.valveControl->beginAction();
        connect(m_cfg.valveControl, &ValveControl::securityClosed,
                this, &ValveTestWorker::onSecurityClosed, Qt::UniqueConnection);
    }

    m_currentRepeat = 0;
    m_repeatsDone   = 0;
    m_repeatsError  = 0;
    startRepeat();
}

// ─── repeat lifecycle ─────────────────────────────────────────────────────────

void ValveTestWorker::startRepeat()
{
    m_currentStep  = 0;
    m_dwellElapsed = 0;
    qDebug() << "ValveTestWorker: repeat" << m_currentRepeat + 1 << "of" << m_cfg.totalRepeats;

    if (m_cfg.manager)
        m_cfg.manager->confirmConditionCompletion(m_cfg.regimeId, m_currentRepeat);

    enterGlobalPauseBefore();
}

void ValveTestWorker::enterGlobalPauseBefore()
{
    m_state     = State::GlobalPauseBefore;
    m_countdown = m_cfg.globalPauseBeforeSec;

    if (m_countdown <= 0) {
        enterStepPauseBefore();
    } else {
        m_timer.start(1000);
    }
}

void ValveTestWorker::enterStepPauseBefore()
{
    if (m_currentStep >= m_cfg.steps.size()) {
        // All steps done — enter global pause after
        enterGlobalPauseAfter();
        return;
    }

    m_state     = State::StepPauseBefore;
    m_countdown = m_cfg.steps.at(m_currentStep).pauseBeforeSec;

    if (m_countdown <= 0) {
        openCurrentStepValves();
    } else {
        m_timer.start(1000);
    }
}

void ValveTestWorker::openCurrentStepValves()
{
    const ValveStepConfig& step = m_cfg.steps.at(m_currentStep);
    m_commanding = true;
    openValves(step.valveNames);
    m_commanding = false;
    m_dwellElapsed = 0;
    if (m_securityAbortPending) {
        m_securityAbortPending = false;
        abortCurrentStep();
        return;
    }
    enterStepDwelling();
}

void ValveTestWorker::onSecurityClosed(const SecurityIssue& issue, ValveSource previous)
{
    if (previous != ValveSource::Regime)
        return;
    if (m_state != State::StepDwelling && !m_commanding)
        return;
    if (!m_cfg.steps.at(m_currentStep).valveNames.contains(issue.valve))
        return;
    qWarning() << "ValveTestWorker: Security закрыл клапан шага" << m_currentStep << issue.toString();
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kSecurityViolation,
                               m_currentRepeat, m_dwellElapsed,
                               QString("step %1: %2").arg(m_currentStep).arg(issue.toString()));
    if (m_commanding) {
        m_securityAbortPending = true;
        return;
    }
    abortCurrentStep();
}

void ValveTestWorker::enterStepDwelling()
{
    m_state     = State::StepDwelling;
    m_countdown = m_cfg.steps.at(m_currentStep).dwellSec;
    m_timer.start(1000);
}

void ValveTestWorker::closeCurrentStepValves()
{
    const ValveStepConfig& step = m_cfg.steps.at(m_currentStep);
    closeValves(step.valveNames);

    m_state     = State::StepPauseAfter;
    m_countdown = step.pauseAfterSec;

    if (m_countdown <= 0) {
        advanceToNextStep();
    } else {
        m_timer.start(1000);
    }
}

// Авария на выдержке: закрыть клапаны шага и завершить повтор с ошибкой.
// closeCurrentStepValves здесь нельзя: при pauseAfterSec == 0 он сразу ведёт
// автомат к следующему шагу и открывает его клапаны, а finishRepeat поверх
// этого запускает второй поток того же автомата.
void ValveTestWorker::abortCurrentStep()
{
    m_timer.stop();
    closeValves(m_cfg.steps.at(m_currentStep).valveNames);
    m_state = State::Idle;
    finishRepeat(false);
}

void ValveTestWorker::enterStepPauseAfter()
{
    // Called only if countdown > 0 (entered from tick)
    advanceToNextStep();
}

void ValveTestWorker::advanceToNextStep()
{
    ++m_currentStep;
    enterStepPauseBefore();
}

void ValveTestWorker::enterGlobalPauseAfter()
{
    m_state     = State::GlobalPauseAfter;
    m_countdown = m_cfg.globalPauseAfterSec;

    if (m_countdown <= 0) {
        finishRepeat(true);
    } else {
        m_timer.start(1000);
    }
}

// ─── QTimer tick ─────────────────────────────────────────────────────────────

void ValveTestWorker::tick()
{
    if (m_paused) return;

    // Security check while valves are open (dwelling)
    if (m_state == State::StepDwelling) {
        if (!checkSecurity()) {
            abortCurrentStep();
            return;
        }
        ++m_dwellElapsed;
        reportProgress();
    }

    --m_countdown;

    if (m_countdown > 0) return;

    // Countdown reached zero — transition
    m_timer.stop();

    switch (m_state) {
    case State::GlobalPauseBefore:
        enterStepPauseBefore();
        break;
    case State::StepPauseBefore:
        openCurrentStepValves();
        break;
    case State::StepDwelling:
        closeCurrentStepValves();
        break;
    case State::StepPauseAfter:
        advanceToNextStep();
        break;
    case State::GlobalPauseAfter:
        finishRepeat(true);
        break;
    default:
        break;
    }
}

// ─── repeat / all-repeats completion ─────────────────────────────────────────

void ValveTestWorker::finishRepeat(bool success)
{
    if (success) {
        ++m_repeatsDone;
        if (m_cfg.manager)
            m_cfg.manager->completeCurrentRepeat(m_cfg.regimeId, m_currentRepeat);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatDone, m_currentRepeat);
    } else {
        ++m_repeatsError;
        if (m_cfg.manager)
            m_cfg.manager->markRepeatAsError(m_cfg.regimeId, m_currentRepeat);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatError, m_currentRepeat);
    }

    ++m_currentRepeat;

    if (m_currentRepeat >= m_cfg.totalRepeats) {
        finishAllRepeats();
    } else {
        startRepeat();
    }
}

void ValveTestWorker::finishAllRepeats()
{
    m_timer.stop();
    m_state = State::Idle;
    if (m_cfg.valveControl) {
        m_cfg.valveControl->endAction();
        disconnect(m_cfg.valveControl, &ValveControl::securityClosed,
                   this, &ValveTestWorker::onSecurityClosed);
    }

    bool ok = (m_repeatsError == 0);

    if (m_cfg.logger) {
        m_cfg.logger->logEvent(m_runId,
                               ok ? RegimeLogger::kRegimeDone : RegimeLogger::kRegimeError,
                               -1, -1,
                               QString("done=%1 error=%2").arg(m_repeatsDone).arg(m_repeatsError));
        m_cfg.logger->closeRun(m_runId, ok ? "success" : "error", m_repeatsDone, m_repeatsError);
    }

    qDebug() << "ValveTestWorker: finished, success=" << ok;
    emit done(ok);
}

// ─── hardware helpers ─────────────────────────────────────────────────────────

void ValveTestWorker::openValves(const QStringList& names)
{
    if (!m_cfg.valveControl) return;

    for (const QString& name : names) {
        bool ok = m_cfg.valveControl->setValveFromAction(true, name);
        const QString refusal = ok ? QString() : m_cfg.valveControl->lastRefusal();
        qDebug() << "ValveTestWorker: open" << name << (ok ? "OK" : "BLOCKED") << refusal;
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId,
                                   ok ? RegimeLogger::kValveOpen : RegimeLogger::kValveBlocked,
                                   m_currentRepeat, 0,
                                   refusal.isEmpty() ? name : name + u' ' + refusal);
    }
}

void ValveTestWorker::closeValves(const QStringList& names)
{
    if (!m_cfg.valveControl) return;

    for (const QString& name : names) {
        m_cfg.valveControl->setValveFromAction(false, name);
        qDebug() << "ValveTestWorker: close" << name;
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kValveClose,
                                   m_currentRepeat, m_dwellElapsed, name);
    }
}

bool ValveTestWorker::checkSecurity()
{
    if (!m_cfg.security) return true;

    const PressureCheck check = m_cfg.security->checkPressure(
        m_cfg.valveControl ? m_cfg.valveControl->valveStates() : QMap<QString, bool>{});
    for (const SecurityIssue& issue : check.warnings) {
        qWarning() << "ValveTestWorker: предупреждение Security на шаге" << m_currentStep
                   << issue.toString();
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kWarning,
                                   m_currentRepeat, m_dwellElapsed, issue.toString());
    }
    for (const SecurityIssue& issue : check.violations) {
        qWarning() << "ValveTestWorker: security violation at step" << m_currentStep
                   << issue.toString();
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kSecurityViolation,
                                   m_currentRepeat, m_dwellElapsed,
                                   QString("step %1: %2").arg(m_currentStep).arg(issue.toString()));
    }
    return check.ok();
}

void ValveTestWorker::reportProgress()
{
    if (!m_cfg.manager) return;

    // Overall elapsed = completed steps * avg dwell + current dwell
    int totalElapsed = m_currentStep * 2 + m_dwellElapsed;  // rough estimate
    m_cfg.manager->updateRegimeProgress(m_cfg.regimeId, totalElapsed, m_currentRepeat);
}

// ─── pause / resume ───────────────────────────────────────────────────────────

void ValveTestWorker::onPauseRequested()
{
    if (m_paused) return;
    m_paused = true;
    m_timer.stop();

    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Paused);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kPaused,
                               m_currentRepeat, m_dwellElapsed,
                               QString("step %1").arg(m_currentStep));

    qDebug() << "ValveTestWorker: paused at step" << m_currentStep;
}

void ValveTestWorker::onResumeRequested()
{
    if (!m_paused) return;
    m_paused = false;

    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Running);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kResumed,
                               m_currentRepeat, m_dwellElapsed);

    // Restart timer only if there's remaining time in current state
    if (m_state != State::Idle)
        m_timer.start(1000);

    qDebug() << "ValveTestWorker: resumed at step" << m_currentStep;
}
