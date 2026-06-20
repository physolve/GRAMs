#pragma once

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QList>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "RegimeLogger.h"

// ─────────────────────────────────────────────────────────────────────────────
// ValveStepConfig — one step in the valve test sequence.
//
// Valves in one step open and close simultaneously (parallel).
// Steps execute sequentially.
// ─────────────────────────────────────────────────────────────────────────────

struct ValveStepConfig {
    QStringList valveNames;     // valves opened/closed together in this step
    int pauseBeforeSec = 0;     // wait before opening
    int dwellSec       = 2;     // hold time (open → close)
    int pauseAfterSec  = 0;     // wait after closing
};

// ─────────────────────────────────────────────────────────────────────────────
// ValveTestConfig
// ─────────────────────────────────────────────────────────────────────────────

struct ValveTestConfig {
    int     regimeId   = -1;
    QString regimeName = "Тест клапанов";
    int     totalRepeats            = 1;
    int     globalPauseBeforeSec    = 0;  // pause before first step
    int     globalPauseAfterSec     = 0;  // pause after last step

    QList<ValveStepConfig> steps;

    RegimeManager*   manager         = nullptr;
    ValveControl*    valveControl    = nullptr;
    DataAcquisition* dataAcquisition = nullptr;
    Security*        security        = nullptr;
    RegimeLogger*    logger          = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// ValveTestWorker
//
// QCustomTask<ValveTestWorker> — runs in the main event loop via QTimer.
//
// Per-repeat state machine (1-second ticks):
//   GlobalPauseBefore
//   → for each step:
//       StepPauseBefore  → [open valves] → StepDwelling → [close valves] → StepPauseAfter
//   → GlobalPauseAfter
//   → next repeat or done
//
// Pause: stops the QTimer, sets Paused in RegimeManager.
// Resume: restarts the QTimer.
// Security violation: closes all open valves, marks repeat as error.
// ─────────────────────────────────────────────────────────────────────────────

class ValveTestWorker : public QObject
{
    Q_OBJECT

public:
    explicit ValveTestWorker(QObject* parent = nullptr);
    ~ValveTestWorker() override = default;

    void setConfig(const ValveTestConfig& cfg);
    void start();  // called by QCustomTask

public slots:
    void onPauseRequested();
    void onResumeRequested();

signals:
    void done(bool success);

private slots:
    void tick();

private:
    enum class State {
        GlobalPauseBefore,
        StepPauseBefore,
        StepDwelling,
        StepPauseAfter,
        GlobalPauseAfter,
        Idle
    };

    void startRepeat();
    void enterGlobalPauseBefore();
    void enterStepPauseBefore();
    void openCurrentStepValves();
    void enterStepDwelling();
    void closeCurrentStepValves();
    void enterStepPauseAfter();
    void advanceToNextStep();
    void enterGlobalPauseAfter();
    void finishRepeat(bool success);
    void finishAllRepeats();

    void openValves(const QStringList& names);
    void closeValves(const QStringList& names);
    bool checkSecurity();
    void reportProgress();

    ValveTestConfig m_cfg;
    QTimer          m_timer;

    State   m_state           = State::Idle;
    int     m_currentRepeat   = 0;
    int     m_currentStep     = 0;
    int     m_countdown       = 0;   // ticks remaining in current state
    int     m_dwellElapsed    = 0;   // elapsed during StepDwelling (for progress)
    bool    m_paused          = false;
    int     m_repeatsDone     = 0;
    int     m_repeatsError    = 0;
    qint64  m_runId           = -1;
};
