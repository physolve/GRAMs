#include <gtest/gtest.h>

#include "app/SimOptions.h"

using namespace sim;

namespace {
QProcessEnvironment env(std::initializer_list<std::pair<const char *, const char *>> vars)
{
    QProcessEnvironment e;
    for (const auto &[k, v] : vars)
        e.insert(QString::fromLatin1(k), QString::fromLatin1(v));
    return e;
}
} // namespace

TEST(SimOptions, DisabledByDefault)
{
    const auto o = SimOptions::parse({"GRAMs.exe"}, env({}));
    EXPECT_FALSE(o.enabled);
    EXPECT_EQ(o.rpcPort, 8770);
    EXPECT_TRUE(o.token.isEmpty());
    EXPECT_TRUE(o.error.isEmpty());
}

TEST(SimOptions, Flags)
{
    auto o = SimOptions::parse({"GRAMs.exe", "--sim", "--sim-rpc-port", "9001", "--sim-token=abc"}, env({}));
    EXPECT_TRUE(o.enabled);
    EXPECT_EQ(o.rpcPort, 9001);
    EXPECT_EQ(o.token, "abc");

    o = SimOptions::parse({"GRAMs.exe", "--sim-rpc-port=9002", "--sim-token", "xyz"}, env({}));
    EXPECT_FALSE(o.enabled);   // порт и токен без --sim ничего не включают
    EXPECT_EQ(o.rpcPort, 9002);
}

TEST(SimOptions, Environment)
{
    const auto o = SimOptions::parse({"GRAMs.exe"},
                                     env({{"GRAMS_SIM", "1"}, {"GRAMS_SIM_TOKEN", "t"}, {"GRAMS_SIM_RPC_PORT", "8800"}}));
    EXPECT_TRUE(o.enabled);
    EXPECT_EQ(o.token, "t");
    EXPECT_EQ(o.rpcPort, 8800);
    EXPECT_FALSE(SimOptions::parse({"GRAMs.exe"}, env({{"GRAMS_SIM", "0"}})).enabled);
}

TEST(SimOptions, FlagsOverrideEnvironment)
{
    const auto o = SimOptions::parse({"GRAMs.exe", "--sim-token", "cli"}, env({{"GRAMS_SIM_TOKEN", "env"}}));
    EXPECT_EQ(o.token, "cli");
}

TEST(SimOptions, BadPortDisables)
{
    auto o = SimOptions::parse({"GRAMs.exe", "--sim", "--sim-rpc-port", "70000"}, env({}));
    EXPECT_FALSE(o.enabled);
    EXPECT_FALSE(o.error.isEmpty());
    o = SimOptions::parse({"GRAMs.exe", "--sim", "--sim-rpc-port"}, env({}));
    EXPECT_FALSE(o.enabled);
    EXPECT_FALSE(o.error.isEmpty());
}
