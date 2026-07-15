#include "RegimeWorkers.h"
#include "VacuumRunMonitor.h"
#include <QDebug>
#include <algorithm>

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
    m_repeatsDone   = 0;
    m_repeatsError  = 0;
    m_paused        = false;

    // Open a DB run record
    if (m_cfg.logger)
        m_runId = m_cfg.logger->openRun(m_cfg.regimeId, m_cfg.regimeName, m_cfg.totalRepeats);

    startNextRepeat();
}

// ─── Repeat lifecycle ─────────────────────────────────────────────────────────

void RegimeWorkerBase::startNextRepeat()
{
    if (m_currentRepeat >= m_cfg.totalRepeats) {
        // All repeats done
        bool ok = (m_repeatsError == 0);
        if (m_cfg.logger) {
            m_cfg.logger->logEvent(m_runId,
                                   ok ? RegimeLogger::kRegimeDone : RegimeLogger::kRegimeError,
                                   -1, -1,
                                   QString("done=%1 error=%2").arg(m_repeatsDone).arg(m_repeatsError));
            m_cfg.logger->closeRun(m_runId, ok ? "success" : "error",
                                   m_repeatsDone, m_repeatsError);
        }
        emit done(ok);
        return;
    }

    m_phaseElapsedSec = 0;

    if (m_cfg.conditionType == "none" || m_cfg.conditionTimeSec <= 0) {
        if (m_cfg.manager)
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

    qDebug() << "[Regime" << m_cfg.regimeId << "] Condition phase, repeat" << m_currentRepeat;
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kConditionStart, m_currentRepeat, 0);

    m_timer.start(m_cfg.tickIntervalMs);
}

void RegimeWorkerBase::enterExecutionPhase()
{
    m_phase = Phase::Execution;
    m_phaseElapsedSec = 0;

    qDebug() << "[Regime" << m_cfg.regimeId << "] Execution phase, repeat" << m_currentRepeat;
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kExecutionStart, m_currentRepeat, 0);

    onExecutionPhaseStart();
    m_timer.start(m_cfg.tickIntervalMs);
}

// ─── QTimer tick ─────────────────────────────────────────────────────────────

void RegimeWorkerBase::tick()
{
    if (m_paused) return;  // safety guard — timer should be stopped while paused

    ++m_phaseElapsedSec;

    if (m_phase == Phase::Condition) {

        if (m_cfg.manager)
            m_cfg.manager->updateConditionProgress(
                m_cfg.regimeId, m_phaseElapsedSec, m_currentRepeat);

        bool conditionMet = false;
        if (m_cfg.conditionType == "time") {
            conditionMet = (m_phaseElapsedSec >= m_cfg.conditionTimeSec);
        } else if (m_cfg.conditionType == "temp") {
            if (m_cfg.conditionTempSensor)
                conditionMet = (m_cfg.conditionTempSensor->getCurValue()
                                <= m_cfg.conditionTargetTemp);
            if (m_phaseElapsedSec >= m_cfg.conditionTimeSec)
                conditionMet = true;  // timeout fallback
        }

        if (conditionMet) {
            m_timer.stop();
            if (m_cfg.manager)
                m_cfg.manager->confirmConditionCompletion(m_cfg.regimeId, m_currentRepeat);
            if (m_cfg.logger)
                m_cfg.logger->logEvent(m_runId, RegimeLogger::kConditionDone,
                                       m_currentRepeat, m_phaseElapsedSec);
            enterExecutionPhase();
        }

    } else { // Execution

        // ── Security check ────────────────────────────────────────────────────
        if (m_cfg.security) {
            QMap<QString, bool> pressureState = m_cfg.security->checkValvePressure();
            bool violation = std::any_of(pressureState.cbegin(), pressureState.cend(),
                                         [](bool ok) { return !ok; });
            if (violation) {
                qWarning() << "[Regime" << m_cfg.regimeId
                           << "] Security pressure violation — aborting execution";
                if (m_cfg.logger)
                    m_cfg.logger->logEvent(m_runId, RegimeLogger::kSecurityViolation,
                                           m_currentRepeat, m_phaseElapsedSec);
                m_timer.stop();
                onExecutionPhaseEnd(false);
                finishRepeat(false);
                return;
            }
        }

        if (m_cfg.manager)
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
    if (success) {
        ++m_repeatsDone;
        if (m_cfg.manager)
            m_cfg.manager->completeCurrentRepeat(m_cfg.regimeId, m_currentRepeat);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatDone,
                                   m_currentRepeat, m_phaseElapsedSec);
    } else {
        ++m_repeatsError;
        if (m_cfg.manager)
            m_cfg.manager->markRepeatAsError(m_cfg.regimeId, m_currentRepeat);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatError,
                                   m_currentRepeat, m_phaseElapsedSec);
    }

    ++m_currentRepeat;
    startNextRepeat();
}

// ─── Pause / Resume ───────────────────────────────────────────────────────────

void RegimeWorkerBase::onPauseRequested()
{
    if (m_paused || !m_timer.isActive()) return;
    m_paused = true;
    m_timer.stop();

    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Paused);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kPaused,
                               m_currentRepeat, m_phaseElapsedSec);

    qDebug() << "[Regime" << m_cfg.regimeId << "] Paused";
}

void RegimeWorkerBase::onResumeRequested()
{
    if (!m_paused) return;
    m_paused = false;

    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Running);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kResumed,
                               m_currentRepeat, m_phaseElapsedSec);

    m_timer.start(m_cfg.tickIntervalMs);
    qDebug() << "[Regime" << m_cfg.regimeId << "] Resumed";
}

// ─── Default virtuals ─────────────────────────────────────────────────────────

void RegimeWorkerBase::onExecutionPhaseStart() {}
void RegimeWorkerBase::onExecutionTick(int /*elapsedSec*/) {}
bool RegimeWorkerBase::isExecutionComplete(int elapsedSec) const
{
    return elapsedSec >= m_cfg.maxTimeSec;
}
void RegimeWorkerBase::onExecutionPhaseEnd(bool /*success*/) {}


// ═════════════════════════════════════════════════════════════════════════════
// VacuumRegimeWorker ("Вакуум") — Ф1–Ф3 перенесены из GramQt
// (SequencerThread + testSeq/vacuum_cond.csv) на внутренний QTaskTree.
// Рецепт и вся логика последовательности — в VacuumTaskTree.cpp.
// ═════════════════════════════════════════════════════════════════════════════

VacuumRegimeWorker::VacuumRegimeWorker(QObject* parent)
    : QObject(parent)
{
}

VacuumRegimeWorker::~VacuumRegimeWorker()
{
    // Страховка на случай удаления без внешнего cancelTree(): отмена
    // синхронно прогоняет done-хендлеры рецепта, закрывающие клапаны.
    cancelTree();
}

void VacuumRegimeWorker::setConfig(const RegimeWorkerConfig& cfg)
{
    m_cfg = cfg;
}

void VacuumRegimeWorker::setVacuumOptions(const VacuumOptions& opts)
{
    m_opts = opts;
}

void VacuumRegimeWorker::setMonitor(VacuumRunMonitor* monitor)
{
    m_monitor = monitor;
}

void VacuumRegimeWorker::start()
{
    if (m_cfg.logger)
        m_runId = m_cfg.logger->openRun(m_cfg.regimeId, m_cfg.regimeName,
                                        m_cfg.totalRepeats);
    if (m_cfg.valveControl)
        m_cfg.valveControl->beginAction();

    m_tree = new QtTaskTree::QTaskTree(buildVacuumRecipe(makeContext()), this);

    // Агрегатный прогресс дерева → монитор (как в TaskTree demo).
    if (m_monitor) {
        m_monitor->setProgressMax(m_tree->progressMaximum());
        connect(m_tree, &QtTaskTree::QTaskTree::progressValueChanged, this,
                [this](int v) { if (m_monitor) m_monitor->setProgress(v); });
    }

    connect(m_tree, &QtTaskTree::QTaskTree::done, this,
            [this](QtTaskTree::DoneWith w) {
                if (m_cfg.valveControl)
                    m_cfg.valveControl->endAction();
                if (m_monitor)
                    m_monitor->setRunning(false);
                emit done(w == QtTaskTree::DoneWith::Success);
            });

    m_tree->start();
}

void VacuumRegimeWorker::cancelTree()
{
    if (!m_tree || !m_tree->isRunning())
        return;
    // Рвём связь inner-tree.done → emit done(): при отмене внешний QCustomTask
    // уже сообщил Cancel и завершился. Повторный emit done() из отменяемого
    // внутреннего дерева реэмитил бы в уже завершённый QTaskInterface → краш
    // (в т.ч. сценарий pause→stop из QML). Клапаны закрывают done-хендлеры
    // рецепта во время cancel(); endAction/monitor выполняем здесь напрямую.
    disconnect(m_tree, &QtTaskTree::QTaskTree::done, this, nullptr);
    m_tree->cancel();
    if (m_cfg.valveControl)
        m_cfg.valveControl->endAction();
    if (m_monitor)
        m_monitor->setRunning(false);
}

// ─── Контекст рецепта: швы к железу и RegimeManager ──────────────────────────

VacuumTreeContext VacuumRegimeWorker::makeContext()
{
    VacuumTreeContext ctx;

    ctx.dbSbrLim     = m_opts.dbSbrLim;
    ctx.skipRK10     = m_opts.skipRK10;
    ctx.skipRK50     = m_opts.skipRK50;
    ctx.skipRK300    = m_opts.skipRK300;
    ctx.secondTract  = m_opts.secondTract;
    ctx.totalRepeats = m_cfg.totalRepeats;
    ctx.perActionPauseMs = m_opts.perActionPauseMs;
    ctx.stepPauseMs  = m_opts.stepPauseMs;
    ctx.reliefDwellSec = qMax(1, m_opts.reliefDwellSec);   // К118 держим ≥1 с
    ctx.foreVacuum   = m_opts.foreVacuum;
    ctx.pumpRateCheck = m_opts.pumpRateCheck;
    ctx.operatorBus  = m_opts.operatorBus;                 // стабильная шина из RegimeTaskTree
    ctx.pauseBus     = &m_pauseBus;

    // ДВ301 (Вакууметр) читает в Торр (setAltUnitCoef 0.001333 торр→бар);
    // пороги форвакуума в ТЗ — в Па. 1 Торр = 133.322 Па.
    if (m_opts.vacuumGaugeSensor) {
        DataCollection* gauge = m_opts.vacuumGaugeSensor;
        ctx.pressureVacPa = [gauge] { return gauge->getCurValue() * 133.322; };
    } else {
        qWarning() << "[Вакуум] Датчик ДВ301 (Вакууметр) не задан — форвакуумные "
                      "этапы 11.5–11.7 завершатся по таймауту";
    }

    ctx.conditionType       = m_cfg.conditionType;
    ctx.conditionTimeSec    = m_cfg.conditionTimeSec;
    ctx.conditionTargetTemp = m_cfg.conditionTargetTemp;
    if (m_cfg.conditionTempSensor) {
        DataCollection* sensor = m_cfg.conditionTempSensor;
        ctx.conditionTemp = [sensor] { return sensor->getCurValue(); };
    }

    if (m_opts.pressureSensorB) {
        DataCollection* pB = m_opts.pressureSensorB;
        ctx.pressureB = [pB] { return pB->getCurValue(); };
    } else {
        // Без датчика страж ДВ_СБР видит 0.0 <= dbSbrLim — сбросы через К118
        // не выполняются (клапан не открывается вслепую).
        qWarning() << "[Вакуум] Датчик объёма B (prSB) не задан — "
                      "сбросы через К118 отключены";
    }

    ctx.setValve = [this](bool open, const QString& name) -> bool {
        if (!m_cfg.valveControl) {
            qWarning() << "[Вакуум] ValveControl не задан — операция с" << name
                       << "отклонена";
            return false;
        }
        const bool ok = m_cfg.valveControl->setValveFromAction(open, name);
        if (open && !ok)
            qWarning() << "[Вакуум] Открытие клапана" << name << "заблокировано";
        if (m_monitor)
            m_monitor->onValve(name, open && ok);
        if (m_cfg.logger) {
            const char* type = open ? (ok ? RegimeLogger::kValveOpen
                                          : RegimeLogger::kValveBlocked)
                                    : RegimeLogger::kValveClose;
            m_cfg.logger->logEvent(m_runId, type, -1, -1, name);
        }
        return ok;
    };

    ctx.onConditionProgress = [this](int elapsed, int repeat) {
        if (m_cfg.manager)
            m_cfg.manager->updateConditionProgress(m_cfg.regimeId, elapsed, repeat);
        if (m_monitor)
            m_monitor->onConditionProgress(elapsed, repeat);
    };
    ctx.onConditionDone = [this](int repeat) {
        if (m_cfg.manager)
            m_cfg.manager->confirmConditionCompletion(m_cfg.regimeId, repeat);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kConditionDone, repeat);
    };
    ctx.onProgress = [this](int elapsed, int repeat) {
        if (m_cfg.manager)
            m_cfg.manager->updateRegimeProgress(m_cfg.regimeId, elapsed, repeat);
        if (m_monitor)
            m_monitor->onProgress(elapsed, repeat);
    };
    ctx.onRepeatDone = [this](QtTaskTree::DoneWith w, int repeat) {
        if (w == QtTaskTree::DoneWith::Cancel)
            return;  // Stopped выставляет done-хендлер RegimeTaskTree
        if (w == QtTaskTree::DoneWith::Success) {
            if (m_cfg.manager)
                m_cfg.manager->completeCurrentRepeat(m_cfg.regimeId, repeat);
            if (m_cfg.logger)
                m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatDone, repeat);
            if (m_monitor)
                m_monitor->onRepeatDone(true, repeat);
        } else {
            if (m_cfg.manager)
                m_cfg.manager->markRepeatAsError(m_cfg.regimeId, repeat);
            if (m_cfg.logger)
                m_cfg.logger->logEvent(m_runId, RegimeLogger::kRepeatError, repeat);
            if (m_monitor)
                m_monitor->onRepeatDone(false, repeat);
        }
    };
    ctx.onRunFinished = [this](QtTaskTree::DoneWith w,
                               int repeatsDone, int repeatsError) {
        if (m_monitor)
            m_monitor->onRunFinished(repeatsDone, repeatsError);
        if (!m_cfg.logger)
            return;
        if (w == QtTaskTree::DoneWith::Cancel) {
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kCancelled, -1, -1,
                                   m_cfg.regimeName);
            m_cfg.logger->closeRun(m_runId, "cancelled", repeatsDone, repeatsError);
            return;
        }
        const bool ok = (w == QtTaskTree::DoneWith::Success) && repeatsError == 0;
        m_cfg.logger->logEvent(m_runId,
                               ok ? RegimeLogger::kRegimeDone : RegimeLogger::kRegimeError,
                               -1, -1,
                               QString("done=%1 error=%2")
                                   .arg(repeatsDone).arg(repeatsError));
        m_cfg.logger->closeRun(m_runId, ok ? "success" : "error",
                               repeatsDone, repeatsError);
    };
    ctx.onLabel = [this](const QString& label) {
        qDebug() << "[Вакуум]" << label;
        if (m_monitor)
            m_monitor->onLabel(label);
    };
    ctx.onNode = [this](VacuumNode node, NodeState state) {
        if (m_monitor)
            m_monitor->onNode(node, state);
    };

    return ctx;
}

// ─── Пауза / возобновление (шина PauseBus → PausableTicker в рецепте) ─────────
//
// Пауза замораживает текущую выдержку с открытым клапаном (легаси-подобное
// поведение; альтернатива «закрыть и перезапустить» — см. неоднозначность №4
// в docs/regimes/vacuum.md).

void VacuumRegimeWorker::onPauseRequested()
{
    if (m_pauseBus.isPaused() || !m_tree || !m_tree->isRunning())
        return;
    m_pauseBus.pause();
    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Paused);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kPaused);
    qDebug() << "[Вакуум] Пауза";
}

void VacuumRegimeWorker::onResumeRequested()
{
    if (!m_pauseBus.isPaused())
        return;
    m_pauseBus.resume();
    if (m_cfg.manager)
        m_cfg.manager->setRegimeState(m_cfg.regimeId, RegimeEnums::State::Running);
    if (m_cfg.logger)
        m_cfg.logger->logEvent(m_runId, RegimeLogger::kResumed);
    qDebug() << "[Вакуум] Возобновление";
}


// ═════════════════════════════════════════════════════════════════════════════
// RegimeBWorker ("Режим в")
// ═════════════════════════════════════════════════════════════════════════════

void RegimeBWorker::onExecutionPhaseStart()
{
    qDebug() << "[Режим в] Execution start";
    // TODO: define valve sequence from hardware profile
}

void RegimeBWorker::onExecutionTick(int /*elapsedSec*/)
{
    // TODO: sensor polling, flow / pressure calculations
}

bool RegimeBWorker::isExecutionComplete(int elapsedSec) const
{
    // TODO: add pressure / flow completion condition
    return elapsedSec >= m_cfg.maxTimeSec;
}

void RegimeBWorker::onExecutionPhaseEnd(bool success)
{
    qDebug() << "[Режим в] Execution end, success=" << success;
    // TODO: close valve sequence
}


// ═════════════════════════════════════════════════════════════════════════════
// RegimeGWorker ("Режим г")
// ═════════════════════════════════════════════════════════════════════════════

void RegimeGWorker::onExecutionPhaseStart()
{
    qDebug() << "[Режим г] Execution start";
    // TODO: define valve sequence from hardware profile
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
    qDebug() << "[Режим г] Execution end, success=" << success;
    // TODO: close valve sequence
}
