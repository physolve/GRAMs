#include "RegimeTaskTree.h"

#include <QDebug>
#include <qtasktree.h>

using namespace QtTaskTree;

// ── Custom task type aliases ──────────────────────────────────────────────────
//
// QCustomTask<T> requires T: QObject, T::start(), T::done(bool) signal.
// Workers satisfy all three conditions via RegimeWorkerBase.

using VacuumTask  = QCustomTask<VacuumRegimeWorker>;
using RegimeBTask = QCustomTask<RegimeBWorker>;
using RegimeGTask = QCustomTask<RegimeGWorker>;

// ═════════════════════════════════════════════════════════════════════════════
// RegimeTaskTree
// ═════════════════════════════════════════════════════════════════════════════

RegimeTaskTree::RegimeTaskTree(QObject* parent)
    : QObject(parent)
{}

RegimeTaskTree::~RegimeTaskTree()
{
    cleanupTree();
}

// ── Dependency injection ──────────────────────────────────────────────────────

void RegimeTaskTree::setRegimeManager(RegimeManager* manager)
{
    m_manager = manager;
}

void RegimeTaskTree::setValveControl(ValveControl* valveControl)
{
    m_valveControl = valveControl;
}

void RegimeTaskTree::setDataAcquisition(DataAcquisition* dataAcquisition)
{
    m_dataAcquisition = dataAcquisition;
}

void RegimeTaskTree::setSecurity(Security* security)
{
    m_security = security;
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
        qDebug() << "RegimeTaskTree: execution finished, success=" << success;
        emit executionFinished(success);
        setRunning(false);
        // docs: do not delete from done() handler — use deleteLater()
        m_tree->deleteLater();
        m_tree = nullptr;
    });

    setRunning(true, startRegimeId);
    m_tree->start();
}

void RegimeTaskTree::stop()
{
    if (!m_running || !m_tree)
        return;

    qDebug() << "RegimeTaskTree: stop requested";
    // cancel() is synchronous — all handlers finish before this returns
    m_tree->cancel();
    // The done() signal is NOT emitted after cancel from destructor path,
    // but QTaskTree::cancel() DOES invoke done handlers for cleanup.
    // Reset state manually here:
    setRunning(false);
}

// ── Recipe builders ───────────────────────────────────────────────────────────

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
        // No waiting regimes — produce an immediately-successful empty group
        items << onGroupSetup([]() -> SetupResult {
            qDebug() << "RegimeTaskTree: no waiting regimes";
            return SetupResult::StopWithSuccess;
        });
    }

    return Group(items);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildRegimeGroup
//
// Each regime gets a Group that:
//   1. onGroupSetup  → calls startRegimeExecution() on RegimeManager
//   2. XxxTask       → worker runs condition + execution phases (all repeats)
//   3. onGroupDone   → calls completeRegimeExecution() or Error state
//
// The setup handler returns SetupResult, allowing the group to be skipped when
// startRegimeExecution() fails (e.g. hardware not connected).
// ─────────────────────────────────────────────────────────────────────────────

Group RegimeTaskTree::buildRegimeGroup(int regimeId, const Regime& regime)
{
    RegimeWorkerConfig cfg = makeConfig(regimeId, regime);
    const QString name = regime.m_name;

    // ── Common setup & done lambdas ──────────────────────────────────────────

    // Called by TaskTree before worker.start(). Returns SetupResult.
    auto setupFn = [cfg, name, this](auto& worker) -> SetupResult {
        if (!cfg.manager->startRegimeExecution(cfg.regimeId)) {
            qWarning() << "RegimeTaskTree: startRegimeExecution failed for" << name;
            return SetupResult::StopWithError;
        }
        worker.setConfig(cfg);
        m_activeRegimeId = cfg.regimeId;
        emit activeRegimeChanged();
        emit regimeStarted(cfg.regimeId, name);
        qDebug() << "RegimeTaskTree: starting regime" << name << "id=" << cfg.regimeId;
        return SetupResult::Continue;
    };

    // Called by TaskTree after worker.done() — result already written by worker.
    auto doneFn = [cfg, name, this](const auto& /*worker*/, DoneWith result) {
        bool ok = (result == DoneWith::Success);
        qDebug() << "RegimeTaskTree: regime" << name << (ok ? "OK" : "FAILED");

        if (!ok) {
            // Worker already called markRepeatAsError; set final Error state.
            cfg.manager->setRegimeState(cfg.regimeId, RegimeEnums::State::Error);
        } else {
            cfg.manager->completeRegimeExecution(cfg.regimeId);
        }
        emit regimeFinished(cfg.regimeId, ok);
    };

    // ── Dispatch to the correct worker type ──────────────────────────────────

    if (name == "Вакуум") {
        return Group {
            sequential,
            VacuumTask(
                [setupFn](VacuumRegimeWorker& w) -> SetupResult { return setupFn(w); },
                [doneFn] (const VacuumRegimeWorker& w, DoneWith r) { doneFn(w, r); }
            )
        };
    }

    if (name == "Режим в") {
        return Group {
            sequential,
            RegimeBTask(
                [setupFn](RegimeBWorker& w) -> SetupResult { return setupFn(w); },
                [doneFn] (const RegimeBWorker& w, DoneWith r) { doneFn(w, r); }
            )
        };
    }

    if (name == "Режим г") {
        return Group {
            sequential,
            RegimeGTask(
                [setupFn](RegimeGWorker& w) -> SetupResult { return setupFn(w); },
                [doneFn] (const RegimeGWorker& w, DoneWith r) { doneFn(w, r); }
            )
        };
    }

    // Unknown regime name — skip silently
    qWarning() << "RegimeTaskTree: unknown regime name" << name << "— skipping";
    return Group {
        onGroupSetup([name]() -> SetupResult {
            qDebug() << "Skipping unknown regime:" << name;
            return SetupResult::StopWithSuccess;
        })
    };
}

// ── Helpers ───────────────────────────────────────────────────────────────────

RegimeWorkerConfig RegimeTaskTree::makeConfig(int regimeId, const Regime& regime) const
{
    RegimeWorkerConfig cfg;
    cfg.regimeId              = regimeId;
    cfg.totalRepeats          = regime.m_repeatCount;
    cfg.maxTimeSec            = regime.m_maxTime;
    cfg.conditionType         = regime.m_condition.type;
    cfg.conditionTimeSec      = regime.m_condition.time * 60;
    cfg.conditionTargetTemp   = regime.m_condition.temp;
    cfg.manager               = m_manager;
    cfg.valveControl          = m_valveControl;
    cfg.dataAcquisition       = m_dataAcquisition;
    cfg.security              = m_security;
    cfg.conditionTempSensor   = nullptr; // wire per-regime when needed
    return cfg;
}

void RegimeTaskTree::setRunning(bool running, int activeRegimeId)
{
    if (m_running != running) {
        m_running = running;
        emit runningChanged();
    }
    if (!running) {
        m_activeRegimeId = -1;
        emit activeRegimeChanged();
    } else {
        m_activeRegimeId = activeRegimeId;
    }
}

void RegimeTaskTree::cleanupTree()
{
    if (!m_tree)
        return;

    // Destructor of QTaskTree safely cancels any running tree.
    // Called from our destructor or before re-start.
    delete m_tree;
    m_tree = nullptr;
    m_running = false;
}
