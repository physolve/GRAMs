#pragma once

// Интерполяция ключевых точек и шум — формулы раздела 2.4 контракта.
// Чистые функции, без таймеров: время трека передаётся явно.

#include <QVector>

#include <cstdint>
#include <random>

namespace sim {

enum class Interp { Step, Linear, Log, Exp };

struct Keypoint {
    double t = 0.0;
    double v = 0.0;
    bool current = false;   // "current": значение канала на входе в фазу
};

namespace TrackEngine {

// Ключевые точки уже разрешены ("current" подставлен), t строго возрастает.
// До первой точки — её значение, после последней — удержание последнего.
// tau используется только для Interp::Exp. Пустой список → NaN.
double evaluate(Interp interp, double tau, const QVector<Keypoint> &points, double t);

// Подставляет currentValue вместо точек с current == true.
QVector<Keypoint> resolve(const QVector<Keypoint> &points, double currentValue);

// Ограничение диапазоном канала; для логарифмической шкалы — не ниже min.
double clampToRange(double v, double min, double max);

} // namespace TrackEngine

// Воспроизводимый гауссов шум. std::normal_distribution зависит от
// реализации стандартной библиотеки, поэтому нормальное распределение
// строится здесь (Бокс–Мюллер поверх mt19937_64, чей выход стандартизован).
class SimNoise {
public:
    explicit SimNoise(std::uint64_t seed = 0);

    void reseed(std::uint64_t seed);
    double gaussian();   // N(0, 1)

    // v' = v + N(0, (sigmaRel·|v|)² + sigmaAbs²)
    double apply(double v, double sigmaRel, double sigmaAbs);

private:
    double uniformOpen();   // (0, 1)

    std::mt19937_64 m_engine;
    bool m_hasSpare = false;
    double m_spare = 0.0;
};

} // namespace sim
