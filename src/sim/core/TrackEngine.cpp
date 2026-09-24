#include "TrackEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace sim {

namespace TrackEngine {

double evaluate(Interp interp, double tau, const QVector<Keypoint> &points, double t)
{
    if (points.isEmpty())
        return std::numeric_limits<double>::quiet_NaN();
    if (t <= points.first().t)
        return points.first().v;
    if (t >= points.last().t)
        return points.last().v;

    // Отрезок [ti, tj], содержащий t.
    qsizetype j = 1;
    while (j < points.size() && points[j].t <= t)
        ++j;
    const Keypoint &a = points[j - 1];
    const Keypoint &b = points[j];
    if (t == a.t)
        return a.v;

    const double span = b.t - a.t;
    const double s = (t - a.t) / span;

    switch (interp) {
    case Interp::Step:
        return a.v;
    case Interp::Linear:
        return a.v + (b.v - a.v) * s;
    case Interp::Log: {
        const double la = std::log10(a.v);
        const double lb = std::log10(b.v);
        return std::pow(10.0, la + (lb - la) * s);
    }
    case Interp::Exp: {
        const double num = 1.0 - std::exp(-(t - a.t) / tau);
        const double den = 1.0 - std::exp(-span / tau);
        return a.v + (b.v - a.v) * num / den;
    }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

QVector<Keypoint> resolve(const QVector<Keypoint> &points, double currentValue)
{
    QVector<Keypoint> out = points;
    for (auto &p : out) {
        if (p.current) {
            p.v = currentValue;
            p.current = false;
        }
    }
    return out;
}

double clampToRange(double v, double min, double max)
{
    return std::clamp(v, min, max);
}

} // namespace TrackEngine

SimNoise::SimNoise(std::uint64_t seed) : m_engine(seed) {}

void SimNoise::reseed(std::uint64_t seed)
{
    m_engine.seed(seed);
    m_hasSpare = false;
    m_spare = 0.0;
}

double SimNoise::uniformOpen()
{
    // 53 старших бита → [0, 1); ноль исключаем для логарифма.
    for (;;) {
        const double u = double(m_engine() >> 11) * (1.0 / 9007199254740992.0);
        if (u > 0.0)
            return u;
    }
}

double SimNoise::gaussian()
{
    if (m_hasSpare) {
        m_hasSpare = false;
        return m_spare;
    }
    const double u1 = uniformOpen();
    const double u2 = uniformOpen();
    const double r = std::sqrt(-2.0 * std::log(u1));
    const double phi = 2.0 * std::numbers::pi * u2;
    m_spare = r * std::sin(phi);
    m_hasSpare = true;
    return r * std::cos(phi);
}

double SimNoise::apply(double v, double sigmaRel, double sigmaAbs)
{
    const double rel = sigmaRel * std::abs(v);
    const double sigma = std::sqrt(rel * rel + sigmaAbs * sigmaAbs);
    if (sigma <= 0.0)
        return v;
    return v + sigma * gaussian();
}

} // namespace sim
