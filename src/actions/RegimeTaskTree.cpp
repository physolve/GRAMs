#include "RegimeTaskTree.h"

#include <QDebug>
#include <qtasktree.h>

using namespace QtTaskTree;

// ── Custom task type aliases ──────────────────────────────────────────────────
// QCustomTask<T> adapts any QObject with start() + done(bool) to TaskTree.

using VacuumTask    = QCustomTask<VacuumRegimeWorker>;
using RegimeBTask   = QCustomTask<RegimeBWorker>;
using RegimeGTask   = QCustomTask<RegimeGWorker>;
using ValveTestTask = QCustomTask<ValveTestWorker>;
using SupplyTask    = QCustomTask<SupplyRegimeWorker>;
using LeakageTask   = QCustomTask<LeakageRegimeWorker>;

// ═════════════════════════════════════════════════════════════════════════════
// Construction / destruction
// ═════════════════════════════════════════════════════════════════════════════

RegimeTaskTree::RegimeTaskTree(QObject* parent)
    : QObject(parent)
{
    m_logger.init("data/regime_log.db");
    // Стабильная (переживающая пересоздание дерева) шина решений оператора,
    // прокидывается в воркер «Вакуума» и доступна QML как RegimeTaskTree.operatorBus.
    m_vacuumOptions.operatorBus = &m_operatorBus;
}

RegimeTaskTree::~RegimeTaskTree()
{
    cleanupTree();
}

// ── Dependency injection ──────────────────────────────────────────────────────

void RegimeTaskTree::setRegimeManager(RegimeManager* manager)
    { m_manager = manager; }

void RegimeTaskTree::setValveControl(ValveControl* valveControl)
    { m_valveControl = valveControl; }

void RegimeTaskTree::setDataAcquisition(DataAcquisition* dataAcquisition)
    { m_dataAcquisition = dataAcquisition; }

void RegimeTaskTree::setSecurity(Security* security)
    { m_security = security; }

void RegimeTaskTree::setVacuumPressureSensor(DataCollection* sensor)
{
    m_vacuumOptions.pressureSensorB = sensor;
}

void RegimeTaskTree::setVacuumGaugeSensor(DataCollection* sensor)
{
    m_vacuumOptions.vacuumGaugeSensor = sensor;
}

void RegimeTaskTree::setVacuumTurboGaugeSensor(DataCollection* sensor)
{
    m_vacuumOptions.turboGaugeSensor = sensor;
}

void RegimeTaskTree::setVacuumForevac(bool enabled)
{
    m_vacuumOptions.foreVacuum = enabled;
    qDebug() << "RegimeTaskTree: setVacuumForevac" << enabled;
}

void RegimeTaskTree::setVacuumForevacTarget(double targetPa, int holdSec, int timeoutSec)
{
    // Границы намеренно широкие: 1e-4 Па перекрывает турбо-диапазон, верх — грубый
    // форвакуум. Ноль/отрицательное давление недостижимо и повесило бы этап до таймаута.
    m_vacuumOptions.targetVacPa        = qBound(1e-4, targetPa, 1.0e5);
    m_vacuumOptions.turboSwitchHoldSec = qMax(1, holdSec);
    m_vacuumOptions.foreVacTimeoutSec  = qMax(1, timeoutSec);
    m_vacuumMonitor.setForevacTarget(m_vacuumOptions.targetVacPa,
                                     m_vacuumOptions.turboSwitchHoldSec,
                                     m_vacuumOptions.foreVacTimeoutSec);
    qDebug() << "RegimeTaskTree: setVacuumForevacTarget"
             << m_vacuumOptions.targetVacPa << "Pa, hold"
             << m_vacuumOptions.turboSwitchHoldSec << "s, timeout"
             << m_vacuumOptions.foreVacTimeoutSec << "s";
}

void RegimeTaskTree::setVacuumOptions(bool skipRK10, bool skipRK50,
                                      bool skipRK300, bool secondTract)
{
    m_vacuumOptions.skipRK10    = skipRK10;
    m_vacuumOptions.skipRK50    = skipRK50;
    m_vacuumOptions.skipRK300   = skipRK300;
    m_vacuumOptions.secondTract = secondTract;
    qDebug() << "RegimeTaskTree: setVacuumOptions skipRK10=" << skipRK10
             << "skipRK50=" << skipRK50 << "skipRK300=" << skipRK300
             << "secondTract=" << secondTract;
}

void RegimeTaskTree::setVacuumContinuousPumping(bool enabled)
{
    m_vacuumOptions.continuousPumping = enabled;
    // Опция редактируется и на ходу: хвост рецепта выполняется в самом конце
    // прогона, поэтому решение оператора, принятое уже во время режима, обязано
    // до него дойти. Сигнал подхватывает активный воркер (см. buildRegimeGroup).
    emit vacuumContinuousPumpingChanged(enabled);
    qDebug() << "RegimeTaskTree: setVacuumContinuousPumping" << enabled;
}

void RegimeTaskTree::setVacuumTract(const QStringList& generalPumping,
                                    const QStringList& leakTest,
                                    const QStringList& finalPumping,
                                    int testEvacTimeSec, int leakTestDurationSec,
                                    const QMap<QString, double>& dPLeakMax)
{
    m_vacuumOptions.generalPumpingValves = generalPumping;
    m_vacuumOptions.leakTestValves       = leakTest;
    m_vacuumOptions.finalPumpingValves   = finalPumping;
    m_vacuumOptions.testEvacTimeSec      = qMax(1, testEvacTimeSec);
    m_vacuumOptions.leakTestDurationSec  = qMax(1, leakTestDurationSec);
    m_vacuumOptions.dPLeakMax            = dPLeakMax;
    // Пустой набор — не ошибка конфигурации, а осознанное «этап не выполняем».
    // Но оператор обязан узнать об этом до старта, а не по серой строке в
    // развёртке, поэтому предупреждение выводится уже здесь.
    if (generalPumping.isEmpty())
        qWarning() << "RegimeTaskTree: vacuumTract.generalPumping пуст — этап 11.7б "
                      "(общая откачка) выполняться не будет";
    if (finalPumping.isEmpty())
        qWarning() << "RegimeTaskTree: vacuumTract.finalPumping пуст — этапы 11.9-11.10 "
                      "(финальная откачка камеры) выполняться не будут";
    if (leakTest.isEmpty() || dPLeakMax.isEmpty())
        qWarning() << "RegimeTaskTree: проверка герметичности 11.8 не сконфигурирована "
                      "(vacuumTract.leakTest / vacuumSafety.dP_leak_max) — этап будет "
                      "пропущен с указанием причины";
    qDebug() << "RegimeTaskTree: setVacuumTract general=" << generalPumping
             << "leak=" << leakTest << "final=" << finalPumping
             << "testEvac=" << m_vacuumOptions.testEvacTimeSec
             << "leakHold=" << m_vacuumOptions.leakTestDurationSec
             << "dP_leak_max=" << dPLeakMax;
}

void RegimeTaskTree::setVacuumLeakSensor(const QString& name, DataCollection* sensor)
{
    if (name.isEmpty() || !sensor)
        return;
    m_vacuumOptions.leakSensors.insert(name, sensor);
}

void RegimeTaskTree::setVacuumFinalTarget(double targetVacuumPa, int evacTimeSec)
{
    // Границы как у setVacuumForevacTarget: ноль или отрицательная цель
    // недостижимы физически и превратили бы этап в вечное ожидание.
    m_vacuumOptions.targetVacuumPa = qBound(1e-6, targetVacuumPa, 1.0e5);
    m_vacuumOptions.evacTimeSec    = qMax(1, evacTimeSec);
    qDebug() << "RegimeTaskTree: setVacuumFinalTarget" << m_vacuumOptions.targetVacuumPa
             << "Pa," << m_vacuumOptions.evacTimeSec << "s";
}

void RegimeTaskTree::setVacuumSafety(double turboSwitchPressurePa, int turboSwitchHoldSec,
                                     double turboReturnPressurePa, int turboTimeoutSec,
                                     int overrangeWaitSec, int overrangeWaitSec2)
{
    // Границы как у setVacuumForevacTarget: ноль или отрицательное давление
    // недостижимы и подвесили бы этап до таймаута.
    m_vacuumOptions.turboSwitchPressurePa = qBound(1e-4, turboSwitchPressurePa, 1.0e5);
    m_vacuumOptions.turboSwitchHoldSec    = qMax(1, turboSwitchHoldSec);
    m_vacuumOptions.turboReturnPressurePa = qBound(1e-4, turboReturnPressurePa, 1.0e5);
    m_vacuumOptions.turboTimeoutSec       = qMax(1, turboTimeoutSec);
    m_vacuumOptions.overrangeWaitSec      = qMax(1, overrangeWaitSec);
    m_vacuumOptions.overrangeWaitSec2     = qMax(1, overrangeWaitSec2);

    // Гистерезис обязан быть: при равных порогах входа и выхода система у
    // границы начнёт циклически переключать К176 и К179, что опаснее любого
    // из двух устойчивых состояний.
    if (m_vacuumOptions.turboReturnPressurePa <= m_vacuumOptions.turboSwitchPressurePa) {
        qWarning() << "RegimeTaskTree: порог возврата"
                   << m_vacuumOptions.turboReturnPressurePa
                   << "Па не выше порога перехода"
                   << m_vacuumOptions.turboSwitchPressurePa
                   << "Па — гистерезиса нет, возможны частые переключения насосов";
    }
    m_vacuumMonitor.setTurboGate(m_vacuumOptions.turboSwitchPressurePa,
                                 m_vacuumOptions.turboSwitchHoldSec,
                                 m_vacuumOptions.turboTimeoutSec);
    qDebug() << "RegimeTaskTree: setVacuumSafety gate"
             << m_vacuumOptions.turboSwitchPressurePa << "Pa, hold"
             << m_vacuumOptions.turboSwitchHoldSec << "s, return"
             << m_vacuumOptions.turboReturnPressurePa << "Pa";
}

void RegimeTaskTree::setVacuumTurboTract(bool enabled)
{
    m_vacuumOptions.turboTract = enabled;
    qDebug() << "RegimeTaskTree: setVacuumTurboTract" << enabled;
}

// ── Цепочка «Вакуум → Напуск → Натекание» ────────────────────────────────────

void RegimeTaskTree::setSupplyParams(const QString& port, int openTimeMs,
                                     double pressureLimitBar)
{
    m_supplyOptions.port             = port;
    m_supplyOptions.openTimeMs       = qMax(1, openTimeMs);
    // Предел давления зажимаем так же, как цель форвакуума: ноль недостижим и
    // подвесил бы напуск до конца отведённого времени.
    m_supplyOptions.pressureLimitBar = qBound(1e-4, pressureLimitBar, 1.0e3);
    qDebug() << "RegimeTaskTree: setSupplyParams port" << port
             << "time" << m_supplyOptions.openTimeMs << "ms, limit"
             << m_supplyOptions.pressureLimitBar << "bar";
}

void RegimeTaskTree::setLeakageParams(const QString& valve, int durationSec,
                                      double targetDeltaBar)
{
    m_leakageOptions.valve          = valve;
    m_leakageOptions.durationSec    = qMax(1, durationSec);
    // 0 — законное значение: «не использовать перепад как условие остановки».
    m_leakageOptions.targetDeltaBar = qMax(0.0, targetDeltaBar);
    qDebug() << "RegimeTaskTree: setLeakageParams valve" << valve
             << "duration" << m_leakageOptions.durationSec << "s, target dP"
             << m_leakageOptions.targetDeltaBar << "bar";
}

void RegimeTaskTree::setChainGates(double supplyMaxStartBar,
                                   double leakageMinStorageBar,
                                   double leakageMaxReactionBar)
{
    m_supplyOptions.gateMaxStartBar     = qMax(0.0, supplyMaxStartBar);
    m_leakageOptions.gateMinStorageBar  = qMax(0.0, leakageMinStorageBar);
    m_leakageOptions.gateMaxReactionBar = qMax(0.0, leakageMaxReactionBar);
    qDebug() << "RegimeTaskTree: setChainGates supply<=" << supplyMaxStartBar
             << "leakage storage>=" << leakageMinStorageBar
             << "reaction<=" << leakageMaxReactionBar << "bar";
}

void RegimeTaskTree::setSupplySources(DataCollection* storageSensor,
                                      AddRemoveQuartile* addRemoveQuartile)
{
    m_supplyOptions.storageSensor     = storageSensor;
    m_supplyOptions.addRemoveQuartile = addRemoveQuartile;
}

void RegimeTaskTree::setLeakageSources(DataCollection* storageSensor,
                                       DataCollection* reactionSensor,
                                       DataCollection* temperatureSensor,
                                       ReactionQuartile* reactionQuartile)
{
    m_leakageOptions.storageSensor     = storageSensor;
    m_leakageOptions.reactionSensor    = reactionSensor;
    m_leakageOptions.temperatureSensor = temperatureSensor;
    m_leakageOptions.reactionQuartile  = reactionQuartile;
}

void RegimeTaskTree::setVacuumPumpCheck(bool enabled)
{
    m_vacuumOptions.pumpRateCheck = enabled;
    qDebug() << "RegimeTaskTree: setVacuumPumpCheck" << enabled;
}

void RegimeTaskTree::setValveNamesForTest(const QStringList& names)
{
    m_valveNamesForTest = names;

    // Bootstrap default steps: one valve per step, 2 s dwell, no pauses.
    m_valveTestSteps.clear();
    for (const QString& name : names) {
        ValveStepConfig step;
        step.valveNames    = { name };
        step.pauseBeforeSec = 0;
        step.dwellSec       = 2;
        step.pauseAfterSec  = 0;
        m_valveTestSteps.append(step);
    }
}

void RegimeTaskTree::setValveTestSteps(const QVariantList& steps,
                                        int repeats,
                                        int globalPauseBefore,
                                        int globalPauseAfter)
{
    m_valveTestSteps.clear();

    for (const QVariant& v : steps) {
        QVariantMap m = v.toMap();
        ValveStepConfig step;
        step.valveNames     = m.value("valves").toStringList();
        step.pauseBeforeSec = m.value("pauseBefore", 0).toInt();
        step.dwellSec       = m.value("dwell",       2).toInt();
        step.pauseAfterSec  = m.value("pauseAfter",  0).toInt();
        if (!step.valveNames.isEmpty())
            m_valveTestSteps.append(step);
    }

    m_valveTestRepeats           = qMax(1, repeats);
    m_valveTestGlobalPauseBefore = qMax(0, globalPauseBefore);
    m_valveTestGlobalPauseAfter  = qMax(0, globalPauseAfter);

    qDebug() << "RegimeTaskTree: setValveTestSteps" << m_valveTestSteps.size()
             << "steps, repeats=" << m_valveTestRepeats;
}

// ── Control ───────────────────────────────────────────────────────────────────

void RegimeTaskTree::startAll()
{
    startFrom(0);
}

void RegimeTaskTree::startFrom(int startRegimeId)
{
    if (m_running) {
        qWarning() << "RegimeTaskTree: already running";
        return;
    }
    if (!m_manager) {
        qWarning() << "RegimeTaskTree: RegimeManager not set";
        return;
    }

    Group recipe = buildSequence(startRegimeId);

    cleanupTree();
    m_tree = new QTaskTree(recipe, this);

    connect(m_tree, &QTaskTree::done, this, [this](DoneWith result) {
        bool success = (result == DoneWith::Success);

        qDebug() << "RegimeTaskTree: sequence finished,"
                 << (result == DoneWith::Success ? "success"
                   : result == DoneWith::Error   ? "error"
                                                 : "cancelled")
                 << "—" << (m_vacuumMonitor.finishReason().isEmpty()
                                ? QStringLiteral("причина не зафиксирована")
                                : m_vacuumMonitor.finishReason());

        emit executionFinished(success);
        setRunning(false);
        setPaused(false);

        // docs: do not delete from done() handler — use deleteLater()
        if (m_tree) {
            m_tree->deleteLater();
            m_tree = nullptr;
        }
    });

    setRunning(true, startRegimeId);
    m_tree->start();
}

void RegimeTaskTree::pause()
{
    if (!m_running || m_paused) return;
    setPaused(true);
    emit pauseRequested();
    qDebug() << "RegimeTaskTree: pause requested";
}

void RegimeTaskTree::resume()
{
    if (!m_running || !m_paused) return;
    setPaused(false);
    emit resumeRequested();
    qDebug() << "RegimeTaskTree: resume requested";
}

void RegimeTaskTree::stop()
{
    if (!m_running || !m_tree) return;
    qDebug() << "RegimeTaskTree: stop requested";
    // Если стоим на паузе — сначала снимаем её через штатный канал, чтобы
    // замороженные тикеры (PausableTicker) не тирдаунились в замороженном
    // состоянии при cancel(). Вместе с отложенным cancelTree это устраняет
    // краш «Nested execution of handlers» при pause→stop.
    if (m_paused)
        emit resumeRequested();
    // cancel() is synchronous — done handlers fire with DoneWith::Cancel.
    m_tree->cancel();
    setRunning(false);
    setPaused(false);
}

// ═════════════════════════════════════════════════════════════════════════════
// Recipe builders
// ═════════════════════════════════════════════════════════════════════════════

Group RegimeTaskTree::buildSequence(int startFromId)
{
    const QList<Regime> regimes = m_manager->model()->getRegimes();

    GroupItems items { sequential };

    for (int i = startFromId; i < regimes.size(); ++i) {
        const Regime& regime = regimes.at(i);
        if (regime.m_state != RegimeEnums::State::Waiting)
            continue;
        items << buildRegimeGroup(i, regime);
    }

    if (items.size() == 1) {
        // Nothing to run — produce an immediately-successful group
        items << onGroupSetup([]() -> SetupResult {
            qDebug() << "RegimeTaskTree: no waiting regimes";
            return SetupResult::StopWithSuccess;
        });
    }

    return Group(items);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildRegimeGroup  — single regime wrapper
//
// TaskTree lifecycle per regime:
//   1. setup handler  → startRegimeExecution() in RegimeManager
//                       + connect pause/resume signals to worker
//   2. worker.start() → condition + execution phases (all repeats)
//   3. done handler   → completeRegimeExecution() or Error/Stopped
//
// DoneWith values from QCustomTask:
//   Success  → worker emitted done(true)
//   Error    → worker emitted done(false)
//   Cancel   → QTaskTree::cancel() was called (stop() or destructor)
// ─────────────────────────────────────────────────────────────────────────────

Group RegimeTaskTree::buildRegimeGroup(int regimeId, const Regime& regime)
{
    const QString name = regime.m_name;

    // ── Done handler — shared across all regime types ─────────────────────────
    auto doneFn = [regimeId, name, this](DoneWith result) {
        // Причина завершения: показывается в UI рядом с итогом режима, чтобы
        // «завершён с ошибкой» не приходилось выяснять по логу.
        if (result == DoneWith::Success) {
            m_manager->completeRegimeExecution(regimeId);
            m_vacuumMonitor.setFinish(int(RegimeEnums::State::Done),
                                      QStringLiteral("Режим «%1» завершён успешно").arg(name));
        } else if (result == DoneWith::Cancel) {
            m_manager->setRegimeState(regimeId, RegimeEnums::State::Stopped);
            if (m_logger.isOpen())
                m_logger.logEvent(-1, RegimeLogger::kCancelled, -1, -1, name);
            m_vacuumMonitor.setFinish(int(RegimeEnums::State::Stopped),
                                      QStringLiteral("Режим «%1» остановлен оператором").arg(name));
        } else { // Error
            m_manager->setRegimeState(regimeId, RegimeEnums::State::Error);
            // Пустая строка ⇒ монитор подставит накопленную причину отказа
            // (детальную из рецепта либо имя упавшего узла).
            m_vacuumMonitor.setFinish(int(RegimeEnums::State::Error), QString());
            qWarning() << "RegimeTaskTree: regime" << regimeId << name
                       << "finished with error —" << m_vacuumMonitor.finishReason();
        }
        emit regimeFinished(regimeId, result == DoneWith::Success);
    };

    // ── Dispatch on regime name ───────────────────────────────────────────────

    if (name == "Вакуум") {
        RegimeWorkerConfig cfg  = makeConfig(regimeId, regime);
        VacuumOptions      opts = m_vacuumOptions;

        return Group {
            sequential,
            VacuumTask(
                [cfg, opts, regimeId, name, this](VacuumRegimeWorker& w) -> SetupResult {
                    if (!m_manager->startRegimeExecution(regimeId)) {
                        qWarning() << "RegimeTaskTree: startRegimeExecution failed for" << name;
                        return SetupResult::StopWithError;
                    }
                    w.setConfig(cfg);
                    w.setVacuumOptions(opts);
                    m_vacuumMonitor.beginRun(cfg.totalRepeats);
                    // Цель форвакуума видна в развёртке до входа в 11.5.
                    // Цель форвакуума и гейт турбо видны в развёртке до входа
                    // в соответствующие этапы — это разные величины.
                    m_vacuumMonitor.setTurboGate(opts.turboSwitchPressurePa,
                                                 opts.turboSwitchHoldSec,
                                                 opts.turboTimeoutSec);
                    m_vacuumMonitor.setForevacTarget(opts.targetVacPa,
                                                     opts.turboSwitchHoldSec,
                                                     opts.foreVacTimeoutSec);
                    m_vacuumMonitor.setPaused(false);
                    m_vacuumMonitor.setRunning(true);
                    w.setMonitor(&m_vacuumMonitor);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &VacuumRegimeWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &VacuumRegimeWorker::onResumeRequested);
                    connect(this, &RegimeTaskTree::vacuumContinuousPumpingChanged,
                            &w,   &VacuumRegimeWorker::onContinuousPumpingChanged);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const VacuumRegimeWorker& w, DoneWith r) {
                    // При отмене внешнего дерева воркер уничтожается отложенно
                    // (deleteLater). Внутреннее дерево отменяем ОТЛОЖЕННО через
                    // очередь событий: синхронный вызов cancelTree() отсюда
                    // вложил бы отмену внутреннего дерева в done-хендлер внешнего
                    // («Nested execution of handlers» → краш, особенно при
                    // активной паузе — замороженный PausableTicker/QBarrier).
                    // QueuedConnection выполнит cancelTree() после раскрутки
                    // внешнего хендлера; деструктор воркера — страховка.
                    if (r == DoneWith::Cancel)
                        QMetaObject::invokeMethod(const_cast<VacuumRegimeWorker*>(&w),
                                                  "cancelTree", Qt::QueuedConnection);
                    doneFn(r);
                }
            )
        };
    }

    if (name == "Режим в") {
        RegimeWorkerConfig cfg = makeConfig(regimeId, regime);

        return Group {
            sequential,
            RegimeBTask(
                [cfg, regimeId, name, this](RegimeBWorker& w) -> SetupResult {
                    if (!m_manager->startRegimeExecution(regimeId))
                        return SetupResult::StopWithError;
                    w.setConfig(cfg);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &RegimeBWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &RegimeBWorker::onResumeRequested);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const RegimeBWorker&, DoneWith r) { doneFn(r); }
            )
        };
    }

    if (name == "Режим г") {
        RegimeWorkerConfig cfg = makeConfig(regimeId, regime);

        return Group {
            sequential,
            RegimeGTask(
                [cfg, regimeId, name, this](RegimeGWorker& w) -> SetupResult {
                    if (!m_manager->startRegimeExecution(regimeId))
                        return SetupResult::StopWithError;
                    w.setConfig(cfg);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &RegimeGWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &RegimeGWorker::onResumeRequested);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const RegimeGWorker&, DoneWith r) { doneFn(r); }
            )
        };
    }

    if (name == "Тест клапанов") {
        ValveTestConfig cfg = makeValveTestConfig(regimeId, regime);

        return Group {
            sequential,
            ValveTestTask(
                [cfg, regimeId, name, this](ValveTestWorker& w) -> SetupResult {
                    if (cfg.steps.isEmpty()) {
                        qWarning() << "RegimeTaskTree: no steps for 'Тест клапанов'";
                        return SetupResult::StopWithError;
                    }
                    if (!m_manager->startRegimeExecution(regimeId))
                        return SetupResult::StopWithError;
                    w.setConfig(cfg);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &ValveTestWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &ValveTestWorker::onResumeRequested);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const ValveTestWorker&, DoneWith r) { doneFn(r); }
            )
        };
    }

    if (name == "Напуск") {
        RegimeWorkerConfig cfg = makeConfig(regimeId, regime);
        SupplyOptions opts = m_supplyOptions;

        return Group {
            sequential,
            SupplyTask(
                [cfg, opts, regimeId, name, this](SupplyRegimeWorker& w) -> SetupResult {
                    if (!m_manager->startRegimeExecution(regimeId))
                        return SetupResult::StopWithError;
                    w.setConfig(cfg);
                    w.setOptions(opts);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &SupplyRegimeWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &SupplyRegimeWorker::onResumeRequested);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const SupplyRegimeWorker& w, DoneWith r) {
                    if (r == DoneWith::Cancel)
                        QMetaObject::invokeMethod(const_cast<SupplyRegimeWorker*>(&w),
                                                  "cancelTree", Qt::QueuedConnection);
                    doneFn(r);
                }
            )
        };
    }

    if (name == "Натекание") {
        RegimeWorkerConfig cfg = makeConfig(regimeId, regime);
        LeakageOptions opts = m_leakageOptions;

        return Group {
            sequential,
            LeakageTask(
                [cfg, opts, regimeId, name, this](LeakageRegimeWorker& w) -> SetupResult {
                    if (!m_manager->startRegimeExecution(regimeId))
                        return SetupResult::StopWithError;
                    w.setConfig(cfg);
                    w.setOptions(opts);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &LeakageRegimeWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &LeakageRegimeWorker::onResumeRequested);
                    m_activeRegimeId = regimeId;
                    emit activeRegimeChanged();
                    emit regimeStarted(regimeId, name);
                    return SetupResult::Continue;
                },
                [doneFn](const LeakageRegimeWorker& w, DoneWith r) {
                    if (r == DoneWith::Cancel)
                        QMetaObject::invokeMethod(const_cast<LeakageRegimeWorker*>(&w),
                                                  "cancelTree", Qt::QueuedConnection);
                    doneFn(r);
                }
            )
        };
    }

    // ── Неизвестное имя режима — ОШИБКА, а не тихий пропуск ───────────────────
    //
    // Раньше здесь стоял StopWithSuccess: режим с незнакомым именем молча
    // «выполнялся». Профильные «Режим а»/«Режим б» из regime_a.json именно так
    // и проходили — оператор видел успех там, где не выполнялось ничего.
    qWarning() << "RegimeTaskTree: неизвестное имя режима" << name;
    return Group {
        onGroupSetup([]() -> SetupResult { return SetupResult::Continue; }),
        QSyncTask([this, regimeId, name]() -> bool {
            m_manager->setRegimeState(regimeId, RegimeEnums::State::Error);
            m_vacuumMonitor.setFinish(int(RegimeEnums::State::Error),
                                      QStringLiteral("Неизвестный режим «%1»: воркер "
                                                     "не зарегистрирован в "
                                                     "RegimeTaskTree::buildRegimeGroup()")
                                          .arg(name));
            return false;
        })
    };
}

// ── Config factories ──────────────────────────────────────────────────────────

RegimeWorkerConfig RegimeTaskTree::makeConfig(int regimeId, const Regime& regime) const
{
    RegimeWorkerConfig cfg;
    cfg.regimeId            = regimeId;
    cfg.regimeName          = regime.m_name;
    cfg.totalRepeats        = regime.m_repeatCount;
    cfg.maxTimeSec          = regime.m_maxTime;
    cfg.tickIntervalMs      = 1000;
    cfg.conditionType       = regime.m_condition.type;
    cfg.conditionTimeSec    = regime.m_condition.time * 60;
    cfg.conditionTargetTemp = regime.m_condition.temp;
    cfg.manager             = m_manager;
    cfg.valveControl        = m_valveControl;
    cfg.dataAcquisition     = m_dataAcquisition;
    cfg.security            = m_security;
    cfg.logger              = m_logger.isOpen()
                                  ? const_cast<RegimeLogger*>(&m_logger)
                                  : nullptr;
    return cfg;
}

ValveTestConfig RegimeTaskTree::makeValveTestConfig(int regimeId, const Regime& regime) const
{
    ValveTestConfig cfg;
    cfg.regimeId              = regimeId;
    cfg.regimeName            = regime.m_name;
    cfg.totalRepeats          = m_valveTestRepeats;
    cfg.globalPauseBeforeSec  = m_valveTestGlobalPauseBefore;
    cfg.globalPauseAfterSec   = m_valveTestGlobalPauseAfter;
    cfg.steps                 = m_valveTestSteps;
    cfg.manager               = m_manager;
    cfg.valveControl          = m_valveControl;
    cfg.dataAcquisition       = m_dataAcquisition;
    cfg.security              = m_security;
    cfg.logger                = m_logger.isOpen()
                                    ? const_cast<RegimeLogger*>(&m_logger)
                                    : nullptr;
    return cfg;
}

// ── State helpers ─────────────────────────────────────────────────────────────

void RegimeTaskTree::setRunning(bool running, int activeRegimeId)
{
    if (m_running != running) {
        m_running = running;
        emit runningChanged();
    }
    if (!running) {
        m_activeRegimeId = -1;
        emit activeRegimeChanged();
        m_vacuumMonitor.setRunning(false);
    } else if (activeRegimeId >= 0) {
        m_activeRegimeId = activeRegimeId;
    }
}

void RegimeTaskTree::setPaused(bool paused)
{
    if (m_paused != paused) {
        m_paused = paused;
        emit pausedChanged();
    }
    m_vacuumMonitor.setPaused(paused);
}

void RegimeTaskTree::cleanupTree()
{
    if (!m_tree) return;
    delete m_tree;   // QTaskTree destructor safely cancels any running tree
    m_tree = nullptr;
    m_running = false;
    m_paused  = false;
}
