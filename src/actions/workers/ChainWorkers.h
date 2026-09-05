#pragma once

#include "RegimeWorkers.h"                    // RegimeWorkerConfig
#include "../recipes/SupplyTaskTree.h"
#include "../recipes/LeakageTaskTree.h"
#include "../../addon/AddRemoveQuartile.h"
#include "../../addon/ReactionQuartile.h"

// ─────────────────────────────────────────────────────────────────────────────
// ChainWorkers — воркеры цепочки «Вакуум → Напуск → Натекание».
//
// Оба повторяют standalone-паттерн VacuumRegimeWorker: контракт QCustomTask
// (start() + done(bool)), внутреннее QTaskTree по рецепту, вся пер-ранная
// информация — в Tasking::Storage внутри рецепта, у воркера только
// конфигурация и инфраструктура.
//
// Задача воркера ровно одна: заполнить швы контекста реальными указателями.
// Логика последовательности живёт в рецепте (recipes/), железо — здесь.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Параметры прогонов ──────────────────────────────────────────────────────
//
// Разделение то же, что у вакуума: параметр прогона приходит из UI, порог
// безопасности цепочки — из профиля. Гейт «тракт откачан» защищает не
// оборудование, а достоверность эксперимента, но принцип сохранён: величина
// зависит от стенда, значит живёт в конфигурации.

struct SupplyOptions {
    QString port;                     // DO-имя клапана порта (InletStrategy::usePort)
    int     openTimeMs       = 10000; // InletStrategy::openTime
    double  pressureLimitBar = 10.0;  // InletStrategy::pressureLimit
    bool    gateCheck        = true;
    double  gateMaxStartBar  = 0.05;

    DataCollection*    storageSensor     = nullptr;  // давление накопителя, бар
    AddRemoveQuartile* addRemoveQuartile = nullptr;
};

struct LeakageOptions {
    QString valve;                    // дозирующий клапан
    int     durationSec      = 60;
    double  targetDeltaBar   = 0.0;   // 0 = не использовать как условие
    bool    gateCheck        = true;
    double  gateMinStorageBar  = 0.5;
    double  gateMaxReactionBar = 0.05;

    DataCollection*  storageSensor     = nullptr;
    DataCollection*  reactionSensor    = nullptr;
    DataCollection*  temperatureSensor = nullptr;   // К (абсолютная)
    ReactionQuartile* reactionQuartile = nullptr;
};

// ─── Воркер «Напуск» ─────────────────────────────────────────────────────────
class SupplyRegimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit SupplyRegimeWorker(QObject* parent = nullptr);

    void setConfig(const RegimeWorkerConfig& cfg);
    void setOptions(const SupplyOptions& opts);

    void start();                    // контракт QCustomTask

signals:
    void done(bool success);

public slots:
    void onPauseRequested();
    void onResumeRequested();
    void cancelTree();               // см. комментарий у VacuumRegimeWorker::cancelTree

private:
    SupplyTreeContext makeContext();

    RegimeWorkerConfig m_cfg;
    SupplyOptions      m_opts;
    PauseBus           m_pauseBus;
    QtTaskTree::QTaskTree* m_tree  = nullptr;
    qint64                 m_runId = -1;
};

// ─── Воркер «Натекание» ──────────────────────────────────────────────────────
class LeakageRegimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit LeakageRegimeWorker(QObject* parent = nullptr);

    void setConfig(const RegimeWorkerConfig& cfg);
    void setOptions(const LeakageOptions& opts);

    void start();

signals:
    void done(bool success);

public slots:
    void onPauseRequested();
    void onResumeRequested();
    void cancelTree();

private:
    LeakageTreeContext makeContext();

    RegimeWorkerConfig m_cfg;
    LeakageOptions     m_opts;
    PauseBus           m_pauseBus;
    QtTaskTree::QTaskTree* m_tree  = nullptr;
    qint64                 m_runId = -1;
};
