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

void RegimeTaskTree::setVacuumForevac(bool enabled)
{
    m_vacuumOptions.foreVacuum = enabled;
    qDebug() << "RegimeTaskTree: setVacuumForevac" << enabled;
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

void RegimeTaskTree::setVacuumStepPauseMs(int ms)
{
    m_vacuumOptions.perActionPauseMs = qMax(0, ms);
    qDebug() << "RegimeTaskTree: setVacuumStepPauseMs" << m_vacuumOptions.perActionPauseMs;
}

// Сопоставление строковых ключей шагов с узлами рецепта (VacuumNode).
static const QList<QPair<QString, VacuumNode>>& vacuumStepMap()
{
    static const QList<QPair<QString, VacuumNode>> kMap = {
        { QStringLiteral("f1"),          VacuumNode::F1Relief },
        { QStringLiteral("rk300"),       VacuumNode::F2_RK300 },
        { QStringLiteral("rk10"),        VacuumNode::F2_RK10 },
        { QStringLiteral("rk50"),        VacuumNode::F2_RK50 },
        { QStringLiteral("reliefMid"),   VacuumNode::F2_ReliefMid },
        { QStringLiteral("blockC"),      VacuumNode::F2BlockC },
        { QStringLiteral("secondTract"), VacuumNode::F3SecondTract },
        { QStringLiteral("a1"),          VacuumNode::F5A1 },
        { QStringLiteral("bc"),          VacuumNode::F6BC },
        { QStringLiteral("ef"),          VacuumNode::F7EF },
    };
    return kMap;
}

void RegimeTaskTree::setVacuumStepDelays(const QVariantMap& secondsByStep)
{
    m_vacuumOptions.stepPauseMs.clear();
    for (const auto& pair : vacuumStepMap()) {
        if (secondsByStep.contains(pair.first)) {
            const int sec = qMax(0, secondsByStep.value(pair.first).toInt());
            m_vacuumOptions.stepPauseMs.insert(int(pair.second), sec * 1000);
        }
    }
    qDebug() << "RegimeTaskTree: setVacuumStepDelays" << m_vacuumOptions.stepPauseMs.size()
             << "steps";
}

QVariantList RegimeTaskTree::vacuumStepKeys() const
{
    // {key, label} для построения таблицы задержек в QML.
    static const QList<QPair<QString, QString>> kLabels = {
        { QStringLiteral("f1"),          QStringLiteral("Ф1 сброс К118") },
        { QStringLiteral("rk300"),       QStringLiteral("C1 / RK300 (S3)") },
        { QStringLiteral("rk10"),        QStringLiteral("C3 / RK10 (S1)") },
        { QStringLiteral("rk50"),        QStringLiteral("C2 / RK50 (S2)") },
        { QStringLiteral("reliefMid"),   QStringLiteral("Ф2 средний сброс") },
        { QStringLiteral("blockC"),      QStringLiteral("Ф2 блок C (итог)") },
        { QStringLiteral("secondTract"), QStringLiteral("Ф3 второй тракт") },
        { QStringLiteral("a1"),          QStringLiteral("11.5 форвакуум A1") },
        { QStringLiteral("bc"),          QStringLiteral("11.6 форвакуум B/C") },
        { QStringLiteral("ef"),          QStringLiteral("11.7 форвакуум E/F") },
    };
    QVariantList out;
    for (const auto& p : kLabels) {
        QVariantMap m;
        m["key"]   = p.first;
        m["label"] = p.second;
        out.append(m);
    }
    return out;
}

void RegimeTaskTree::setVacuumReliefHoldSec(int sec)
{
    m_vacuumOptions.reliefDwellSec = qMax(1, sec);
    qDebug() << "RegimeTaskTree: setVacuumReliefHoldSec" << m_vacuumOptions.reliefDwellSec;
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
                                                 : "cancelled");

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
        if (result == DoneWith::Success) {
            m_manager->completeRegimeExecution(regimeId);
        } else if (result == DoneWith::Cancel) {
            m_manager->setRegimeState(regimeId, RegimeEnums::State::Stopped);
            if (m_logger.isOpen())
                m_logger.logEvent(-1, RegimeLogger::kCancelled, -1, -1, name);
        } else { // Error
            m_manager->setRegimeState(regimeId, RegimeEnums::State::Error);
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
                    m_vacuumMonitor.setPaused(false);
                    m_vacuumMonitor.setRunning(true);
                    w.setMonitor(&m_vacuumMonitor);
                    connect(this, &RegimeTaskTree::pauseRequested,
                            &w,   &VacuumRegimeWorker::onPauseRequested);
                    connect(this, &RegimeTaskTree::resumeRequested,
                            &w,   &VacuumRegimeWorker::onResumeRequested);
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

    // ── Unknown regime name — skip silently ───────────────────────────────────
    qWarning() << "RegimeTaskTree: unknown regime name" << name << "— skipping";
    return Group {
        onGroupSetup([name]() -> SetupResult {
            qDebug() << "Skipping unknown regime:" << name;
            return SetupResult::StopWithSuccess;
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
