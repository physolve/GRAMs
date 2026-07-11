#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <qtasktree.h>

#include <functional>

// ─────────────────────────────────────────────────────────────────────────────
// VacuumTaskTree — рецепт режима «Вакуум» на Qt TaskTree (Ф1–Ф3, шаги s1–s24).
//
// Перенос из GramQt (SequencerThread + testSeq/vacuum_cond.csv, 56 шагов).
// Файл намеренно не зависит от Grams/ValveControl/DataAcquisition:
// всё железо и UI проброшены через std::function-швы в VacuumTreeContext,
// поэтому рецепт тестируется без biodaq (см. src/actions/tests/).
//
// Соответствие легаси-клапанов именам DO-каналов GRAMs (src/Grams.h:145-161):
//   К118 → "AR4"  (сброс в атмосферу)
//   К135 → "S3"   (RK300 — большой баллон)
//   К131 → "S1"   (RK10  — малый баллон)
//   К133 → "S2"   (RK50  — средний баллон)
//   К192 → "SL2"  (второй тракт: бочка)
//   К179 → "SL1"  (второй тракт: выход)
//
// Ф4–Ф8 (s25+, откачка К178 / пульсации К176 / блок C проход 2 / финал) —
// заглушки с объявленными сигнатурами, реализация отложена.
// ─────────────────────────────────────────────────────────────────────────────

namespace VacuumValve {
inline const QString K118 = QStringLiteral("AR4");
inline const QString K135 = QStringLiteral("S3");
inline const QString K131 = QStringLiteral("S1");
inline const QString K133 = QStringLiteral("S2");
inline const QString K192 = QStringLiteral("SL2");
inline const QString K179 = QStringLiteral("SL1");
}

// ─── PauseBus ─────────────────────────────────────────────────────────────────
//
// Ретранслятор pause/resume от воркера внутрь дерева. В QtTaskTree нет паузы,
// поэтому её реализуют сами задачи (PausableTicker): по paused() таймер
// останавливается с сохранением остатка, по resumed() дотикивает.

class PauseBus : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    bool isPaused() const { return m_paused; }

    void pause()
    {
        if (m_paused) return;
        m_paused = true;
        emit paused();
    }
    void resume()
    {
        if (!m_paused) return;
        m_paused = false;
        emit resumed();
    }

signals:
    void paused();
    void resumed();

private:
    bool m_paused = false;
};

// ─── PausableTicker ───────────────────────────────────────────────────────────
//
// QCustomTask<PausableTicker>: посекундный отсчёт с поддержкой паузы.
// Используется и для выдержек time_5000, и для condition-фазы.
// isComplete(elapsedTicks) опрашивается после каждого тика; отсутствие
// предиката = завершение после первого тика.
//
// ВАЖНО: onTick/isComplete вызываются из QTimer вне активного контекста
// дерева — обращаться к Tasking::Storage из них НЕЛЬЗЯ (только из
// setup/done-хендлеров задач и групп).

class PausableTicker : public QObject
{
    Q_OBJECT
public:
    explicit PausableTicker(QObject* parent = nullptr);

    int intervalMs = 1000;
    std::function<bool(int)> isComplete;   // elapsed ticks → true = готово
    std::function<void(int)> onTick;       // elapsed ticks (после инкремента)
    QPointer<PauseBus> pauseBus;

    int  elapsed() const { return m_elapsed; }
    void start();                          // контракт QCustomTask

signals:
    void done(bool success);

private slots:
    void tick();

private:
    QTimer m_timer;
    int    m_elapsed = 0;
};

// ─── Состояние одного повтора (инвариант 3: всё пер-ранное — только здесь) ────
//
// Storage<VacuumRunState> привязан к телу For(...)>>Do{...} — пересоздаётся
// на каждый повтор и каждый запуск, поэтому повторный старт не наследует
// состояния по построению.

struct VacuumRunState {
    bool k118Open = false;   // AR4 — сброс в атмосферу
    bool s3Open   = false;   // RK300
    bool s1Open   = false;   // RK10
    bool s2Open   = false;   // RK50
    bool sl2Open  = false;   // K192
    bool sl1Open  = false;   // K179
    int  elapsedSec  = 0;    // суммарное время выдержек execution-фазы повтора
    int  reliefCount = 0;    // сколько сбросов через К118 реально сработало
};

// Итог всего запуска (все повторы). Живёт в корне рецепта.
struct VacuumRunSummary {
    int repeatsDone  = 0;
    int repeatsError = 0;
};

// ─── Параметризация блока C (инвариант 4) ─────────────────────────────────────
//
// Ф2 (проход 1, s5–s19):  reliefAndClose = true  — после каждого подключения
//                          сброс-триплет К118, в конце баллоны закрываются.
// Ф6 (проход 2, строки 39–44): reliefAndClose = false — только подключения,
//                          баллоны остаются открытыми под откачку.
// Слепое слияние Ф2/Ф6 запрещено — различия критичны для безопасности.

struct BlockCOptions {
    bool reliefAndClose = true;
};

// ─── Контекст рецепта ─────────────────────────────────────────────────────────

struct VacuumTreeContext {
    // Швы к железу (обязательные для боевого запуска; в тестах — моки)
    std::function<bool(bool open, const QString& valve)> setValve;
    std::function<double()> pressureB;      // виртуальный объём B (аналог m_vir_B)

    // Параметры последовательности
    double dbSbrLim       = 1.65;  // DB_SBR_LIM (GramQt Definer.h:334)
    int    reliefDwellSec = 5;     // time_5000 у шагов 11118 / 11179
    int    tickIntervalMs = 1000;  // 1 тик = 1 с в бою; в тестах меньше
    int    totalRepeats   = 1;

    // Флаги пропуска (инверсия легаси flagIncludeRK*: по умолчанию
    // весь блок C откачивается — решение Q2)
    bool skipRK10  = false;        // s11 (21003)
    bool skipRK50  = false;        // s13 (21004)
    bool skipRK300 = false;        // s5  (21005)
    bool secondTract = false;      // s20 (21009), легаси flagIncludeSecTract

    // Condition-фаза (повторяет семантику RegimeWorkerBase)
    QString conditionType       = QStringLiteral("none"); // "none"|"time"|"temp"
    int     conditionTimeSec    = 0;
    double  conditionTargetTemp = 0.0;
    std::function<double()> conditionTemp;  // датчик для "temp" (nullable)

    // Обратные вызовы в воркер / RegimeManager (все nullable)
    std::function<void(int elapsedSec, int repeat)> onConditionProgress;
    std::function<void(int repeat)>                 onConditionDone;
    std::function<void(int elapsedSec, int repeat)> onProgress;
    std::function<void(QtTaskTree::DoneWith, int repeat)> onRepeatDone;
    std::function<void(QtTaskTree::DoneWith, int repeatsDone, int repeatsError)> onRunFinished;
    std::function<void(const QString&)>             onLabel;

    PauseBus* pauseBus = nullptr;
};

// ─── Сборка рецепта ───────────────────────────────────────────────────────────
//
// Перегрузка со storages нужна тестам (onStorageSetup-ассерты критерия C)
// и воркеру (onStorageDone для итогов). Однопараметровая — удобство.

QtTaskTree::Group buildVacuumRecipe(const VacuumTreeContext& ctx,
                                    const QtTaskTree::Storage<VacuumRunState>& st,
                                    const QtTaskTree::Storage<VacuumRunSummary>& summary);
QtTaskTree::Group buildVacuumRecipe(const VacuumTreeContext& ctx);
