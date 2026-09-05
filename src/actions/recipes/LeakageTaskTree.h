#pragma once

#include "RecipeCommon.h"

// ─────────────────────────────────────────────────────────────────────────────
// LeakageTaskTree — рецепт режима «Натекание» на Qt TaskTree.
//
// До этого натекание не было прогоном вообще: тумблер в LeakageAdjust.qml
// дёргал две несвязанные вещи — DataSource.setLeakageMeasure(checked) и
// ReactionQuar.startLeakageMeasure(checked). Ни у одной нет ни длительности,
// ни условия завершения, ни причины остановки, ни записи в regime_log.db.
//
// Рецепт добавляет ровно это: предусловие, отмеряемую длительность, различимую
// причину завершения и гарантированное закрытие дозирующего клапана на любом
// исходе.
//
// Физику считает GasLeakage (src/addon/) — она не переписывается. Рецепт
// вызывает startCalc / addMeasure / endCalc / saveResultsToFile через швы, а
// расчёт расхода и запись файлов остаются там же, где были.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Узлы развёртки ──────────────────────────────────────────────────────────
enum class LeakageNode {
    Gate,      // предусловие: накопитель заряжен, камера откачана
    Prepare,   // включение сбора данных, снимок начальной точки, startCalc
    Open,      // открытие дозирующего клапана
    Run,       // измерение: addMeasure раз в тик
    Close      // закрытие клапана, endCalc, сохранение
};

// ─── Причина завершения ──────────────────────────────────────────────────────
enum class LeakageStop {
    None,
    DurationElapsed,   // отработана заданная длительность
    TargetReached,     // достигнут целевой перепад давления
    SensorInvalid,     // показание стало недостоверным — измерять нечем
    Cancelled
};

QString leakageStopName(LeakageStop stop);

// ─── Состояние прогона ───────────────────────────────────────────────────────
struct LeakageRunState {
    bool        valveOpen   = false;
    bool        measureOn   = false;   // сбор данных включён
    bool        calcStarted = false;   // startCalc вызван, endCalc ещё нет
    int         elapsedSec  = 0;
    double      deltaBar    = 0.0;     // накопленный перепад по накопителю
    LeakageStop stop        = LeakageStop::None;
};

// ─── Контекст рецепта ────────────────────────────────────────────────────────
struct LeakageTreeContext {
    // ── Швы к железу ─────────────────────────────────────────────────────────
    std::function<bool(bool open, const QString& valve)> setValve;
    std::function<bool(bool expectedOpen, const QString& valve)> confirmValve; // nullable
    std::function<Reading()> pressureStorageBar;    // накопитель, бар
    std::function<Reading()> pressureReactionBar;   // реакционная область, бар
    std::function<double()>  temperatureReactionK;  // абсолютная, К

    // ── Швы к GasLeakage и сбору данных ──────────────────────────────────────
    // Порядок вызовов повторяет существующий: setMeasureMode(true) включает
    // фильтрацию давлений в DataAcquisition, дальше идёт расчёт.
    std::function<void(bool)> setMeasureMode;
    std::function<void(double sBar, double rBar, double tK)> leakageStart;
    std::function<void(double sBar, double rBar, double tK)> leakageAdd;
    std::function<void(bool)> leakageSetOpen;
    std::function<void()> leakageEnd;
    std::function<void()> leakageSave;

    // ── Наблюдение (nullable) ────────────────────────────────────────────────
    std::function<void(LeakageNode, NodeState)> onNode;
    std::function<void(const QString&)> onLabel;
    std::function<void(int elapsedSec, Reading storage, Reading reaction,
                       double deltaBar)> onProgress;
    std::function<void(const QString&)> onFailure;
    std::function<void(LeakageStop)> onFinished;

    // ── Параметры прогона ────────────────────────────────────────────────────
    QString valve;                  // дозирующий клапан, DO-имя
    int     durationSec    = 60;    // сколько держать открытым
    double  targetDeltaBar = 0.0;   // 0 = не использовать как условие остановки

    // ── Гейт цепочки (из профиля, не из UI) ──────────────────────────────────
    // Пустой накопитель — прогон без содержания; неоткачанная камера — Δp
    // считается от неизвестного начального уровня.
    bool   gateCheck        = true;
    double gateMinStorageBar  = 0.5;
    double gateMaxReactionBar = 0.05;

    int tickIntervalMs = 1000;      // 1 тик = 1 с в бою, меньше в тестах

    PauseBus* pauseBus = nullptr;
};

// ─── Сборка рецепта ──────────────────────────────────────────────────────────
QtTaskTree::Group buildLeakageRecipe(const LeakageTreeContext& ctx,
                                     const QtTaskTree::Storage<LeakageRunState>& st);
QtTaskTree::Group buildLeakageRecipe(const LeakageTreeContext& ctx);
