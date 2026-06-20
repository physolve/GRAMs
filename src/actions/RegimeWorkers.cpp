#include "RegimeWorkers.h"
#include <QDebug>

// ═════════════════════════════════════════════════════════════════════════════
// RegimeWorkerBase
// ═════════════════════════════════════════════════════════════════════════════

RegimeWorkerBase::RegimeWorkerBase(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &RegimeWorkerBase::tick);
}

void RegimeWorkerBase::setConfig(const RegimeWorkerConfig& cfg)
{
    m_cfg = cfg;
}

void RegimeWorkerBase::start()
{
    m_currentRepeat = 0;
    startNextRepeat();
}

void RegimeWorkerBase::startNextRepeat()
{
    if (m_currentRepeat >= m_cfg.totalRepeats) {
        emit done(true);
        return;
    }

    m_phaseElapsedSec = 0;

    if (m_cfg.conditionType == "none" || m_cfg.conditionTimeSec <= 0) {
        // No condition — go straight to execution
        m_cfg.manager->confirmConditionCompletion(m_cfg.regimeId, m_currentRepeat);
        enterExecutionPhase();
    } else {
        enterConditionPhase();
    }
}

void RegimeWorkerBase::enterConditionPhase()
{
    m_phase = Phase::Condition;
    m_phaseElapsedSec = 0;
    qDebug() << "[Regime" << m_cfg.regimeId << "] Condition phase started, repeat" << m_currentRepeat;
    m_timer.start(1000);
}

void RegimeWorkerBase::enterExecutionPhase()
{
    m_phase = Phase::Execution;
    m_phaseElapsedSec = 0;
    qDebug() << "[Regime" << m_cfg.regimeId << "] Execution phase started, repeat" << m_currentRepeat;
    onExecutionPhaseStart();
    m_timer.start(1000);
}

void RegimeWorkerBase::tick()
{
    ++m_phaseElapsedSec;

    if (m_phase == Phase::Condition) {
        // Report condition progress
        m_cfg.manager->updateConditionProgress(
            m_cfg.regimeId, m_phaseElapsedSec, m_currentRepeat);

        bool conditionMet = false;

        if (m_cfg.conditionType == "time") {
            conditionMet = (m_phaseElapsedSec >= m_cfg.conditionTimeSec);

        } else if (m_cfg.conditionType == "temp") {
            // Read temperature sensor if connected
            if (m_cfg.conditionTempSensor) {
                double curTemp = m_cfg.conditionTempSensor->getCurValue();
                conditionMet = (curTemp <= m_cfg.conditionTargetTemp);
            }
            // Timeout fallback — always finish when time runs out
            if (m_phaseElapsedSec >= m_cfg.conditionTimeSec)
                conditionMet = true;
        }

        if (conditionMet) {
            m_timer.stop();
            m_cfg.manager->confirmConditionCompletion(m_cfg.regimeId, m_currentRepeat);
            enterExecutionPhase();
        }

    } else { // Phase::Execution
        // Security check — use checkValvePressure() to get failed valves.
        // If any valve is in a forbidden pressure state, abort.
        if (m_cfg.security) {
            QMap<QString, bool> unsafe = m_cfg.security->checkValvePressure();
            bool violation = std::any_of(unsafe.begin(), unsafe.end(),
                                         [](bool ok) { return !ok; });
            if (violation) {
                qWarning() << "[Regime" << m_cfg.regimeId << "] Security pressure violation — aborting";
                m_timer.stop();
                onExecutionPhaseEnd(false);
                finishRepeat(false);
                return;
            }
        }

        // Report execution progress
        m_cfg.manager->updateRegimeProgress(
            m_cfg.regimeId, m_phaseElapsedSec, m_currentRepeat);

        onExecutionTick(m_phaseElapsedSec);

        if (isExecutionComplete(m_phaseElapsedSec)) {
            m_timer.stop();
            onExecutionPhaseEnd(true);
            finishRepeat(true);
        }
    }
}

void RegimeWorkerBase::finishRepeat(bool success)
{
    if (!success) {
        m_cfg.manager->markRepeatAsError(m_cfg.regimeId, m_currentRepeat);
        emit done(false);
        return;
    }

    m_cfg.manager->completeCurrentRepeat(m_cfg.regimeId, m_currentRepeat);
    ++m_currentRepeat;
    startNextRepeat();
}

// Default virtuals — time-based, no hardware interaction

void RegimeWorkerBase::onExecutionPhaseStart() {}

void RegimeWorkerBase::onExecutionTick(int /*elapsedSec*/) {}

bool RegimeWorkerBase::isExecutionComplete(int elapsedSec) const
{
    return elapsedSec >= m_cfg.maxTimeSec;
}

void RegimeWorkerBase::onExecutionPhaseEnd(bool /*success*/) {}


// ═════════════════════════════════════════════════════════════════════════════
// VacuumRegimeWorker  ("Вакуум")
// ═════════════════════════════════════════════════════════════════════════════
//
// Execution phase:
//   1. Open the vacuum pump valve via ValveControl
//   2. Poll vacuum sensor every second
//   3. When pressure < target OR time expired — close valve and finish
//
// TODO: wire vacuumValveName and targetPressure from the hardware profile

static constexpr double kVacuumTargetPressure = 0.01; // bar — replace from profile
static const QString    kVacuumValveName       = "";   // TODO: set from Initialize profile

void VacuumRegimeWorker::onExecutionPhaseStart()
{
    qDebug() << "[Вакуум] Starting vacuum pump";
    if (m_cfg.valveControl && !kVacuumValveName.isEmpty()) {
        m_cfg.valveControl->beginAction();
        m_cfg.valveControl->setValveFromAction(true, kVacuumValveName);
    }
}

void VacuumRegimeWorker::onExecutionTick(int /*elapsedSec*/)
{
    if (!m_cfg.dataAcquisition)
        return;

    // Trigger a fast buffer read so vacuum sensor stays current
    m_cfg.dataAcquisition->fastBufferRead();
}

bool VacuumRegimeWorker::isExecutionComplete(int elapsedSec) const
{
    // Pressure target
    if (m_cfg.dataAcquisition) {
        // TODO: expose vacuum DataCollection via getter in DataAcquisition
        // double pressure = m_cfg.dataAcquisition->vacuumPressure();
        // if (pressure <= kVacuumTargetPressure) return true;
    }
    // Timeout
    return elapsedSec >= m_cfg.maxTimeSec;
}

void VacuumRegimeWorker::onExecutionPhaseEnd(bool success)
{
    qDebug() << "[Вакуум] Stopping vacuum pump," << (success ? "success" : "error");
    if (m_cfg.valveControl && !kVacuumValveName.isEmpty()) {
        m_cfg.valveControl->setValveFromAction(false, kVacuumValveName);
        m_cfg.valveControl->endAction();
    }
}


// ═════════════════════════════════════════════════════════════════════════════
// RegimeBWorker  ("Режим в")
// ═════════════════════════════════════════════════════════════════════════════
//
// TODO: implement when the physics of "Режим в" is defined.
// Placeholder: time-based execution only.

void RegimeBWorker::onExecutionPhaseStart()
{
    qDebug() << "[Режим в] Execution start";
    // TODO: valve sequence, DAQ start, etc.
}

void RegimeBWorker::onExecutionTick(int /*elapsedSec*/)
{
    // TODO: sensor polling, flow calculations
}

bool RegimeBWorker::isExecutionComplete(int elapsedSec) const
{
    // TODO: add pressure / flow condition
    return elapsedSec >= m_cfg.maxTimeSec;
}

void RegimeBWorker::onExecutionPhaseEnd(bool success)
{
    qDebug() << "[Режим в] Execution end," << (success ? "success" : "error");
    // TODO: valve close sequence
}


// ═════════════════════════════════════════════════════════════════════════════
// RegimeGWorker  ("Режим г")
// ═════════════════════════════════════════════════════════════════════════════
//
// TODO: implement when the physics of "Режим г" is defined.

void RegimeGWorker::onExecutionPhaseStart()
{
    qDebug() << "[Режим г] Execution start";
    // TODO: valve sequence
}

void RegimeGWorker::onExecutionTick(int /*elapsedSec*/)
{
    // TODO: sensor polling
}

bool RegimeGWorker::isExecutionComplete(int elapsedSec) const
{
    return elapsedSec >= m_cfg.maxTimeSec;
}

void RegimeGWorker::onExecutionPhaseEnd(bool success)
{
    qDebug() << "[Режим г] Execution end," << (success ? "success" : "error");
    // TODO: valve close sequence
}
