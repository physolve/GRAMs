#include "test_support.h"

#include "core/SimProfile.h"

#include <QDir>

using namespace sim;
using namespace simtest;

namespace {

ValidationResult validateFile(const QString &relative, Profile *out = nullptr)
{
    const QJsonDocument doc = readJson(profilePath(relative));
    const QJsonValue json = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    return parseProfile(json, gram50().catalog, out);
}

std::string dump(const QList<Issue> &issues)
{
    std::string s;
    for (const auto &i : issues)
        s += "  " + i.code.toStdString() + " @ " + i.path.toStdString() + "\n";
    return s;
}

bool hasIssue(const QList<Issue> &issues, const QString &code, const QString &path)
{
    for (const auto &i : issues)
        if (i.code == code && i.path == path) return true;
    return false;
}

struct Case {
    const char *file;
    const char *code;
    const char *path;
};

} // namespace

TEST(SimCatalog, Gram50HasRealIds)
{
    const Catalog &c = gram50().catalog;
    ASSERT_EQ(c.channels.size(), 18);
    ASSERT_NE(c.channel("P.DD311"), nullptr);
    EXPECT_EQ(c.channel("P.DD311")->zone, "accumulator");
    EXPECT_NEAR(c.channel("P.DD311")->max, 49.9924e5, 1e3);
    ASSERT_NE(c.channel("P.DD334"), nullptr);
    EXPECT_NEAR(c.channel("P.DD334")->max, 0.1e5, 10);
    EXPECT_DOUBLE_EQ(c.channel("P.DD334")->defaultValue, c.channel("P.DD334")->max);
    EXPECT_EQ(c.channel("T.DT341")->kind, ChannelKind::Temperature);
    EXPECT_EQ(c.channel("T.DT358")->zone, "chamber");
    EXPECT_EQ(c.channel("VAC.DV302")->scale, Scale::Log);
    ASSERT_EQ(c.valves.size(), 16);
    ASSERT_NE(c.valve("K176"), nullptr);
    EXPECT_EQ(c.valve("K176")->code, "AR6");
    EXPECT_EQ(c.valveByCode("SL2")->id, "K179");
    EXPECT_EQ(c.zones.size(), 4);
}

TEST(SimProfile, MinimalValidParses)
{
    Profile p;
    const auto r = validateFile("minimal_valid.json", &p);
    ASSERT_TRUE(r.ok()) << dump(r.errors);
    EXPECT_TRUE(r.warnings.isEmpty()) << dump(r.warnings);
    EXPECT_EQ(p.name, "fixture");
    ASSERT_TRUE(p.seed.has_value());
    EXPECT_EQ(*p.seed, 42u);
    EXPECT_DOUBLE_EQ(p.initial.value("P.DD331"), 101325);
    ASSERT_EQ(p.phases.size(), 2);
    const PhaseSpec &a = p.phases[0];
    EXPECT_EQ(a.kind, PhaseKind::Static);
    ASSERT_TRUE(a.durationSec.has_value());
    EXPECT_DOUBLE_EQ(*a.durationSec, 10);
    ASSERT_EQ(a.tracks.size(), 1);
    EXPECT_TRUE(a.tracks[0].keypoints[0].current);
    EXPECT_DOUBLE_EQ(a.tracks[0].sigmaRel, 0.01);
    ASSERT_EQ(a.transitions.size(), 1);
    EXPECT_EQ(a.transitions[0].type, TransitionSpec::Type::Timeout);
    EXPECT_FALSE(p.phases[1].durationSec.has_value());
    EXPECT_EQ(p.phaseIndex("b"), 1);
}

TEST(SimProfile, DefaultInterpByKind)
{
    const Catalog &c = gram50().catalog;
    EXPECT_EQ(defaultInterp(PhaseKind::Static, *c.channel("P.DD331")), Interp::Step);
    EXPECT_EQ(defaultInterp(PhaseKind::Pumping, *c.channel("VAC.DV301")), Interp::Log);
    EXPECT_EQ(defaultInterp(PhaseKind::Pumping, *c.channel("P.DD331")), Interp::Linear);
    EXPECT_EQ(defaultInterp(PhaseKind::GasInlet, *c.channel("P.DD311")), Interp::Linear);
}

class SimProfileErrors : public ::testing::TestWithParam<Case> {};

TEST_P(SimProfileErrors, ReportsCodeAtPath)
{
    const Case c = GetParam();
    const auto r = validateFile(QStringLiteral("invalid/%1.json").arg(QLatin1StringView(c.file)));
    EXPECT_FALSE(r.ok());
    EXPECT_TRUE(hasIssue(r.errors, QString::fromLatin1(c.code), QString::fromLatin1(c.path)))
        << c.file << ": ожидался " << c.code << " @ " << c.path << "\nполучено:\n" << dump(r.errors);
}

INSTANTIATE_TEST_SUITE_P(Fixtures, SimProfileErrors, ::testing::Values(
    Case{"not_object", "SCHEMA", ""},
    Case{"bad_schema", "SCHEMA", "schema"},
    Case{"empty_phases", "EMPTY_PHASES", "phases"},
    Case{"bad_phase_id", "BAD_PHASE_ID", "phases[0].id"},
    Case{"duplicate_phase_id", "DUPLICATE_PHASE_ID", "phases[1].id"},
    Case{"bad_kind", "BAD_KIND", "phases[0].kind"},
    Case{"bad_zone", "BAD_ZONE", "phases[0].zone"},
    Case{"h2_zone", "H2_ZONE", "phases[0].zone"},
    Case{"unknown_channel", "UNKNOWN_CHANNEL", "phases[0].tracks[0].channel"},
    Case{"duplicate_track", "DUPLICATE_TRACK", "phases[0].tracks[1].channel"},
    Case{"unknown_valve", "UNKNOWN_VALVE", "phases[0].transitions[0].on.valve"},
    Case{"unknown_target", "UNKNOWN_TARGET", "phases[0].transitions[0].to"},
    Case{"first_point_not_zero", "FIRST_POINT_NOT_ZERO", "phases[0].tracks[0].keypoints[0]"},
    Case{"non_monotonic_time", "NON_MONOTONIC_TIME", "phases[0].tracks[0].keypoints[2]"},
    Case{"current_not_first", "CURRENT_NOT_FIRST", "phases[0].tracks[0].keypoints[1]"},
    Case{"log_nonpositive", "LOG_NONPOSITIVE", "phases[0].tracks[0].keypoints[1]"},
    Case{"exp_no_tau", "EXP_NO_TAU", "phases[0].tracks[0].tau"}
), [](const auto &info) { return std::string(info.param.file); });

class SimProfileWarnings : public ::testing::TestWithParam<Case> {};

TEST_P(SimProfileWarnings, WarnsButStaysValid)
{
    const Case c = GetParam();
    const auto r = validateFile(QStringLiteral("invalid/%1.json").arg(QLatin1StringView(c.file)));
    EXPECT_TRUE(r.ok()) << dump(r.errors);
    EXPECT_TRUE(hasIssue(r.warnings, QString::fromLatin1(c.code), QString::fromLatin1(c.path)))
        << c.file << ": ожидалось " << c.code << " @ " << c.path << "\nполучено:\n" << dump(r.warnings);
}

INSTANTIATE_TEST_SUITE_P(Fixtures, SimProfileWarnings, ::testing::Values(
    Case{"warn_pressure_rise_in_pumping", "PRESSURE_RISE_IN_PUMPING", "phases[0].tracks[0].keypoints[2]"},
    Case{"warn_pressure_fall_in_inlet", "PRESSURE_FALL_IN_INLET", "phases[0].tracks[0].keypoints[1]"},
    Case{"warn_timeout_without_duration", "TIMEOUT_WITHOUT_DURATION", "phases[0].transitions[0]"},
    Case{"warn_value_out_of_range", "VALUE_OUT_OF_RANGE", "phases[0].tracks[0].keypoints[1]"},
    Case{"warn_unreachable_phase", "UNREACHABLE_PHASE", "phases[2]"}
), [](const auto &info) { return std::string(info.param.file); });

TEST(SimProfile, EveryFixtureIsCovered)
{
    // Новая фикстура без строки в таблицах выше — ошибка теста.
    const QStringList files = QDir(profilePath("invalid")).entryList({"*.json"}, QDir::Files);
    EXPECT_EQ(files.size(), 22);
}

TEST(SimProfile, IssuesToJson)
{
    const QJsonArray arr = issuesToJson({{"phases[0]", "BAD_KIND", "msg"}}).toArray();
    ASSERT_EQ(arr.size(), 1);
    EXPECT_EQ(arr[0].toObject().value("code").toString(), "BAD_KIND");
    EXPECT_EQ(arr[0].toObject().value("path").toString(), "phases[0]");
}
