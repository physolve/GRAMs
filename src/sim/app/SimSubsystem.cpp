#include "SimSubsystem.h"

#include "hw/SimSensorSource.h"
#include "hw/SimValveEcho.h"

#include <QDebug>

namespace sim {

SimStatusBridge::SimStatusBridge(SimController *controller, bool allowed, QString reason, QObject *parent)
    : QObject(parent), m_controller(controller), m_allowed(allowed), m_reason(std::move(reason))
{
    connect(controller, &SimController::machineChanged, this, &SimStatusBridge::stateChanged);
    connect(controller, &SimController::phaseChanged, this, &SimStatusBridge::stateChanged);
}

void SimStatusBridge::setRpc(bool listening, int port)
{
    m_rpcListening = listening;
    m_rpcPort = port;
    emit rpcChanged();
}

SimSubsystem::SimSubsystem(const CatalogInput &input, const SimOptions &options, bool allowed,
                           const QString &notAllowedReason, const QString &appVersion, QObject *parent)
    : QObject(parent), m_build(buildCatalog(input)), m_options(options), m_allowed(allowed)
{
    m_controller = std::make_unique<SimController>(m_build.catalog, &m_clock);

    RpcConfig cfg;
    cfg.simAllowed = allowed;
    cfg.notAllowedReason = notAllowedReason;
    cfg.token = options.token;
    cfg.appVersion = appVersion;
    cfg.regime = [this] { return m_regime ? m_regime() : QJsonValue(QJsonValue::Null); };
    m_dispatcher = std::make_unique<SimRpcDispatcher>(m_controller.get(), cfg);
    m_server = std::make_unique<SimRpcServer>(m_dispatcher.get());
    m_bridge = std::make_unique<SimStatusBridge>(m_controller.get(), allowed, notAllowedReason);
}

SimSubsystem::~SimSubsystem() = default;

bool SimSubsystem::startRpc()
{
    const bool ok = m_server->listen(m_options.rpcPort);
    m_bridge->setRpc(ok, ok ? m_server->port() : 0);
    if (ok)
        qInfo() << "Демо-режим: JSON-RPC на 127.0.0.1:" << m_server->port()
                << (m_options.token.isEmpty() ? "(без токена)" : "(с токеном)");
    else
        qWarning() << "Демо-режим: RPC не поднялся на порту" << m_options.rpcPort << "—" << m_server->errorString();
    return ok;
}

std::unique_ptr<ISensorSource> SimSubsystem::createSensorSource()
{
    return std::make_unique<SimSensorSource>(m_controller.get(), m_build.bindings);
}

std::unique_ptr<IDoPort> SimSubsystem::createValvePort(const QStringList &valveCodes)
{
    QStringList ids;
    for (const QString &code : valveCodes) {
        const ValveDesc *v = m_build.catalog.valveByCode(code);
        ids << (v ? v->id : code);
    }
    return std::make_unique<SimValveEcho>(ids, m_controller.get());
}

} // namespace sim
