#pragma once

#include <QElapsedTimer>

namespace sim {

// Часы машины демо-данных. В приложении — монотонные, в тестах — ручные:
// время фаз ускоряется подменой часов, а не правкой профиля или рецепта.
class ISimClock {
public:
    virtual ~ISimClock() = default;
    virtual double nowSec() const = 0;
};

class SteadyClock final : public ISimClock {
public:
    SteadyClock() { m_timer.start(); }
    double nowSec() const override { return double(m_timer.nsecsElapsed()) / 1e9; }

private:
    QElapsedTimer m_timer;
};

class ManualClock final : public ISimClock {
public:
    double nowSec() const override { return m_now; }
    void advance(double sec) { m_now += sec; }
    void set(double sec) { m_now = sec; }

private:
    double m_now = 0.0;
};

} // namespace sim
