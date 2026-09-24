#pragma once

#include <QObject>
#include <QStringList>
#include <qtasktree.h>

#include "../runtable/regimemanager.h"
#include "../ValveControl.h"
#include "../DataAcquisition.h"
#include "../Security.h"
#include "workers/RegimeWorkers.h"
#include "workers/ValveTestWorker.h"
#include "workers/ChainWorkers.h"
#include "RegimeLogger.h"
#include "workers/VacuumRunMonitor.h"

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

    // Датчик ДВ301 (Вакууметр) для форвакуумных этапов 11.5–11.7. Вызывается из
    // Grams::initActionHandler().
    void setVacuumGaugeSensor(DataCollection* sensor);
    // ДВ302 (второй тракт, турбо). Без него условие У3 гейта не выполняется
    // и режим остаётся на форвакууме — безопасный исход.
    void setVacuumTurboGaugeSensor(DataCollection* sensor);

    // Включение/выключение форвакуумной откачки 11.5–11.7 (по умолчанию вкл).

    // Флаги пропуска блока C и второго тракта (инверсия легаси flagIncludeRK*;
    // по умолчанию откачивается весь блок C, второй тракт выключен).
    Q_INVOKABLE void setVacuumOptions(bool skipRK10, bool skipRK50,
                                      bool skipRK300, bool secondTract);

    // Оставить тракт открытым после успешного завершения всех повторов
    // («непрерывная откачка»): клапаны C/К151/К178/К176 не закрываются, насос
    // продолжает качать. Длительности шагов (3 с) и сброса К118 (10 с)
    // зафиксированы в коде — настройки из UI для них нет.
    Q_INVOKABLE void setVacuumContinuousPumping(bool enabled);

    // Пороги безопасности турбо-этапа из profile/GRAMsPfp.json (vacuumSafety).
    // Вызывается из Grams::initActionHandler(), НЕ из QML: гейт перехода на
    // турбонасос не настраивается с рабочего экрана (см. ТЗ REQ-022).
    void setVacuumSafety(double turboSwitchPressurePa, int turboSwitchHoldSec,
                         double turboReturnPressurePa, int turboTimeoutSec,
                         int overrangeWaitSec, int overrangeWaitSec2);

    // Наборы клапанов и пороги этапов 11.7б/11.8/11.9-11.11 из профиля
    // (vacuumTract, vacuumSafety). Как и setVacuumSafety, вызывается из
    // Grams::initActionHandler(), а НЕ из QML: REQ-055 требует хранить точный
    // набор клапанов в конфигурации, и менять его с рабочего экрана нельзя.
    void setVacuumTract(const QStringList& generalPumping,
                        const QStringList& leakTest,
                        const QStringList& finalPumping,
                        int testEvacTimeSec, int leakTestDurationSec,
                        const QMap<QString, double>& dPLeakMax);
    // Датчик канала герметичности: имя из dP_leak_max -> источник показаний.
    void setVacuumLeakSensor(const QString& name, DataCollection* sensor);

    // Цель и длительность финальной откачки камеры 11.10 (targetVAC, EvacTime).
    Q_INVOKABLE void setVacuumFinalTarget(double targetVacuumPa, int evacTimeSec);

    // Advanced-опция «исключить тракт турбомолекулярного насоса» (разд. 9.1).
    //
    // В интерфейсе выбора БОЛЬШЕ НЕТ: турбо-этап 12.2 — обязательная часть
    // режима «Вакуум», и оператор его не отключает. Метод оставлен как
    // сервисный шов (отладка стенда без турбонасоса, тесты); по умолчанию
    // turboTract = true, и рецепт пропускает этап только сам — когда нет ДВ302
    // или выключен форвакуум.
    Q_INVOKABLE void setVacuumTurboTract(bool enabled);

    // Включение dP/dt-watchdog после открытия К176.
    Q_INVOKABLE void setVacuumPumpCheck(bool enabled);

    // Цель форвакуумных этапов 11.5–11.7: до какого давления качать (ДВ301, Па),
    // сколько секунд его надо удержать непрерывно и лимит этапа целиком.
    // Дефолты ТЗ REQ-022: 10 Па / 60 с / 300 с.
    Q_INVOKABLE void setVacuumForevacTarget(double targetPa, int holdSec, int timeoutSec);

    // ── Цепочка «Вакуум → Напуск → Натекание» ────────────────────────────────
    //
    // Параметры прогона приходят из UI, пороги гейтов — из профиля. Гейт здесь
    // защищает достоверность эксперимента, а не оборудование, но принцип тот
    // же: величина зависит от стенда, значит живёт в конфигурации.
    Q_INVOKABLE void setSupplyParams(const QString& port, int openTimeMs,
                                     double pressureLimitBar);
    Q_INVOKABLE void setLeakageParams(const QString& valve, int durationSec,
                                      double targetDeltaBar);
    void setChainGates(double supplyMaxStartBar, double leakageMinStorageBar,
                       double leakageMaxReactionBar);
    // Указатели на подсистемы — из Grams::initActionHandler().
    void setSupplySources(DataCollection* storageSensor,
                          AddRemoveQuartile* addRemoveQuartile);
    void setLeakageSources(DataCollection* storageSensor,
                           DataCollection* reactionSensor,
                           DataCollection* temperatureSensor,
                           ReactionQuartile* reactionQuartile);

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
    // Путь журнала прогонов. По умолчанию data/regime_log.db; демо-режим
    // переносит журнал в отдельную базу, чтобы не смешивать с экспериментом.
    bool setLogDatabasePath(const QString& path) { return m_logger.reopen(path); }

    // Наблюдатель состояния рецепта «Вакуум» для live-развёртки в QML
    // (RegimeApiSandbox). Позже переедет в RegimeManager.
    Q_PROPERTY(QObject* vacuumMonitor READ vacuumMonitor CONSTANT)
    QObject* vacuumMonitor() { return &m_vacuumMonitor; }

    // Стабильная шина решений оператора (диалог dP/dt Стоп/Продолжить в QML).
    Q_PROPERTY(QObject* operatorBus READ operatorBus CONSTANT)
    QObject* operatorBus() { return &m_operatorBus; }

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
    // Опция «непрерывная откачка» изменена во время прогона: воркер «Вакуума»
    // подхватывает её на ходу, хвост рецепта читает значение в момент запуска.
    void vacuumContinuousPumpingChanged(bool enabled);

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

    VacuumOptions  m_vacuumOptions;
    SupplyOptions  m_supplyOptions;
    LeakageOptions m_leakageOptions;

    QStringList            m_valveNamesForTest;
    QList<ValveStepConfig> m_valveTestSteps;
    int                    m_valveTestRepeats           = 1;
    int                    m_valveTestGlobalPauseBefore = 0;
    int                    m_valveTestGlobalPauseAfter  = 0;

    QtTaskTree::QTaskTree* m_tree           = nullptr;
    RegimeLogger           m_logger;
    VacuumRunMonitor       m_vacuumMonitor;
    OperatorBus            m_operatorBus;
    bool                   m_running        = false;
    bool                   m_paused         = false;
    int                    m_activeRegimeId = -1;
};
