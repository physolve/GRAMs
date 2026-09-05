#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <qtasktree.h>

#include "../SensorQuality.h"

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
// Форвакуумная откачка 11.5–11.7 (DO-имена из src/Grams.cpp:60-120):
inline const QString K176 = QStringLiteral("AR6");  // форвакуумный насос
inline const QString K178 = QStringLiteral("AR5");  // магистраль (импульс 2 с)
inline const QString K151 = QStringLiteral("R3");   // линия E/F (макс. поток к камере)
// Тракт общей откачки 11.7б (REQ-055). DO-каналы уже существуют
// (src/Grams.h:153,160) — не хватало только символов здесь.
inline const QString K171 = QStringLiteral("S4");   // pressure range storage
inline const QString K173 = QStringLiteral("R4");   // pressure range reaction
}

// ─── Показание датчика ────────────────────────────────────────────────────────
//
// Значение в паскалях плюс признак качества (Quality — см. src/SensorQuality.h).

struct Reading {
    double  valuePa = 0.0;
    Quality quality = Quality::Valid;

    Reading() = default;
    // НЕ explicit намеренно: std::function<Reading()> принимает лямбду,
    // возвращающую double, поэтому существующие тесты вида
    //   ctx.pressureVacPa = [] { return 5.0; };
    // компилируются без правок.
    Reading(double pa) : valuePa(pa) {}
    Reading(double pa, Quality q) : valuePa(pa), quality(q) {}

    bool isValid()     const { return quality == Quality::Valid; }
    bool isOverRange() const { return quality == Quality::OverRange; }
};

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

// ─── OperatorBus ──────────────────────────────────────────────────────────────
//
// Блокирующее решение оператора (аналог PauseBus). Задача рецепта вызывает
// request(code,msg) → QML показывает диалог → оператор жмёт кнопку → respond().
// Используется watchdog'ом dP/dt: «откачка не идёт» → Стоп / Продолжить.

class OperatorBus : public QObject
{
    Q_OBJECT
public:
    enum Decision { None = 0, Continue = 1, Stop = 2 };
    Q_ENUM(Decision)

    using QObject::QObject;

    Decision decision() const { return m_decision; }

    // Из рецепта (главный поток): поднять запрос к оператору.
    void request(int code, const QString& message)
    {
        m_decision = None;
        m_activeCode = code;
        m_activeMessage = message;
        emit decisionRequired(code, message);
    }

    // Свойства для QML-диалога.
    Q_PROPERTY(bool active READ active NOTIFY decisionRequired)
    Q_PROPERTY(QString message READ message NOTIFY decisionRequired)
    bool    active()  const { return m_decision == None && !m_activeMessage.isEmpty(); }
    QString message() const { return m_activeMessage; }

    // Из QML: ответ оператора (1 = Продолжить, 2 = Стоп).
    Q_INVOKABLE void respond(int d)
    {
        m_decision = Decision(d);
        m_activeMessage.clear();
        emit decisionReceived(m_decision);
    }

signals:
    void decisionRequired(int code, QString message);
    void decisionReceived(OperatorBus::Decision d);

private:
    Decision m_decision     = None;
    int      m_activeCode    = 0;
    QString  m_activeMessage;
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

// ─── PumpRateWatchdog ─────────────────────────────────────────────────────────
//
// Проверка «откачка идёт» по dP/dt после открытия К176: за окно windowSec
// давление ДВ301 должно упасть не менее чем на minDropPa. Если нет —
// поднимается операторский диалог (OperatorBus): «Продолжить» повторяет окно,
// «Стоп» завершает задачу ошибкой (→ останов режима). QCustomTask-совместим
// (start() + done(bool)). Pause-aware. Без operatorBus «не идёт» → done(false).
//
// startDelaySec — «мёртвая зона» перед ПЕРВЫМ окном: сразу после открытия К176
// насос ещё выходит на режим, датчик ДВ301 стоит в over-range и dP/dt ложно
// мала. Отсчёт окна (и точка p0) берётся только после этой задержки. На повторы
// после «Продолжить» задержка не накладывается — насос уже работает, оператор
// ждёт немедленной перепроверки.

class PumpRateWatchdog : public QObject
{
    Q_OBJECT
public:
    explicit PumpRateWatchdog(QObject* parent = nullptr);

    std::function<Reading()> pressurePa;  // ДВ301 в Па с качеством (nullable)
    int    intervalMs   = 1000;
    int    startDelaySec = 30;            // пауза перед первым окном (выход насоса на режим)
    int    windowSec    = 5;
    double minDropPa    = 0.5;            // мин. падение давления за окно
    QPointer<PauseBus>    pauseBus;
    QPointer<OperatorBus> operatorBus;

    void start();                         // контракт QCustomTask

signals:
    void done(bool success);

private slots:
    void tick();
    void onDecision(int d);

private:
    void beginWindow();

    QTimer m_timer;
    int    m_elapsed         = 0;
    int    m_delayLeft       = 0;      // остаток «мёртвой зоны» текущего окна
    bool   m_firstWindow     = true;   // задержка накладывается только на первое окно
    double m_p0              = 0.0;
    bool   m_awaitingOperator = false;
};

// ─── Состояние одного повтора (инвариант 3: всё пер-ранное — только здесь) ────
//
// Storage<VacuumRunState> привязан к телу For(...)>>Do{...} — пересоздаётся
// на каждый повтор и каждый запуск, поэтому повторный старт не наследует
// состояния по построению.

struct VacuumRunState {
    bool k118Open = false;   // AR4 — сброс в атмосферу
    bool s3Open   = false;   // RK300 / C1
    bool s1Open   = false;   // RK10  / C3
    bool s2Open   = false;   // RK50  / C2
    bool sl2Open  = false;   // K192
    bool sl1Open  = false;   // K179
    bool k176Open = false;   // AR6 — форвакуумный насос
    bool k179Open = false;   // SL1 — турбомолекулярный насос (12.2)
    bool k178Open = false;   // AR5 — магистраль
    bool k151Open = false;   // R3  — линия E/F
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

// ─── Наблюдение за узлами рецепта (для live-развёртки в UI) ───────────────────
//
// Состояние узла развёртки и стабильные идентификаторы узлов. Порядок VacuumNode
// = порядок обхода дерева (используется монитором для построения строк модели).
// Инструментация в VacuumTaskTree.cpp — аддитивная (onGroupSetup/onGroupDone),
// control-flow не меняет; шов onNode nullable, тесты его не задают.

enum class NodeState { Initial, Running, Success, Error, Cancelled, Skipped };

enum class VacuumNode {
    Condition,       // condition-фаза повтора
    F1Relief,        // Ф1: сброс-триплет К118 (s1–s4)
    F2BlockC,        // Ф2: блок C целиком (s5–s19)
    F2_RK300,        //   подключение RK300 (s5–s10)
    F2_RK10,         //   подключение RK10  (s11–s12)
    F2_RK50,         //   подключение RK50  (s13–s14)
    F2_ReliefMid,    //   средний сброс-триплет (s15–s17)
    F3SecondTract,   // Ф3: второй тракт (s20–s24)
    F5A1,            // 11.5: форвакуум A1 (К118→К178-импульс→К176)
    F6BC,            // 11.6: форвакуум B/C (клапаны C→К178-импульс→К176)
    F7EF,            // 11.7: форвакуум E/F (К151→К178-импульс→К176)
    TurboGate,       // 12.2: гейт перехода (ДВ301 ≤ порог, удержание, ДВ302 валиден)
    TurboSwitch,     // 12.2: переключение К176 → К179 с подтверждением readback
    TurboPumping,    // 12.2: турбо-откачка, контроль по ДВ302
    TurboFallback,   // 12.2: откат на форвакуум по turboReturnPressurePa
    ContinuousPumping // финал: тракт оставлен открытым под откачку (опция)
};

// ─── Контекст рецепта ─────────────────────────────────────────────────────────

struct VacuumTreeContext {
    // Швы к железу (обязательные для боевого запуска; в тестах — моки)
    std::function<bool(bool open, const QString& valve)> setValve;
    std::function<double()> pressureB;      // виртуальный объём B (аналог m_vir_B)
    // ДВ301 в Паскалях (форвакуум 11.5–11.7). nullable: без датчика удержание
    // не набирается ⇒ этап падает по таймауту, клапаны закрываются.
    // Возвращает Reading; лямбда, возвращающая double, тоже подходит —
    // Reading конструируется из double неявно с Quality::Valid.
    std::function<Reading()> pressureVacPa;
    // ДВ302 в Паскалях (турбо-область, раздел 12.2). nullable: без датчика
    // условие У3 гейта не выполняется ⇒ перехода на турбонасос не будет.
    std::function<Reading()> pressureTurboPa;

    // Readback фактического состояния клапана (V-02, REQ-082/084). nullable.
    //
    // setValve сообщает результат КОМАНДЫ, а не ФАКТА: «клапан закрыт» и
    // «команда на закрытие принята» — разные утверждения, и REQ-082 требует
    // именно первого. Без этого шва турбо-переход не выполняется вовсе:
    // открыть К179 на непрощавшемся форвакуумном тракте — порча насоса.
    //
    // ОГРАНИЧЕНИЕ: на Advantech USB-47xx чтение DO возвращает регистр-защёлку
    // выхода, а не независимый датчик положения. Это ловит потерянную запись,
    // сброс платы, перечисление USB и чужую запись в порт, но НЕ доказывает,
    // что клапан физически переместился. Физическое подтверждение потребовало
    // бы концевиков на InstantDiCtrl — в проекте их нет. Дивергенция от REQ-082
    // осознанная, см. docs/regimes/vacuum.md.
    std::function<bool(bool expectedOpen, const QString& valve)> confirmValve;

    // Параметры последовательности
    double dbSbrLim       = 1.65;  // DB_SBR_LIM (GramQt Definer.h:334)
    // Выдержка «сброса» — К118 (s3/s16) и К179 второго тракта (s22): легаси
    // time_5000. Зафиксирована на 10 с по результатам стенда; из UI не
    // настраивается (см. docs/regimes/vacuum.md §5).
    int    reliefDwellSec = 10;
    int    tickIntervalMs = 1000;  // 1 тик = 1 с в бою; в тестах меньше
    int    totalRepeats   = 1;
    // Пауза-выдержка после каждого дискретного действия рецепта (settlePause):
    // сколько клапан стоит в новом положении до следующей команды. Единая для
    // всех шагов — 3 с (стенд); из UI не настраивается, в тестах 0.
    int    perActionPauseMs = 3000;

    // Флаги пропуска объёмов блока C (инверсия легаси flagIncludeRK*: по
    // умолчанию весь блок C откачивается — решение Q2).
    //
    // Соответствие имён (ТЗ v5 REQ-005 + метки интерфейса C1/C2/C3 в
    // src/qml/measure/ESupply.qml). Легаси-имена RK10/50/300 (из GramQt) —
    // это те же объёмы; на интерфейсе GRAMs они называются C1/C2/C3:
    //
    //   Флаг        Клапан  DO-канал  Интерфейс  Легаси (GramQt)
    //   skipRK300   К135    S3        C1         RK300
    //   skipRK50    К133    S2        C2         RK50
    //   skipRK10    К131    S1        C3         RK10
    //
    bool skipRK10  = false;        // C3 (К131/S1), s11 (21003)
    bool skipRK50  = false;        // C2 (К133/S2), s13 (21004)
    bool skipRK300 = false;        // C1 (К135/S3), s5  (21005)
    bool secondTract = false;      // s20 (21009), легаси flagIncludeSecTract

    // Форвакуумная откачка 11.5–11.7 (по умолчанию включена в бою; тесты Ф1–Ф3
    // выключают, чтобы изолировать эталон). Параметры-ориентиры из ТЗ REQ-022.
    //
    // ВАЖНО: targetVacPa — ЦЕЛЬ форвакуумных этапов, а НЕ гейт перехода на
    // турбонасос. Это разные величины (REQ-047/075 против REQ-078/080). Если
    // совместить их в одном параметре, оператор, подняв цель до 40 Па «чтобы
    // форвакуум отработал быстрее», одновременно разрешит запуск турбонасоса
    // при 40 Па — тихий отказ безопасности. Гейт живёт ниже, в блоке 12.2.
    bool   foreVacuum        = true;
    double targetVacPa       = 40.0;   // цель этапов 11.5–11.7 (REQ-047/075)
    int    foreVacHoldSec    = 60;     // непрерывное удержание ≤ targetVacPa
    int    k178PulseMs       = 2000;   // импульс К178 (REQ-046)
    int    foreVacTimeoutSec = 300;    // таймаут форвакуумного этапа

    // ── Турбомолекулярный насос, раздел 12.2 (REQ-077…084) ────────────────────
    //
    // Топология: магистраль → AR6/К176 (форвакуум) и SL1/К179 (турбо) → ДВ302 →
    // SL2/К192 (выход второго тракта; атмосфера, если камеры нет). К176 и К179
    // НИКОГДА не открыты одновременно (REQ-008, REQ-084).
    //
    // Пороги приходят из JSON (profile/GRAMsPfp.json → vacuumSafety), а не из
    // UI: это гейты безопасности, их нельзя менять с рабочего экрана.
    bool   turboTract            = true;   // Advanced: исключить тракт турбонасоса (разд. 9.1)
    double turboSwitchPressurePa = 10.0;   // гейт перехода по ДВ301 (REQ-078/080)
    int    turboSwitchHoldSec    = 60;     // непрерывное удержание гейта (REQ-080)
    double turboReturnPressurePa = 30.0;   // порог отката на форвакуум (REQ-083)
    int    turboTimeoutSec       = 600;    // лимит набора гейта целиком
    // Гистерезис 10 Па на вход / 30 Па на выход — намеренный: при равных
    // порогах система у границы начала бы циклически переключать К176 и К179,
    // что опаснее любого из двух устойчивых состояний.
    //
    // Over range — не отказ датчика, а нормальная фаза процесса (REQ-079/081):
    // выдержка → повторная проверка → предупреждение и безопасное действие →
    // повторная выдержка → критическая ошибка либо ожидание оператора.
    // Безопасное действие РАЗНОЕ: для ДВ301 — закрыть К176, для ДВ302 —
    // не открывать / закрыть К179. Смешивать их нельзя.
    int    overrangeWaitSec  = 300;    // ожидание over range ДВ301 (REQ-079)
    int    overrangeWaitSec2 = 350;    // ожидание over range ДВ302 (REQ-081)

    // dP/dt-watchdog после открытия К176 (REQ-020/076): проверка, что откачка
    // идёт. Диалог оператора — через OperatorBus.
    bool   pumpRateCheck     = true;
    int    pumpCheckDelaySec  = 30;    // «мёртвая зона» перед первым окном (выход насоса на режим)
    int    pumpCheckWindowSec = 5;     // окно измерения dP/dt
    double pumpMinDropPa      = 0.5;   // мин. падение давления за окно
    OperatorBus* operatorBus = nullptr;

    // Непрерывная откачка: после успешного завершения ВСЕХ повторов оставить
    // тракт открытым (C по skipRK* → К151 → К178 → К176), чтобы насос продолжал
    // качать без автоматического закрытия. К118 (атмосфера) и К179 (турбо,
    // интерлок с К176) не открываются никогда. При отмене/ошибке этап
    // пропускается — рецепт закрывает всё, как раньше.
    bool   continuousPumping = false;

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
    // Наблюдатель состояния узлов развёртки (nullable). Вызывается из
    // onGroupSetup (Running/Skipped) и onGroupDone (Success/Error/Cancelled).
    std::function<void(VacuumNode, NodeState)>      onNode;

    // Live-прогресс форвакуумного удержания 11.5–11.7 (nullable). Тикает раз в
    // ctx.tickIntervalMs, пока идёт pausableHoldUntil внутри foreVacPumpToTarget:
    //   node      — какой из узлов F5A1/F6BC/F7EF сейчас качает
    //   currentPa — показание ДВ301 (Па); NaN, если датчик не задан
    //   heldSec   — набранное непрерывное удержание ≤ targetVacPa
    //   elapsedSec— время с начала этапа (против foreVacTimeoutSec)
    std::function<void(VacuumNode node, double currentPa,
                       int heldSec, int elapsedSec)> onForevacProgress;
    // Этап завершился (успех или нет) — снять live-индикацию.
    std::function<void(VacuumNode node, bool success)> onForevacDone;

    // Live-прогресс турбо-этапа 12.2 (nullable). Тикает раз в ctx.tickIntervalMs,
    // пока набирается гейт либо идёт откачка через К179:
    //   node    — TurboGate / TurboPumping
    //   p301    — ДВ301 с качеством, p302 — ДВ302 с качеством
    //   heldSec — набранное непрерывное удержание гейта (0 на TurboPumping)
    std::function<void(VacuumNode node, Reading p301, Reading p302,
                       int heldSec, int elapsedSec)> onTurboProgress;
    // Факт переключения насосного клапана: true = перешли на турбо (К179),
    // false = откат на форвакуум (К176). Обе стороны обязаны попасть в журнал
    // (REQ-082/083), поэтому шов вызывается и при откате.
    std::function<void(bool toTurbo)> onTurboSwitched;
    // Человекочитаемая причина отказа этапа — «причина завершения режима» в UI.
    // Вызывается в точке отказа; первый вызов за прогон считается основным.
    std::function<void(const QString& reason)>       onFailure;

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
