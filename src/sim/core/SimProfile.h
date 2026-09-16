#pragma once

// Профиль grams.sim.profile/1 (раздел 2.4 контракта): разбор и валидация.

#include "SimTypes.h"
#include "TrackEngine.h"

#include <QHash>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QVector>

#include <cstdint>
#include <optional>

namespace sim {

inline constexpr auto kProfileSchema = "grams.sim.profile/1";

enum class PhaseKind { Static, Pumping, GasInlet, H2Inlet };

QString toWire(PhaseKind kind);
std::optional<PhaseKind> phaseKindFromWire(const QString &s);

struct TrackSpec {
    QString channel;
    Interp interp = Interp::Linear;
    double tau = 0.0;
    QVector<Keypoint> keypoints;
    double sigmaRel = 0.0;
    double sigmaAbs = 0.0;
};

struct TransitionSpec {
    enum class Type { Timeout, Valve, Manual };
    QString to;
    Type type = Type::Manual;
    QString valve;      // id клапана из каталога, для Type::Valve
    bool open = false;  // целевое состояние, для Type::Valve
};

struct PhaseSpec {
    QString id;
    PhaseKind kind = PhaseKind::Static;
    QString zone;
    QString label;
    std::optional<double> durationSec;   // пусто — держать до перехода
    QVector<TrackSpec> tracks;
    QVector<TransitionSpec> transitions;
};

struct Profile {
    QString name;
    std::optional<std::uint64_t> seed;
    QHash<QString, double> initial;
    QVector<PhaseSpec> phases;

    int phaseIndex(const QString &id) const;
};

struct ValidationResult {
    QList<Issue> errors;
    QList<Issue> warnings;
    bool ok() const { return errors.isEmpty(); }
};

// Умолчание интерполяции, если в треке нет "interp". Контракт поле
// не делает обязательным; выбор зависит от вида фазы и шкалы канала.
Interp defaultInterp(PhaseKind kind, const ChannelDesc &channel);

// Разбирает и проверяет профиль против каталога. При result.ok()
// out заполнен полностью; при ошибках — частично, использовать нельзя.
ValidationResult parseProfile(const QJsonValue &json, const Catalog &catalog, Profile *out);

// Коды Issue — предложение к согласованию (docs/sim/rpc-contract.md).
namespace IssueCode {
inline constexpr auto Schema = "SCHEMA";
inline constexpr auto EmptyPhases = "EMPTY_PHASES";
inline constexpr auto BadPhaseId = "BAD_PHASE_ID";
inline constexpr auto DuplicatePhaseId = "DUPLICATE_PHASE_ID";
inline constexpr auto BadKind = "BAD_KIND";
inline constexpr auto BadZone = "BAD_ZONE";
inline constexpr auto UnknownChannel = "UNKNOWN_CHANNEL";
inline constexpr auto DuplicateTrack = "DUPLICATE_TRACK";
inline constexpr auto UnknownValve = "UNKNOWN_VALVE";
inline constexpr auto UnknownTarget = "UNKNOWN_TARGET";
inline constexpr auto FirstPointNotZero = "FIRST_POINT_NOT_ZERO";
inline constexpr auto NonMonotonicTime = "NON_MONOTONIC_TIME";
inline constexpr auto CurrentNotFirst = "CURRENT_NOT_FIRST";
inline constexpr auto LogNonPositive = "LOG_NONPOSITIVE";
inline constexpr auto ExpNoTau = "EXP_NO_TAU";
inline constexpr auto H2Zone = "H2_ZONE";
// warnings
inline constexpr auto PressureRiseInPumping = "PRESSURE_RISE_IN_PUMPING";
inline constexpr auto PressureFallInInlet = "PRESSURE_FALL_IN_INLET";
inline constexpr auto TimeoutWithoutDuration = "TIMEOUT_WITHOUT_DURATION";
inline constexpr auto ValueOutOfRange = "VALUE_OUT_OF_RANGE";
inline constexpr auto UnreachablePhase = "UNREACHABLE_PHASE";
} // namespace IssueCode

QJsonValue issuesToJson(const QList<Issue> &issues);

} // namespace sim
