#pragma once

// JSON-RPC 2.0 контракта grams.sim/1 (разделы 2.1–2.3) без транспорта:
// тело HTTP-запроса и заголовок токена на входе, JSON-ответ на выходе.

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <functional>
#include <optional>

namespace sim {

class SimController;

struct RpcConfig {
    bool simAllowed = false;
    QString notAllowedReason;               // для sim.hello и SIM_NOT_ALLOWED
    QByteArray token;                       // пусто — заголовок не проверяется
    QString appVersion;
    std::function<QJsonValue()> regime;     // {name, step} | null
};

class SimRpcDispatcher {
public:
    SimRpcDispatcher(SimController *controller, RpcConfig config);

    // token — значение X-Grams-Sim-Token, std::nullopt — заголовка нет.
    QJsonObject handle(const QByteArray &body, const std::optional<QByteArray> &token) const;

    const RpcConfig &config() const { return m_config; }

private:
    QJsonObject dispatch(const QString &method, const QJsonObject &params, const QJsonValue &id) const;

    SimController *m_controller;
    RpcConfig m_config;
};

} // namespace sim
