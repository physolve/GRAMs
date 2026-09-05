#pragma once

#include "RecipeCommon.h"

// ─────────────────────────────────────────────────────────────────────────────
// SupplyTaskTree — рецепт режима «Напуск» на Qt TaskTree.
//
// Перенос src/actions/legacy/InletAction.cpp шаг в шаг. Алгоритм сохранён
// дословно; изменилось то, КАК он исполняется:
//
//   было                              стало
//   QtConcurrent::run + QPromise      узлы TaskTree
//   QThread::msleep(100) в цикле      PausableTicker (пауза реально работает)
//   promise.isCanceled() раз в 100 мс onGroupDone закрывает клапан на любом исходе
//   сырые указатели на железо         std::function-швы (тестируется без biodaq)
//
// Почему не обёртка над InletAction: спящий поток нельзя поставить на паузу,
// точка отмены не определена (клапан мог остаться открытым в произвольный
// момент), а сырые указатели на ValveControl/DataAcquisition тянут biodaq в
// тесты. Подробности — в плане цепочки, раздел 3.2.
//
// Физику напуска считает SupplyPort (src/addon/), она не переписывается:
// рецепт только управляет клапаном и временем, а расчёт скорости и запись
// файлов остаются за AddRemoveQuartile/SupplyPort через швы.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Узлы развёртки ──────────────────────────────────────────────────────────
// Порядок = порядок обхода дерева (монитор строит по нему строки модели).
enum class SupplyNode {
    Gate,      // предусловие: тракт откачан, клапан порта закрыт
    Prepare,   // beginAction, снимок нулевой точки, прогрев быстрого буфера
    Open,      // открытие клапана порта
    Run,       // накопление: тик, чтение буфера, запись точек
    Close      // закрытие клапана и сохранение результатов
};

// ─── Причина штатного завершения набора ──────────────────────────────────────
// Различать их обязательно: «набрали давление» и «вышло время» — разные
// результаты эксперимента, и оператор должен видеть, какой именно получен.
enum class SupplyStop {
    None,
    TimeElapsed,      // истекло openTimeMs
    PressureReached,  // достигнут pressureLimitBar
    QuartileVeto,     // checkSupplyAction() сказал «дальше нельзя»
    Cancelled
};

QString supplyStopName(SupplyStop stop);

// ─── Состояние прогона ───────────────────────────────────────────────────────
// Живёт в Tasking::Storage: пересоздаётся на каждый запуск, поэтому повторный
// старт не наследует состояния по построению.
struct SupplyRunState {
    bool       portOpen   = false;
    bool       actionOpen = false;   // beginAction вызван, endAction ещё нет
    int        elapsedMs  = 0;
    SupplyStop stop       = SupplyStop::None;
};

// ─── Контекст рецепта ────────────────────────────────────────────────────────
struct SupplyTreeContext {
    // ── Швы к железу (в тестах — моки) ───────────────────────────────────────
    std::function<bool(bool open, const QString& valve)> setValve;
    // Подтверждение факта состояния клапана (V-02). nullable: без него гейт
    // проверяет только команду, как было в легаси.
    std::function<bool(bool expectedOpen, const QString& valve)> confirmValve;
    // Давление накопителя в барах — единица InletStrategy::m_pressureLimit.
    std::function<Reading()> pressureStorageBar;

    // ── Швы к подсистемам сбора данных ───────────────────────────────────────
    // beginAction/endAction у ValveControl и DataAcquisition вызываются парой,
    // поэтому один шов с флагом, а не два: рассинхронизировать их нечем.
    std::function<void(bool active)> setActionMode;
    std::function<void()> fastBufferRead;      // DataAcquisition::fastBufferRead
    std::function<void()> runSupplyAction;     // DataAcquisition::runSupplyAction
    std::function<void(int elapsedMs)> fillSupplyData;    // нулевая точка
    std::function<bool(int elapsedMs)> appendSupplyData;  // очередная точка
    std::function<void()> saveSupplyData;
    std::function<bool()> checkSupplyAction;   // условие продолжения из quartile
    // Оператор трогал клапаны вручную во время прогона: легаси перезапрашивал
    // beginAction и продолжал. Поведение сохранено.
    std::function<bool()> actionInterrupted;

    // ── Наблюдение (nullable) ────────────────────────────────────────────────
    std::function<void(SupplyNode, NodeState)> onNode;
    std::function<void(const QString&)> onLabel;
    std::function<void(int elapsedMs, Reading pressure)> onProgress;
    std::function<void(const QString&)> onFailure;
    std::function<void(SupplyStop)> onFinished;

    // ── Параметры прогона (из InletStrategy) ─────────────────────────────────
    QString port;                      // DO-имя клапана порта, «usePort»
    int     openTimeMs       = 10000;  // InletStrategy::m_openTime
    double  pressureLimitBar = 10.0;   // InletStrategy::m_pressureLimit

    // ── Гейт цепочки (из профиля, не из UI) ──────────────────────────────────
    // Напуск на неоткачанный тракт смешивает остаточный газ с напускаемым, и
    // состав смеси после этого неизвестен — прогон испорчен молча.
    bool   gateCheck        = true;
    double gateMaxStartBar  = 0.05;    // выше — считаем, что тракт не откачан

    // ── Тайминги ─────────────────────────────────────────────────────────────
    // 1000 мс и 100 мс — из легаси (InletAction.cpp): пауза перед первым
    // чтением буфера и шаг цикла накопления.
    int tickIntervalMs  = 100;
    int preReadDelayMs  = 1000;
    int postReadDelayMs = 100;

    PauseBus* pauseBus = nullptr;
};

// ─── Сборка рецепта ──────────────────────────────────────────────────────────
QtTaskTree::Group buildSupplyRecipe(const SupplyTreeContext& ctx,
                                    const QtTaskTree::Storage<SupplyRunState>& st);
QtTaskTree::Group buildSupplyRecipe(const SupplyTreeContext& ctx);
