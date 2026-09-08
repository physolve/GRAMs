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
    ctx.reliefDwellSec = qMax(1, m_opts.reliefDwellSec);   // К118 держим ≥1 с
    ctx.targetVacPa       = m_opts.targetVacPa;
    ctx.foreVacHoldSec    = m_opts.foreVacHoldSec;
    ctx.foreVacTimeoutSec = m_opts.foreVacTimeoutSec;
    ctx.pumpRateCheck = m_opts.pumpRateCheck;
    // Турбо 12.2 — пороги безопасности из JSON, не из UI.
    ctx.turboTract            = m_opts.turboTract;
    ctx.turboSwitchPressurePa = m_opts.turboSwitchPressurePa;
    ctx.turboSwitchHoldSec    = m_opts.turboSwitchHoldSec;
    ctx.turboReturnPressurePa = m_opts.turboReturnPressurePa;
    ctx.turboTimeoutSec       = m_opts.turboTimeoutSec;
    ctx.overrangeWaitSec      = m_opts.overrangeWaitSec;
    ctx.overrangeWaitSec2     = m_opts.overrangeWaitSec2;
    // Этапы 11.7б/11.8/11.9-11.11: наборы клапанов и пороги приходят из профиля
    // (REQ-055/059/064/066), в коде их нет намеренно.
    ctx.generalPumpingValves = m_opts.generalPumpingValves;
    ctx.leakTestValves       = m_opts.leakTestValves;
    ctx.finalPumpingValves   = m_opts.finalPumpingValves;
    ctx.testEvacTimeSec      = m_opts.testEvacTimeSec;
    ctx.leakTestDurationSec  = m_opts.leakTestDurationSec;
    ctx.targetVacuumPa       = m_opts.targetVacuumPa;
    ctx.evacTimeSec          = m_opts.evacTimeSec;

    // Каналы герметичности собираются только из тех имён, у которых есть И
    // порог в конфигурации, И живой датчик. Канал без одного из двух в список
    // не попадает: проверить его нечем, а «проверен и герметичен» — ложь.
    for (auto it = m_opts.dPLeakMax.cbegin(); it != m_opts.dPLeakMax.cend(); ++it) {
        DataCollection* sensor = m_opts.leakSensors.value(it.key(), nullptr);
        if (!sensor) {
            qWarning() << "[Вакуум] 11.8: порог герметичности задан для" << it.key()
                       << "но датчик не подключён — канал не проверяется";
            continue;
        }
        LeakChannel ch;
        ch.name    = it.key();
        ch.maxRise = it.value();
        ch.read    = [sensor] { return Reading(sensor->getCurValue(), sensor->quality()); };
        ctx.leakChannels.append(ch);
    }
    if (ctx.leakChannels.isEmpty())
        qWarning() << "[Вакуум] 11.8: ни одного канала герметичности — этап будет "
                      "пропущен с указанием причины, а не пройден";

    // T_total прогона — время строки RunTable (max_time, секунды). Раньше оно
    // доезжало до RegimeWorkerConfig и там умирало: вакуумный воркер его не
    // читал, и суммарное время режима ничем не ограничивалось.
    ctx.totalBudgetSec = qMax(0, m_cfg.maxTimeSec);

    ctx.continuousPumping = m_opts.continuousPumping;
    // Живое значение тумблера: оператор вправе передумать, пока режим идёт.
    ctx.continuousPumpingLive = [this] { return m_opts.continuousPumping; };
    ctx.operatorBus  = m_opts.operatorBus;                 // стабильная шина из RegimeTaskTree
    ctx.pauseBus     = &m_pauseBus;

    // ДВ301 (Вакууметр) читает в Торр (setAltUnitCoef 0.001333 торр→бар);
    // пороги форвакуума в ТЗ — в Па. 1 Торр = 133.322 Па.
    if (m_opts.vacuumGaugeSensor) {
        DataCollection* gauge = m_opts.vacuumGaugeSensor;
        ctx.pressureVacPa = [gauge] {
            return Reading(gauge->getCurValue() * 133.322, gauge->quality());
        };
    } else {
        qWarning() << "[Вакуум] Датчик ДВ301 (Вакууметр) не задан — форвакуумные "
                      "этапы 11.5–11.7 завершатся по таймауту";
    }

    // ДВ302 (Вакууметр турбо) — тот же прибор, что раньше стоял как ДВ301,
    // теперь на втором тракте за К179. Без него условие У3 гейта перехода
    // не выполняется, и режим остаётся на форвакууме — это безопасный исход.
    if (m_opts.turboGaugeSensor) {
        DataCollection* gauge = m_opts.turboGaugeSensor;
        ctx.pressureTurboPa = [gauge] {
            return Reading(gauge->getCurValue() * 133.322, gauge->quality());
        };
    } else {
        qWarning() << "[Вакуум] Датчик ДВ302 (Вакууметр турбо) не задан — "
                      "перехода на турбомолекулярный насос не будет";
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

    // V-02 (REQ-082/084): подтверждение ФАКТА, а не команды. Без ValveControl
    // шов не ставится вовсе — рецепт тогда откажется открывать К179.
    if (m_cfg.valveControl) {
        ctx.confirmValve = [this](bool expectedOpen, const QString& name) -> bool {
            const bool ok = m_cfg.valveControl->confirmValve(name, expectedOpen);
            if (!ok)
                qWarning() << "[Вакуум] Readback клапана" << name
                           << "не подтвердил состояние" << expectedOpen;
            if (m_cfg.logger && !ok)
                m_cfg.logger->logEvent(m_runId, RegimeLogger::kValveBlocked, -1, -1,
                                       QStringLiteral("readback %1").arg(name));
            return ok;
        };
    }

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
    ctx.onBudget = [this](int elapsedSec, int remainingSec) {
        if (m_monitor)
            m_monitor->onBudget(elapsedSec, remainingSec);
    };
    ctx.onNode = [this](VacuumNode node, NodeState state) {
        if (m_monitor)
            m_monitor->onNode(node, state);
    };
    ctx.onForevacProgress = [this](VacuumNode node, double currentPa,
                                   int heldSec, int elapsedSec) {
        if (m_monitor)
            m_monitor->onForevacProgress(node, currentPa, heldSec, elapsedSec);
    };
    ctx.onForevacDone = [this](VacuumNode node, bool success) {
        if (m_monitor)
            m_monitor->onForevacDone(node, success);
    };
    ctx.onTurboProgress = [this](VacuumNode node, Reading p301, Reading p302,
                                 int heldSec, int elapsedSec) {
        if (m_monitor)
            m_monitor->onTurboProgress(node, p301, p302, heldSec, elapsedSec);
    };
    ctx.onTurboSwitched = [this](bool toTurbo) {
        // Переключение насосного клапана — событие журнала в обе стороны
        // (REQ-082 переход, REQ-083 откат), а не только при успехе.
        qWarning() << "[Вакуум] Переключение насоса:"
                   << (toTurbo ? "форвакуум → турбо" : "турбо → форвакуум");
        if (m_monitor)
            m_monitor->onTurboSwitched(toTurbo);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kValveOpen, -1, -1,
                                   toTurbo ? QStringLiteral("переход на турбонасос (К179)")
                                           : QStringLiteral("откат на форвакуум (К176)"));
    };
    ctx.onFailure = [this](const QString& reason) {
        qWarning() << "[Вакуум] Отказ:" << reason;
        if (m_monitor)
            m_monitor->onFailure(reason);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kRegimeError, -1, -1, reason);
    };
    // Пропуск этапа: в журнал отдельным типом и в UI отдельной строкой. Тихо
    // пропущенный этап читается оператором как выполненный — этого нельзя
    // допускать в первую очередь для проверки герметичности (REQ-062).
    ctx.onStageSkipped = [this](VacuumNode node, const QString& reason) {
        qWarning() << "[Вакуум] Этап пропущен:" << reason;
        if (m_monitor)
            m_monitor->onStageSkipped(int(node), reason);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kStageSkipped, -1, -1, reason);
    };
    ctx.onWarning = [this](const QString& message) {
        qWarning() << "[Вакуум] Предупреждение:" << message;
        if (m_monitor)
            m_monitor->onWarning(message);
        if (m_cfg.logger)
            m_cfg.logger->logEvent(m_runId, RegimeLogger::kWarning, -1, -1, message);
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

void VacuumRegimeWorker::onContinuousPumpingChanged(bool enabled)
{
    if (m_opts.continuousPumping == enabled)
        return;
    m_opts.continuousPumping = enabled;
    // Хвост рецепта читает значение через шов continuousPumpingLive в момент
    // своего выполнения, поэтому пересобирать дерево не нужно.
    qDebug() << "[Вакуум] Непрерывная откачка"
             << (enabled ? "включена" : "выключена") << "на ходу";
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
