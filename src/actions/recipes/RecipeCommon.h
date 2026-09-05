#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <qtasktree.h>

#include <functional>

#include "../../SensorQuality.h"

// ─────────────────────────────────────────────────────────────────────────────
// RecipeCommon — инфраструктура, общая для всех рецептов режимов на Qt TaskTree.
//
// Здесь нет ничего, что зависит от конкретного режима или от железа: только
// шины паузы и решений оператора, посекундный прерываемый отсчёт и тип
// показания датчика. Файл обязан собираться с Qt::Core + Qt::TaskTree и
// больше ни с чем — это то же правило, что для всей папки recipes/
// (см. src/actions/README.md), и оно проверяется сборкой VacuumTreeTests.
//
// Вынесено из VacuumTaskTree.h: рецепты напуска и натекания нуждаются в той же
// инфраструктуре, и включать ради неё вакуумный заголовок целиком неправильно.
// ─────────────────────────────────────────────────────────────────────────────

// ─── Состояние узла развёртки ────────────────────────────────────────────────
//
// Общее для всех рецептов: UI строит одинаковые строки развёртки независимо от
// того, какой режим исполняется.
enum class NodeState { Initial, Running, Success, Error, Cancelled, Skipped };

// Исход группы TaskTree → состояние узла развёртки. Общее для всех рецептов:
// раньше эта функция дублировалась бы в каждом.
inline NodeState doneToNode(QtTaskTree::DoneWith w)
{
    switch (w) {
    case QtTaskTree::DoneWith::Success: return NodeState::Success;
    case QtTaskTree::DoneWith::Cancel:  return NodeState::Cancelled;
    default:                            return NodeState::Error;
    }
}

// ─── Показание датчика ────────────────────────────────────────────────────────
//
// Значение в паскалях плюс признак качества (Quality — см. src/SensorQuality.h).

struct Reading {
    // Единицу задаёт шов, а не тип: вакуумные швы отдают паскали, напуск —
    // бары. Поэтому поле называется просто value — имя вроде valuePa лгало бы
    // в половине мест использования.
    double  value = 0.0;
    Quality quality = Quality::Valid;

    Reading() = default;
    // НЕ explicit намеренно: std::function<Reading()> принимает лямбду,
    // возвращающую double, поэтому существующие тесты вида
    //   ctx.pressureVacPa = [] { return 5.0; };
    // компилируются без правок.
    Reading(double v) : value(v) {}
    Reading(double v, Quality q) : value(v), quality(q) {}

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
//
// Код события говорит QML, ЧТО именно случилось, чтобы диалог мог показать
// осмысленный заголовок и подходящий набор кнопок. ТЗ требует решения
// оператора минимум в четырёх точках, и «Стоп/Продолжить» на все случаи
// не годится: при over range ДВ302 продолжать «как есть» нельзя — К179
// открывать запрещено.

class OperatorBus : public QObject
{
    Q_OBJECT
public:
    enum Decision { None = 0, Continue = 1, Stop = 2, Retry = 3, Abort = 4 };
    Q_ENUM(Decision)

    // Единый реестр кодов событий для ВСЕХ рецептов, а не только вакуумного.
    // Держать его в одном месте важно: QML сопоставляет код с заголовком и
    // набором кнопок диалога, и два режима не должны занять одно значение.
    // Новый режим дописывает свои коды сюда, не переиспользуя чужие.
    // Значения стабильны — менять нельзя, QML на них завязан.
    enum Event {
        NoEvent          = 0,
        PumpRateLow      = 1,  // dP/dt мала (REQ-020/076)
        Dv301OverRange   = 2,  // ДВ301 вне диапазона (REQ-079)
        Dv302OverRange   = 3,  // ДВ302 вне диапазона (REQ-081)
        TurboReadbackBad = 4   // закрытие К176 не подтверждено (REQ-082)
    };
    Q_ENUM(Event)

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
    Q_PROPERTY(int code READ code NOTIFY decisionRequired)
    Q_PROPERTY(QString title READ title NOTIFY decisionRequired)
    bool    active()  const { return m_decision == None && !m_activeMessage.isEmpty(); }
    QString message() const { return m_activeMessage; }
    int     code()    const { return m_activeCode; }

    // Заголовок диалога по коду события — чтобы оператор с первого взгляда
    // понимал, о каком узле установки идёт речь.
    QString title() const
    {
        switch (m_activeCode) {
        case PumpRateLow:      return QStringLiteral("Проверка откачки");
        case Dv301OverRange:   return QStringLiteral("ДВ301 вне диапазона");
        case Dv302OverRange:   return QStringLiteral("ДВ302 вне диапазона");
        case TurboReadbackBad: return QStringLiteral("Клапан К176 не подтверждён");
        }
        return QStringLiteral("Требуется решение оператора");
    }

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

// ─── OperatorPrompt ───────────────────────────────────────────────────────────
//
// Блокирующее ожидание решения оператора (REQ-020/033/069/079/081).
// QCustomTask-совместим: start() + done(bool). Pause-aware не нужен — задача и
// так стоит, пока человек не ответит.
//
// Без operatorBus решение получить неоткуда, поэтому done(false): «нет способа
// спросить» трактуется как отказ, а не как молчаливое согласие.

class OperatorPrompt : public QObject
{
    Q_OBJECT
public:
    explicit OperatorPrompt(QObject* parent = nullptr);

    int     code = OperatorBus::NoEvent;
    QString message;
    // Решения, которые считаются согласием продолжить. Остальные → done(false).
    QList<int> acceptDecisions { OperatorBus::Continue, OperatorBus::Retry };
    QPointer<OperatorBus> operatorBus;

    void start();                         // контракт QCustomTask

signals:
    void done(bool success);

private slots:
    void onDecision(int d);

private:
    bool m_awaiting = false;
};

