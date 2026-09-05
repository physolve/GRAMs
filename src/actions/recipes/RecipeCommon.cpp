#include "RecipeCommon.h"

// Реализации общей инфраструктуры рецептов. Вынесено из VacuumTaskTree.cpp
// вместе с объявлениями: напуску и натеканию нужны те же примитивы.
// PumpRateWatchdog сюда НЕ переносится — он про dP/dt конкретно откачки.

// ═════════════════════════════════════════════════════════════════════════════
// PausableTicker
// ═════════════════════════════════════════════════════════════════════════════

PausableTicker::PausableTicker(QObject* parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &PausableTicker::tick);
}

void PausableTicker::start()
{
    if (isComplete && isComplete(0)) {
        emit done(true);
        return;
    }
    if (pauseBus) {
        connect(pauseBus, &PauseBus::paused,  this, [this] { m_timer.stop(); });
        connect(pauseBus, &PauseBus::resumed, this, [this] { m_timer.start(intervalMs); });
        if (pauseBus->isPaused())
            return;  // стартуем замороженными, ждём resumed()
    }
    m_timer.start(intervalMs);
}

void PausableTicker::tick()
{
    ++m_elapsed;
    if (onTick)
        onTick(m_elapsed);
    if (!isComplete || isComplete(m_elapsed)) {
        m_timer.stop();
        emit done(true);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// OperatorPrompt
// ═════════════════════════════════════════════════════════════════════════════

OperatorPrompt::OperatorPrompt(QObject* parent) : QObject(parent) {}

void OperatorPrompt::start()
{
    if (!operatorBus) {
        // Спросить некого. Молчаливое «продолжаем» здесь означало бы обойти
        // требование ТЗ о решении оператора, поэтому — безопасный отказ.
        emit done(false);
        return;
    }
    connect(operatorBus, &OperatorBus::decisionReceived, this,
            [this](OperatorBus::Decision d) { onDecision(int(d)); },
            Qt::SingleShotConnection);
    m_awaiting = true;
    operatorBus->request(code, message);
}

void OperatorPrompt::onDecision(int d)
{
    if (!m_awaiting)
        return;
    m_awaiting = false;
    emit done(acceptDecisions.contains(d));
}

