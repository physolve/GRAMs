#include <gtest/gtest.h>

#include "core/TrackEngine.h"

#include <cmath>

using namespace sim;

namespace {

QVector<Keypoint> pts(std::initializer_list<std::pair<double, double>> list)
{
    QVector<Keypoint> out;
    for (const auto &[t, v] : list)
        out.append(Keypoint{t, v, false});
    return out;
}

constexpr double kEps = 1e-9;

} // namespace

TEST(TrackEngine, EmptyIsNaN)
{
    EXPECT_TRUE(std::isnan(TrackEngine::evaluate(Interp::Linear, 0, {}, 1.0)));
}

TEST(TrackEngine, StepHoldsLeftValue)
{
    const auto p = pts({{0, 10}, {5, 20}, {10, 30}});
    EXPECT_DOUBLE_EQ(TrackEngine::evaluate(Interp::Step, 0, p, 0.0), 10);
    EXPECT_DOUBLE_EQ(TrackEngine::evaluate(Interp::Step, 0, p, 4.999), 10);
    EXPECT_DOUBLE_EQ(TrackEngine::evaluate(Interp::Step, 0, p, 5.0), 20);
    EXPECT_DOUBLE_EQ(TrackEngine::evaluate(Interp::Step, 0, p, 7.5), 20);
    EXPECT_DOUBLE_EQ(TrackEngine::evaluate(Interp::Step, 0, p, 10.0), 30);
}

TEST(TrackEngine, LinearKeypointsAndMidpoints)
{
    const auto p = pts({{0, 100}, {10, 200}, {30, 0}});
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, p, 0), 100, kEps);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, p, 5), 150, kEps);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, p, 10), 200, kEps);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, p, 20), 100, kEps);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, p, 30), 0, kEps);
}

TEST(TrackEngine, LogIsGeometricBetweenPoints)
{
    const auto p = pts({{0, 100000}, {60, 1000}, {240, 10}});
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Log, 0, p, 0), 100000, 1e-6);
    // середина первого отрезка: 10^((5+3)/2) = 1e4
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Log, 0, p, 30), 10000, 1e-6);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Log, 0, p, 60), 1000, 1e-9);
    // середина второго: 10^((3+1)/2) = 100
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Log, 0, p, 150), 100, 1e-9);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Log, 0, p, 240), 10, 1e-12);
}

TEST(TrackEngine, ExpHitsEndpoints)
{
    for (double tau : {0.5, 5.0, 40.0, 1e4}) {
        const auto p = pts({{0, 1000}, {240, 10}});
        EXPECT_NEAR(TrackEngine::evaluate(Interp::Exp, tau, p, 0), 1000, kEps) << tau;
        EXPECT_NEAR(TrackEngine::evaluate(Interp::Exp, tau, p, 240), 10, kEps) << tau;
        EXPECT_NEAR(TrackEngine::evaluate(Interp::Exp, tau, p, 239.9999999), 10, 1e-4) << tau;
    }
}

TEST(TrackEngine, ExpMidpointMatchesFormula)
{
    const auto p = pts({{10, 0}, {50, 100}});
    const double tau = 20;
    const double t = 30;
    const double expected = 100 * (1 - std::exp(-(t - 10) / tau)) / (1 - std::exp(-40 / tau));
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Exp, tau, p, t), expected, kEps);
}

TEST(TrackEngine, HoldsBeforeFirstAndAfterLast)
{
    const auto p = pts({{5, 7}, {10, 9}});
    for (auto interp : {Interp::Step, Interp::Linear, Interp::Log, Interp::Exp}) {
        EXPECT_DOUBLE_EQ(TrackEngine::evaluate(interp, 3, p, 0), 7);
        EXPECT_DOUBLE_EQ(TrackEngine::evaluate(interp, 3, p, 1e6), 9);
    }
}

TEST(TrackEngine, ResolveCurrent)
{
    QVector<Keypoint> p{{0, 0, true}, {60, 2000, false}};
    const auto r = TrackEngine::resolve(p, 101325);
    EXPECT_FALSE(r[0].current);
    EXPECT_DOUBLE_EQ(r[0].v, 101325);
    EXPECT_DOUBLE_EQ(r[1].v, 2000);
    EXPECT_NEAR(TrackEngine::evaluate(Interp::Linear, 0, r, 30), (101325 + 2000) / 2.0, kEps);
}

TEST(TrackEngine, ClampToRange)
{
    EXPECT_DOUBLE_EQ(TrackEngine::clampToRange(-5, 0, 10), 0);
    EXPECT_DOUBLE_EQ(TrackEngine::clampToRange(15, 0, 10), 10);
    EXPECT_DOUBLE_EQ(TrackEngine::clampToRange(5, 0, 10), 5);
}

TEST(SimNoise, ReproducibleBySeed)
{
    SimNoise a(42), b(42), c(43);
    bool differs = false;
    for (int i = 0; i < 1000; ++i) {
        const double x = a.gaussian();
        EXPECT_DOUBLE_EQ(x, b.gaussian());
        if (x != c.gaussian()) differs = true;
    }
    EXPECT_TRUE(differs);
}

TEST(SimNoise, ReseedRestartsSequence)
{
    SimNoise a(7);
    const double first = a.gaussian();
    a.gaussian();
    a.gaussian();
    a.reseed(7);
    EXPECT_DOUBLE_EQ(a.gaussian(), first);
}

TEST(SimNoise, StatisticsRoughlyStandardNormal)
{
    SimNoise n(1);
    const int count = 200000;
    double sum = 0, sq = 0;
    for (int i = 0; i < count; ++i) {
        const double x = n.gaussian();
        sum += x;
        sq += x * x;
    }
    EXPECT_NEAR(sum / count, 0.0, 0.01);
    EXPECT_NEAR(sq / count, 1.0, 0.02);
}

TEST(SimNoise, ZeroSigmaIsIdentity)
{
    SimNoise n(3);
    EXPECT_DOUBLE_EQ(n.apply(123.0, 0.0, 0.0), 123.0);
}

TEST(SimNoise, SigmaCombinesRelAndAbs)
{
    SimNoise n(5);
    const int count = 100000;
    double sq = 0;
    for (int i = 0; i < count; ++i) {
        const double d = n.apply(100.0, 0.03, 4.0) - 100.0;
        sq += d * d;
    }
    // σ² = 3² + 4² = 25
    EXPECT_NEAR(sq / count, 25.0, 0.6);
}
