#include "SimRpcDispatcher.h"

#include "core/SimCatalog.h"
#include "core/SimController.h"
#include "core/SimErrors.h"
#include "core/SimProfile.h"

#include <QJsonDocument>

namespace sim {

namespace {

QJsonObject ok(const QJsonValue &id, const QJsonValue &result)
{
    return {{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
}

QJsonObject fail(const QJsonValue &id, int code, const QString &message, const QJsonValue &data = {})
{
    QJsonObject err{{"code", code}, {"message", message}};
    if (!data.isUndefined())
        err.insert("data", data);
    return {{"jsonrpc", "2.0"}, {"id", id}, {"error", err}};
}

bool validId(const QJsonValue &id)
{
    return id.isUndefined() || id.isNull() || id.isString() || id.isDouble();
}

} // namespace

SimRpcDispatcher::SimRpcDispatcher(SimController *controller, RpcConfig config)
    : m_controller(controller), m_config(std::move(config))
{
}

QJsonObject SimRpcDispatcher::handle(const QByteArray &body, const std::optional<QByteArray> &token) const
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    const QJsonValue id = doc.isObject() && validId(doc.object().value("id"))
                              ? doc.object().value("id") : QJsonValue(QJsonValue::Null);
    const QJsonValue replyId = id.isUndefined() ? QJsonValue(QJsonValue::Null) : id;

    // Токен проверяется до разбора запроса: без него ничего не исполняется.
    if (!m_config.token.isEmpty() && (!token || *token != m_config.token))
        return fail(replyId, RpcError::Unauthorized, QStringLiteral("неверный токен X-Grams-Sim-Token"));

    if (err.error != QJsonParseError::NoError)
        return fail(QJsonValue::Null, RpcError::ParseError, QStringLiteral("parse error: %1").arg(err.errorString()));
    if (!doc.isObject())
        return fail(QJsonValue::Null, RpcError::InvalidRequest, QStringLiteral("ожидается один объект запроса, батчи не поддерживаются"));

    const QJsonObject req = doc.object();
    if (req.value("jsonrpc").toString() != u"2.0" || !req.value("method").isString()
        || !validId(req.value("id")))
        return fail(replyId, RpcError::InvalidRequest, QStringLiteral("invalid request"));

    const QJsonValue params = req.value("params");
    if (!params.isUndefined() && !params.isNull() && !params.isObject())
        return fail(replyId, RpcError::InvalidParams, QStringLiteral("params — объект"));

    return dispatch(req.value("method").toString(), params.toObject(), replyId);
}

QJsonObject SimRpcDispatcher::dispatch(const QString &method, const QJsonObject &params, const QJsonValue &id) const
{
    const Catalog &catalog = m_controller->catalog();

    if (method == u"sim.hello") {
        return ok(id, QJsonObject{
            {"api", "grams.sim/1"},
            {"appVersion", m_config.appVersion},
            {"simEnabled", true},   // без --sim сервер не поднимается вовсе
            {"simAllowed", m_config.simAllowed},
            {"reason", m_config.simAllowed ? QJsonValue(QJsonValue::Null) : QJsonValue(m_config.notAllowedReason)},
        });
    }
    if (method == u"sim.catalog")
        return ok(id, catalogToJson(catalog, m_controller->values(), m_controller->valveStates()));
    if (method == u"sim.status")
        return ok(id, m_controller->statusJson(m_config.regime ? m_config.regime() : QJsonValue(QJsonValue::Null)));
    if (method == u"sim.validate") {
        if (!params.contains("profile"))
            return fail(id, RpcError::InvalidParams, QStringLiteral("нужен params.profile"));
        const ValidationResult r = parseProfile(params.value("profile"), catalog, nullptr);
        return ok(id, QJsonObject{{"ok", r.ok()},
                                  {"errors", issuesToJson(r.errors)},
                                  {"warnings", issuesToJson(r.warnings)}});
    }

    const bool control = method == u"sim.run" || method == u"sim.goto" || method == u"sim.pause"
                      || method == u"sim.resume" || method == u"sim.stop";
    if (!control)
        return fail(id, RpcError::MethodNotFound, QStringLiteral("неизвестный метод %1").arg(method));

    if (!m_config.simAllowed)
        return fail(id, RpcError::SimNotAllowed, QStringLiteral("симуляция запрещена"),
                    QJsonObject{{"reason", m_config.notAllowedReason}});

    auto toReply = [&](const SimController::Result &r, const QJsonValue &result) {
        if (r.ok())
            return ok(id, result);
        return fail(id, r.code, r.message);
    };

    if (method == u"sim.run") {
        if (!params.contains("profile"))
            return fail(id, RpcError::InvalidParams, QStringLiteral("нужен params.profile"));
        const QJsonValue start = params.value("startPhaseId");
        if (!start.isUndefined() && !start.isNull() && !start.isString())
            return fail(id, RpcError::InvalidParams, QStringLiteral("startPhaseId — строка"));
        Profile profile;
        const ValidationResult v = parseProfile(params.value("profile"), catalog, &profile);
        if (!v.ok())
            return fail(id, RpcError::InvalidProfile, QStringLiteral("профиль не прошёл проверку"),
                        QJsonObject{{"errors", issuesToJson(v.errors)}, {"warnings", issuesToJson(v.warnings)}});
        const auto r = m_controller->run(profile, start.toString());
        return toReply(r, QJsonObject{{"runId", r.runId}});
    }

    const QJsonValue runId = params.value("runId");
    if (!runId.isString())
        return fail(id, RpcError::InvalidParams, QStringLiteral("нужен params.runId"));
    const QJsonObject done{{"ok", true}};

    if (method == u"sim.goto") {
        const QJsonValue phaseId = params.value("phaseId");
        if (!phaseId.isString())
            return fail(id, RpcError::InvalidParams, QStringLiteral("нужен params.phaseId"));
        return toReply(m_controller->gotoPhase(runId.toString(), phaseId.toString()), done);
    }
    if (method == u"sim.pause")
        return toReply(m_controller->pause(runId.toString()), done);
    if (method == u"sim.resume")
        return toReply(m_controller->resume(runId.toString()), done);
    return toReply(m_controller->stop(runId.toString()), done);
}

} // namespace sim
