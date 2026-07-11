#pragma once

#include <QObject>
#include <QTimer>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "RegimeLogger.h"
#include "VacuumTaskTree.h"

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
//
// Настройки режима «Вакуум», задаваемые снаружи (RegimeTaskTree::setVacuumOptions
// / setVacuumPressureSensor). Флаги — инверсия легаси flagIncludeRK*: по
// умолчанию весь блок C откачивается, второй тракт выключен.

struct VacuumOptions {
    bool   skipRK10    = false;
    bool   skipRK50    = false;
    bool   skipRK300   = false;
    bool   secondTract = false;
    double dbSbrLim    = 1.65;   // порог сброса объёма B (GramQt Definer.h:334)
    DataCollection* pressureSensorB = nullptr;  // виртуальный объём B (prSB)
};

// Standalone-воркер (контракт QCustomTask, как ValveTestWorker): start() строит
// внутренний QTaskTree по рецепту buildVacuumRecipe (VacuumTaskTree.h) и
// эмитит done(bool) по его завершении. Всё пер-ранное состояние живёт в
// Tasking::Storage внутри рецепта — у воркера только конфигурация и
// инфраструктура (дерево, шина паузы, хендл записи лога).
//
// Отмена: внешний done-хендлер (RegimeTaskTree::buildRegimeGroup) обязан при
// DoneWith::Cancel синхронно вызвать cancelTree() — иначе deleteLater отложит
// отмену внутреннего дерева и cleanup-закрытия клапанов. Деструктор дублирует
// отмену как страховку.

class VacuumRegimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit VacuumRegimeWorker(QObject* parent = nullptr);
    ~VacuumRegimeWorker() override;

    void setConfig(const RegimeWorkerConfig& cfg);
    void setVacuumOptions(const VacuumOptions& opts);

    void start();        // контракт QCustomTask
    void cancelTree();   // синхронная отмена внутреннего дерева

signals:
    void done(bool success);

public slots:
    void onPauseRequested();
    void onResumeRequested();

private:
    VacuumTreeContext makeContext();

    RegimeWorkerConfig m_cfg;
    VacuumOptions      m_opts;
    PauseBus           m_pauseBus;
    QtTaskTree::QTaskTree* m_tree  = nullptr;  // внутреннее дерево (child)
    qint64                 m_runId = -1;       // хендл записи в regime_log.db
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
