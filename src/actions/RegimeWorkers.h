#pragma once

#include <QObject>
#include <QTimer>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "RegimeLogger.h"

// ─── Configuration injected by TaskTree setup handler ─────────────────────────

struct RegimeWorkerConfig {
    int     regimeId   = -1;
    QString regimeName;        // for DB logging

    int totalRepeats     = 1;
    int maxTimeSec       = 60;
    int tickIntervalMs   = 1000; // QTimer interval; override per worker type

    QString conditionType        = "none";   // "none" | "time" | "temp"
    int     conditionTimeSec     = 0;
    double  conditionTargetTemp  = 0.0;

    // Optional temp sensor for "temp" condition type
    DataCollection* conditionTempSensor = nullptr;

    // Injected dependencies (not owned)
    RegimeManager*   manager         = nullptr;
    ValveControl*    valveControl    = nullptr;
    DataAcquisition* dataAcquisition = nullptr;
    Security*        security        = nullptr;
    RegimeLogger*    logger          = nullptr;
};

// ─── Base worker ──────────────────────────────────────────────────────────────
//
// QCustomTask<T> requires:
//   • T derives from QObject
//   • T has a public void start() method
//   • T emits void done(bool) when finished
//
// Workers run in the main event loop (QTimer). RegimeManager calls are safe.
// Pause/Resume: connect RegimeTaskTree::pauseRequested / resumeRequested
//               to onPauseRequested / onResumeRequested before start().

class RegimeWorkerBase : public QObject
{
    Q_OBJECT
public:
    explicit RegimeWorkerBase(QObject* parent = nullptr);
    ~RegimeWorkerBase() override = default;

    void setConfig(const RegimeWorkerConfig& cfg);
    void start();   // called by QCustomTask / TaskTree

signals:
    void done(bool success);

public slots:
    void onPauseRequested();
    void onResumeRequested();

protected:
    // Override in concrete workers for regime-specific logic.
    virtual void onExecutionPhaseStart();
    virtual void onExecutionTick(int elapsedSec);
    virtual bool isExecutionComplete(int elapsedSec) const;
    virtual void onExecutionPhaseEnd(bool success);

    RegimeWorkerConfig m_cfg;
    qint64             m_runId          = -1;

    // Accessible by subclasses for logging in onExecutionPhaseEnd() etc.
    int  m_currentRepeat   = 0;
    int  m_phaseElapsedSec = 0;

private slots:
    void tick();

private:
    void startNextRepeat();
    void enterConditionPhase();
    void enterExecutionPhase();
    void finishRepeat(bool success);

    enum class Phase { Condition, Execution };

    QTimer m_timer;
    int    m_repeatsDone  = 0;
    int    m_repeatsError = 0;
    bool   m_paused       = false;
    Phase  m_phase        = Phase::Condition;
};

// ─── Вакуум ───────────────────────────────────────────────────────────────────

class VacuumRegimeWorker : public RegimeWorkerBase
{
    Q_OBJECT
public:
    explicit VacuumRegimeWorker(QObject* parent = nullptr)
        : RegimeWorkerBase(parent) {}

protected:
    void onExecutionPhaseStart() override;
    void onExecutionTick(int elapsedSec) override;
    bool isExecutionComplete(int elapsedSec) const override;
    void onExecutionPhaseEnd(bool success) override;
};

// ─── Режим в ─────────────────────────────────────────────────────────────────

class RegimeBWorker : public RegimeWorkerBase
{
    Q_OBJECT
public:
    explicit RegimeBWorker(QObject* parent = nullptr)
        : RegimeWorkerBase(parent) {}

protected:
    void onExecutionPhaseStart() override;
    void onExecutionTick(int elapsedSec) override;
    bool isExecutionComplete(int elapsedSec) const override;
    void onExecutionPhaseEnd(bool success) override;
};

// ─── Режим г ─────────────────────────────────────────────────────────────────

class RegimeGWorker : public RegimeWorkerBase
{
    Q_OBJECT
public:
    explicit RegimeGWorker(QObject* parent = nullptr)
        : RegimeWorkerBase(parent) {}

protected:
    void onExecutionPhaseStart() override;
    void onExecutionTick(int elapsedSec) override;
    bool isExecutionComplete(int elapsedSec) const override;
    void onExecutionPhaseEnd(bool success) override;
};
