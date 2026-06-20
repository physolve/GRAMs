#pragma once

#include <QObject>
#include <qtasktree.h>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "RegimeWorkers.h"

// ─────────────────────────────────────────────────────────────────────────────
// RegimeTaskTree
//
// Orchestrates sequential execution of regimes listed in RegimeManager using
// Qt TaskTree (Qt 6.11, Technical Preview).
//
// One QTaskTree is created per run. It holds a sequential Group where each
// child Group corresponds to one waiting regime. Each regime Group wraps a
// QCustomTask<XxxRegimeWorker> that runs in the main event loop via QTimer.
//
// QML registration (add to Grams.cpp):
//   qmlRegisterSingletonInstance("GRAMs", 1, 0, "RegimeTaskTree", &m_regimeTaskTree);
//
// QML usage example:
//   RegimeTaskTree.startAll()
//   RegimeTaskTree.stop()
//   if (RegimeTaskTree.running) { ... }
// ─────────────────────────────────────────────────────────────────────────────

class RegimeTaskTree : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running       READ isRunning       NOTIFY runningChanged)
    Q_PROPERTY(int  activeRegime  READ activeRegimeId  NOTIFY activeRegimeChanged)

public:
    explicit RegimeTaskTree(QObject* parent = nullptr);
    ~RegimeTaskTree() override;

    // ── Dependency injection (call before startAll) ──────────────────────────
    void setRegimeManager  (RegimeManager*   manager);
    void setValveControl   (ValveControl*    valveControl);
    void setDataAcquisition(DataAcquisition* dataAcquisition);
    void setSecurity       (Security*        security);

    // ── Properties ───────────────────────────────────────────────────────────
    bool isRunning()      const { return m_running; }
    int  activeRegimeId() const { return m_activeRegimeId; }

public slots:
    // ── QML-invokable control ─────────────────────────────────────────────────
    Q_INVOKABLE void startAll();            // run all Waiting regimes in sequence
    Q_INVOKABLE void startFrom(int regimeId); // run from a specific regime index
    Q_INVOKABLE void stop();               // cancel running tree

signals:
    void runningChanged();
    void activeRegimeChanged();

    void regimeStarted (int regimeId, const QString& name);
    void regimeFinished(int regimeId, bool success);
    void executionFinished(bool allSuccess);

private:
    // ── Recipe builders ──────────────────────────────────────────────────────
    QtTaskTree::Group buildSequence(int startFromId);
    QtTaskTree::Group buildRegimeGroup(int regimeId, const Regime& regime);

    // ── Helpers ──────────────────────────────────────────────────────────────
    RegimeWorkerConfig makeConfig(int regimeId, const Regime& regime) const;
    void setRunning(bool running, int activeRegimeId = -1);
    void cleanupTree();

    // ── State ────────────────────────────────────────────────────────────────
    RegimeManager*   m_manager         = nullptr;
    ValveControl*    m_valveControl    = nullptr;
    DataAcquisition* m_dataAcquisition = nullptr;
    Security*        m_security        = nullptr;

    QtTaskTree::QTaskTree* m_tree           = nullptr;
    bool                   m_running        = false;
    int                    m_activeRegimeId = -1;
};
