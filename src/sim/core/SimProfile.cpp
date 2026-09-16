#include "SimProfile.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include <cmath>

namespace sim {

QString toWire(PhaseKind kind)
{
    switch (kind) {
    case PhaseKind::Static:   return QStringLiteral("static");
    case PhaseKind::Pumping:  return QStringLiteral("pumping");
    case PhaseKind::GasInlet: return QStringLiteral("gasInlet");
    case PhaseKind::H2Inlet:  return QStringLiteral("h2Inlet");
    }
    return {};
}

std::optional<PhaseKind> phaseKindFromWire(const QString &s)
{
    if (s == u"static")   return PhaseKind::Static;
    if (s == u"pumping")  return PhaseKind::Pumping;
    if (s == u"gasInlet") return PhaseKind::GasInlet;
    if (s == u"h2Inlet")  return PhaseKind::H2Inlet;
    return std::nullopt;
}

int Profile::phaseIndex(const QString &id) const
{
    for (int i = 0; i < phases.size(); ++i)
        if (phases[i].id == id) return i;
    return -1;
}

Interp defaultInterp(PhaseKind kind, const ChannelDesc &channel)
{
    switch (kind) {
    case PhaseKind::Static:
        return Interp::Step;
    case PhaseKind::Pumping:
        return channel.scale == Scale::Log ? Interp::Log : Interp::Linear;
    case PhaseKind::GasInlet:
    case PhaseKind::H2Inlet:
        return Interp::Linear;
    }
    return Interp::Linear;
}

QJsonValue issuesToJson(const QList<Issue> &issues)
{
    QJsonArray arr;
    for (const auto &i : issues)
        arr.append(QJsonObject{{"path", i.path}, {"code", i.code}, {"message", i.message}});
    return arr;
}

namespace {

std::optional<Interp> interpFromWire(const QString &s)
{
    if (s == u"step")   return Interp::Step;
    if (s == u"linear") return Interp::Linear;
    if (s == u"log")    return Interp::Log;
    if (s == u"exp")    return Interp::Exp;
    return std::nullopt;
}

class Validator {
public:
    Validator(const Catalog &catalog, Profile *out) : m_catalog(catalog), m_out(out) {}

    ValidationResult run(const QJsonValue &json)
    {
        if (!json.isObject()) {
            error({}, IssueCode::Schema, QStringLiteral("профиль должен быть JSON-объектом"));
            return m_result;
        }
        const QJsonObject root = json.toObject();

        if (root.value("schema").toString() != QLatin1StringView(kProfileSchema))
            error("schema", IssueCode::Schema,
                  QStringLiteral("ожидается schema = \"%1\"").arg(QLatin1StringView(kProfileSchema)));

        const QJsonValue name = root.value("name");
        if (!name.isString() || name.toString().trimmed().isEmpty())
            error("name", IssueCode::Schema, QStringLiteral("name — непустая строка"));
        else
            m_out->name = name.toString();

        parseSeed(root.value("seed"));
        parseInitial(root.value("initial"));

        const QJsonValue phases = root.value("phases");
        if (!phases.isArray()) {
            error("phases", IssueCode::Schema, QStringLiteral("phases — массив"));
            return m_result;
        }
        const QJsonArray arr = phases.toArray();
        if (arr.isEmpty()) {
            error("phases", IssueCode::EmptyPhases, QStringLiteral("в профиле нет фаз"));
            return m_result;
        }

        // Сначала все id: переходы могут ссылаться вперёд.
        for (int i = 0; i < arr.size(); ++i) {
            const QString id = arr[i].toObject().value("id").toString();
            if (!id.isEmpty()) m_phaseIds.insert(id);
        }
        QSet<QString> seen;
        for (int i = 0; i < arr.size(); ++i)
            parsePhase(arr[i], QStringLiteral("phases[%1]").arg(i), seen);

        if (m_result.ok())
            checkReachability();
        return m_result;
    }

private:
    void error(const QString &path, const char *code, const QString &msg)
    {
        m_result.errors.append({path, QString::fromLatin1(code), msg});
    }
    void warning(const QString &path, const char *code, const QString &msg)
    {
        m_result.warnings.append({path, QString::fromLatin1(code), msg});
    }

    void parseSeed(const QJsonValue &v)
    {
        if (v.isUndefined() || v.isNull())
            return;
        const double d = v.toDouble(-1);
        if (!v.isDouble() || d < 0 || d != std::floor(d) || d > 9007199254740991.0) {
            error("seed", IssueCode::Schema, QStringLiteral("seed — целое неотрицательное число"));
            return;
        }
        m_out->seed = std::uint64_t(d);
    }

    void parseInitial(const QJsonValue &v)
    {
        if (v.isUndefined() || v.isNull())
            return;
        if (!v.isObject()) {
            error("initial", IssueCode::Schema, QStringLiteral("initial — объект {канал: значение}"));
            return;
        }
        const QJsonObject obj = v.toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            const QString path = QStringLiteral("initial.%1").arg(it.key());
            const ChannelDesc *ch = m_catalog.channel(it.key());
            if (!ch) {
                error(path, IssueCode::UnknownChannel, QStringLiteral("неизвестный канал %1").arg(it.key()));
                continue;
            }
            if (!it.value().isDouble()) {
                error(path, IssueCode::Schema, QStringLiteral("значение — число"));
                continue;
            }
            const double value = it.value().toDouble();
            checkRange(path, *ch, value);
            m_out->initial.insert(it.key(), value);
        }
    }

    void checkRange(const QString &path, const ChannelDesc &ch, double value)
    {
        if (value < ch.min || value > ch.max)
            warning(path, IssueCode::ValueOutOfRange,
                    QStringLiteral("%1 вне диапазона канала [%2, %3] — будет ограничено")
                        .arg(value).arg(ch.min).arg(ch.max));
    }

    void parsePhase(const QJsonValue &v, const QString &path, QSet<QString> &seen)
    {
        if (!v.isObject()) {
            error(path, IssueCode::Schema, QStringLiteral("фаза — объект"));
            return;
        }
        const QJsonObject obj = v.toObject();
        PhaseSpec phase;

        static const QRegularExpression idRe(QStringLiteral("^[a-z0-9_]+$"));
        phase.id = obj.value("id").toString();
        if (!obj.value("id").isString() || !idRe.match(phase.id).hasMatch())
            error(path + ".id", IssueCode::BadPhaseId, QStringLiteral("id фазы — [a-z0-9_]+"));
        else if (seen.contains(phase.id))
            error(path + ".id", IssueCode::DuplicatePhaseId, QStringLiteral("id %1 повторяется").arg(phase.id));
        seen.insert(phase.id);

        const auto kind = phaseKindFromWire(obj.value("kind").toString());
        if (!kind)
            error(path + ".kind", IssueCode::BadKind,
                  QStringLiteral("kind — static | pumping | gasInlet | h2Inlet"));
        else
            phase.kind = *kind;

        phase.zone = obj.value("zone").toString();
        if (!m_catalog.hasZone(phase.zone))
            error(path + ".zone", IssueCode::BadZone,
                  QStringLiteral("zone — supply | accumulator | chamber | vacuum"));
        else if (kind == PhaseKind::H2Inlet && phase.zone != u"chamber")
            error(path + ".zone", IssueCode::H2Zone,
                  QStringLiteral("напуск водорода допустим только в камеру (zone = chamber)"));

        const QJsonValue label = obj.value("label");
        if (!label.isUndefined() && !label.isNull() && !label.isString())
            error(path + ".label", IssueCode::Schema, QStringLiteral("label — строка"));
        phase.label = label.toString();

        const QJsonValue dur = obj.value("durationSec");
        if (!dur.isUndefined() && !dur.isNull()) {
            if (!dur.isDouble() || dur.toDouble() < 0)
                error(path + ".durationSec", IssueCode::Schema,
                      QStringLiteral("durationSec — неотрицательное число или null"));
            else
                phase.durationSec = dur.toDouble();
        }

        const QJsonValue tracks = obj.value("tracks");
        if (!tracks.isUndefined() && !tracks.isArray()) {
            error(path + ".tracks", IssueCode::Schema, QStringLiteral("tracks — массив"));
        } else {
            const QJsonArray arr = tracks.toArray();
            QSet<QString> channels;
            for (int i = 0; i < arr.size(); ++i)
                parseTrack(arr[i], QStringLiteral("%1.tracks[%2]").arg(path).arg(i), phase, channels);
        }

        const QJsonValue transitions = obj.value("transitions");
        if (!transitions.isUndefined() && !transitions.isArray()) {
            error(path + ".transitions", IssueCode::Schema, QStringLiteral("transitions — массив"));
        } else {
            const QJsonArray arr = transitions.toArray();
            for (int i = 0; i < arr.size(); ++i)
                parseTransition(arr[i], QStringLiteral("%1.transitions[%2]").arg(path).arg(i), phase);
        }

        m_out->phases.append(phase);
    }

    void parseTrack(const QJsonValue &v, const QString &path, PhaseSpec &phase, QSet<QString> &channels)
    {
        if (!v.isObject()) {
            error(path, IssueCode::Schema, QStringLiteral("трек — объект"));
            return;
        }
        const QJsonObject obj = v.toObject();
        TrackSpec track;
        track.channel = obj.value("channel").toString();
        const ChannelDesc *ch = m_catalog.channel(track.channel);
        if (!ch) {
            error(path + ".channel", IssueCode::UnknownChannel,
                  QStringLiteral("неизвестный канал %1").arg(track.channel));
            return;
        }
        if (channels.contains(track.channel))
            error(path + ".channel", IssueCode::DuplicateTrack,
                  QStringLiteral("канал %1 уже имеет трек в этой фазе").arg(track.channel));
        channels.insert(track.channel);

        const QJsonValue interp = obj.value("interp");
        if (interp.isUndefined() || interp.isNull()) {
            track.interp = defaultInterp(phase.kind, *ch);
        } else if (const auto parsed = interpFromWire(interp.toString())) {
            track.interp = *parsed;
        } else {
            error(path + ".interp", IssueCode::Schema, QStringLiteral("interp — step | linear | log | exp"));
            return;
        }

        if (track.interp == Interp::Exp) {
            const QJsonValue tau = obj.value("tau");
            if (!tau.isDouble() || tau.toDouble() <= 0)
                error(path + ".tau", IssueCode::ExpNoTau, QStringLiteral("для exp нужен tau > 0"));
            else
                track.tau = tau.toDouble();
        }

        const QJsonValue noise = obj.value("noise");
        if (!noise.isUndefined() && !noise.isNull()) {
            const QJsonObject n = noise.toObject();
            const QJsonValue rel = n.value("sigmaRel");
            const QJsonValue abs = n.value("sigmaAbs");
            auto bad = [](const QJsonValue &x) {
                return !x.isUndefined() && (!x.isDouble() || x.toDouble() < 0);
            };
            if (!noise.isObject() || bad(rel) || bad(abs))
                error(path + ".noise", IssueCode::Schema,
                      QStringLiteral("noise — {sigmaRel ≥ 0, sigmaAbs ≥ 0}"));
            track.sigmaRel = rel.toDouble(0);
            track.sigmaAbs = abs.toDouble(0);
        }

        const QJsonValue kps = obj.value("keypoints");
        if (!kps.isArray() || kps.toArray().isEmpty()) {
            error(path + ".keypoints", IssueCode::Schema, QStringLiteral("keypoints — непустой массив [t, v]"));
            return;
        }
        const QJsonArray arr = kps.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            const QString kpPath = QStringLiteral("%1.keypoints[%2]").arg(path).arg(i);
            const QJsonArray pair = arr[i].toArray();
            if (!arr[i].isArray() || pair.size() != 2 || !pair[0].isDouble()) {
                error(kpPath, IssueCode::Schema, QStringLiteral("ключевая точка — [t, v]"));
                return;
            }
            Keypoint kp;
            kp.t = pair[0].toDouble();
            if (pair[1].isString() && pair[1].toString() == u"current") {
                kp.current = true;
                if (i != 0)
                    error(kpPath, IssueCode::CurrentNotFirst,
                          QStringLiteral("\"current\" допустим только в первой точке"));
            } else if (pair[1].isDouble()) {
                kp.v = pair[1].toDouble();
                checkRange(kpPath, *ch, kp.v);
                if (track.interp == Interp::Log && kp.v <= 0)
                    error(kpPath, IssueCode::LogNonPositive, QStringLiteral("для log все значения > 0"));
            } else {
                error(kpPath, IssueCode::Schema, QStringLiteral("значение — число или \"current\""));
                return;
            }
            if (i == 0 && kp.t != 0)
                error(kpPath, IssueCode::FirstPointNotZero, QStringLiteral("первая точка — t = 0"));
            if (i > 0 && kp.t <= track.keypoints.last().t)
                error(kpPath, IssueCode::NonMonotonicTime, QStringLiteral("t должно строго возрастать"));
            track.keypoints.append(kp);
        }

        checkDirection(path, phase, *ch, track);
        phase.tracks.append(track);
    }

    // Откачка — давление не должно расти, напуск — падать. Проверяются только
    // каналы давления участка фазы: в h2Inlet камера растёт, а резервуар
    // законно падает. Отрезки от "current" неизвестны до входа — пропускаются.
    void checkDirection(const QString &path, const PhaseSpec &phase, const ChannelDesc &ch, const TrackSpec &track)
    {
        if (!isPressureLike(ch.kind) || ch.zone != phase.zone)
            return;
        const bool pumping = phase.kind == PhaseKind::Pumping;
        const bool inlet = phase.kind == PhaseKind::GasInlet || phase.kind == PhaseKind::H2Inlet;
        if (!pumping && !inlet)
            return;
        for (int i = 1; i < track.keypoints.size(); ++i) {
            const Keypoint &a = track.keypoints[i - 1];
            const Keypoint &b = track.keypoints[i];
            if (a.current || b.current)
                continue;
            const QString kpPath = QStringLiteral("%1.keypoints[%2]").arg(path).arg(i);
            if (pumping && b.v > a.v)
                warning(kpPath, IssueCode::PressureRiseInPumping,
                        QStringLiteral("давление растёт во время откачки (натекание?)"));
            if (inlet && b.v < a.v)
                warning(kpPath, IssueCode::PressureFallInInlet,
                        QStringLiteral("давление падает во время напуска"));
        }
    }

    void parseTransition(const QJsonValue &v, const QString &path, PhaseSpec &phase)
    {
        const QJsonObject obj = v.toObject();
        if (!v.isObject() || !obj.value("on").isObject()) {
            error(path, IssueCode::Schema, QStringLiteral("переход — {to, on: {type, ...}}"));
            return;
        }
        TransitionSpec t;
        t.to = obj.value("to").toString();
        if (!m_phaseIds.contains(t.to))
            error(path + ".to", IssueCode::UnknownTarget, QStringLiteral("нет фазы %1").arg(t.to));

        const QJsonObject on = obj.value("on").toObject();
        const QString type = on.value("type").toString();
        if (type == u"timeout") {
            t.type = TransitionSpec::Type::Timeout;
            if (!phase.durationSec)
                warning(path, IssueCode::TimeoutWithoutDuration,
                        QStringLiteral("timeout у фазы без durationSec никогда не сработает"));
        } else if (type == u"valve") {
            t.type = TransitionSpec::Type::Valve;
            t.valve = on.value("valve").toString();
            if (!m_catalog.valve(t.valve))
                error(path + ".on.valve", IssueCode::UnknownValve, QStringLiteral("неизвестный клапан %1").arg(t.valve));
            const QString state = on.value("state").toString();
            if (state == u"open")
                t.open = true;
            else if (state == u"closed")
                t.open = false;
            else
                error(path + ".on.state", IssueCode::Schema, QStringLiteral("state — open | closed"));
        } else if (type == u"manual") {
            t.type = TransitionSpec::Type::Manual;
        } else {
            error(path + ".on.type", IssueCode::Schema, QStringLiteral("type — timeout | valve | manual"));
            return;
        }
        phase.transitions.append(t);
    }

    void checkReachability()
    {
        QSet<QString> reached{m_out->phases.first().id};
        QList<QString> queue{m_out->phases.first().id};
        while (!queue.isEmpty()) {
            const QString id = queue.takeFirst();
            const int idx = m_out->phaseIndex(id);
            for (const auto &t : m_out->phases[idx].transitions) {
                if (!reached.contains(t.to)) {
                    reached.insert(t.to);
                    queue.append(t.to);
                }
            }
        }
        for (int i = 0; i < m_out->phases.size(); ++i)
            if (!reached.contains(m_out->phases[i].id))
                warning(QStringLiteral("phases[%1]").arg(i), IssueCode::UnreachablePhase,
                        QStringLiteral("фаза недостижима из первой — только через sim.goto / startPhaseId"));
    }

    const Catalog &m_catalog;
    Profile *m_out;
    ValidationResult m_result;
    QSet<QString> m_phaseIds;
};

} // namespace

ValidationResult parseProfile(const QJsonValue &json, const Catalog &catalog, Profile *out)
{
    Profile scratch;
    Validator v(catalog, out ? out : &scratch);
    if (out) *out = Profile{};
    return v.run(json);
}

} // namespace sim
