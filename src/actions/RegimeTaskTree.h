#pragma once

#include <QObject>
#include <QStringList>
#include <qtasktree.h>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "RegimeWorkers.h"
#include "ValveTestWorker.h"
#include "RegimeLogger.h"

// ─────────────────────────────────────────────────────────────────────────────
// RegimeTaskTree
//
// Orchestrates sequential regime execution via Qt TaskTree (Qt 6.11 Tech Preview).
//
// Supported regime names (add to RunTable.qml "Добавить" menu as needed):
//   "Вакуум"           → VacuumRegimeWorker
//   "Режим в"          → RegimeBWorker
//   "Режим г"          → RegimeGWorker
//   "Тест клапанов"    → ValveTestWorker
//
// QML registration in Grams.cpp:
//   qmlRegisterSingletonInstance("Grams.regimeTaskTreeSingleton", 1, 0, "RegimeTaskTree", &m_regimeTaskTree);
//
// QML usage:
//   RegimeTaskTree.startAll()
//   RegimeTaskTree.pause()
//   RegimeTaskTree.resume()
//   RegimeTaskTree.stop()
//   if (RegimeTaskTree.running) { ... }
//   if (RegimeTaskTree.paused)  { ... }
//
// Valve list for "Тест клапанов":
//   Set via setValveNamesForTest(names) from Grams::initActionHandler()
//   after Initialize has been read.
// ─────────────────────────────────────────────────────────────────────────────

class RegimeTaskTree : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running  READ isRunning  NOTIFY runningChanged)
    Q_PROPERTY(bool paused   READ isPaused   NOTIFY pausedChanged)
    Q_PROPERTY(int  activeRegime READ activeRegimeId NOTIFY activeRegimeChanged)

public:
    explicit RegimeTaskTree(QObject* parent = nullptr);
    ~RegimeTaskTree() override;

    // ── Dependency injection ─────────────────────────────────────────────────
    void setRegimeManager  (RegimeManager*   manager);
    void setValveControl   (ValveControl*    valveControl);
    void setDataAcquisition(DataAcquisition* dataAcquisition);
    void setSecurity       (Security*        security);

    // ── Vacuum regime configuration ──────────────────────────────────────────
    // Датчик виртуального объёма B (prSB) для стража ДВ_СБР — вызывается из
    // Grams::initActionHandler().
    void setVacuumPressureSensor(DataCollection* sensor);

    // Флаги пропуска блока C и второго тракта (инверсия легаси flagIncludeRK*;
    // по умолчанию откачивается весь блок C, второй тракт выключен).
    Q_INVOKABLE void setVacuumOptions(bool skipRK10, bool skipRK50,
                                      bool skipRK300, bool secondTract);

    // ── Valve test configuration ─────────────────────────────────────────────
    // Called from Grams::initActionHandler() to seed available valve names.
    void setValveNamesForTest(const QStringList& names);

    // Called from QML "Применить" to push the step configuration.
    // steps: QVariantList of QVariantMap { valves:QStringList, pauseBefore:int,
    //                                      dwell:int, pauseAfter:int }
    Q_INVOKABLE void setValveTestSteps(const QVariantList& steps,
                                       int repeats,
                                       int globalPauseBefore,
                                       int globalPauseAfter);

    // Available valve names for the QML picker ComboBox.
    Q_PROPERTY(QStringList availableValves READ availableValves CONSTANT)
    QStringList availableValves() const { return m_valveNamesForTest; }

    // ── Properties ───────────────────────────────────────────────────────────
    bool isRunning()      const { return m_running; }
    bool isPaused()       const { return m_paused;  }
    int  activeRegimeId() const { return m_activeRegimeId; }

public slots:
    // ── QML-invokable control ─────────────────────────────────────────────────
    Q_INVOKABLE void startAll();             // run all Waiting regimes
    Q_INVOKABLE void startFrom(int id);      // run from a specific regime index
    Q_INVOKABLE void pause();                // pause the running regime
    Q_INVOKABLE void resume();               // resume after pause
    Q_INVOKABLE void stop();                 // cancel — sets Stopped state

signals:
    void runningChanged();
    void pausedChanged();
    void activeRegimeChanged();

    void regimeStarted (int regimeId, const QString& name);
    void regimeFinished(int regimeId, bool success);
    void executionFinished(bool allSuccess);

    // ── Forwarded to workers ─────────────────────────────────────────────────
    void pauseRequested();
    void resumeRequested();

private:
    // ── Recipe builders ──────────────────────────────────────────────────────
    QtTaskTree::Group buildSequence(int startFromId);
    QtTaskTree::Group buildRegimeGroup(int regimeId, const Regime& regime);

    // ── Helpers ──────────────────────────────────────────────────────────────
    RegimeWorkerConfig makeConfig(int regimeId, const Regime& regime) const;
    ValveTestConfig    makeValveTestConfig(int regimeId, const Regime& regime) const;

    void setRunning(bool running, int activeRegimeId = -1);
    void setPaused(bool paused);
    void cleanupTree();

    // ── State ────────────────────────────────────────────────────────────────
    RegimeManager*   m_manager         = nullptr;
    ValveControl*    m_valveControl    = nullptr;
    DataAcquisition* m_dataAcquisition = nullptr;
    Security*        m_security        = nullptr;

    VacuumOptions m_vacuumOptions;

    QStringList            m_valveNamesForTest;
    QList<ValveStepConfig> m_valveTestSteps;
    int                    m_valveTestRepeats           = 1;
    int                    m_valveTestGlobalPauseBefore = 0;
    int                    m_valveTestGlobalPauseAfter  = 0;

    QtTaskTree::QTaskTree* m_tree           = nullptr;
    RegimeLogger           m_logger;
    bool                   m_running        = false;
    bool                   m_paused         = false;
    int                    m_activeRegimeId = -1;
};
