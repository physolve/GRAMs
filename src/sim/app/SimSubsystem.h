#pragma once

// Демо-режим целиком: каталог, машина фаз, RPC и мост для QML.
// Создаётся Grams только при --sim; без флага ни этот объект, ни порт не
// появляются. Если подключено железо — RPC поднимается, чтобы ответить
// SIM_NOT_ALLOWED, но источник данных и порт клапанов не подменяются.

#include "SimOptions.h"
#include "core/SimCatalog.h"
#include "core/SimClock.h"
#include "core/SimController.h"
#include "rpc/SimRpcDispatcher.h"
#include "rpc/SimRpcServer.h"

#include <QObject>

#include <functional>
#include <memory>

class ISensorSource;
class IDoPort;

namespace sim {

// Свойства для плашки «ДЕМО-ДАННЫЕ» (QML-синглтон SimStatus).
class SimStatusBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool allowed READ allowed CONSTANT)
    Q_PROPERTY(QString notAllowedReason READ notAllowedReason CONSTANT)
    Q_PROPERTY(int rpcPort READ rpcPort NOTIFY rpcChanged)
    Q_PROPERTY(bool rpcListening READ rpcListening NOTIFY rpcChanged)
    Q_PROPERTY(bool demoActive READ demoActive NOTIFY stateChanged)
    Q_PROPERTY(QString machine READ machine NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(QString profileName READ profileName NOTIFY stateChanged)
    Q_PROPERTY(QString phaseId READ phaseId NOTIFY stateChanged)
    Q_PROPERTY(QString phaseLabel READ phaseLabel NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY stateChanged)
public:
    SimStatusBridge(SimController *controller, bool allowed, QString reason, QObject *parent = nullptr);

    bool allowed() const { return m_allowed; }
    QString notAllowedReason() const { return m_reason; }
    int rpcPort() const { return m_rpcPort; }
    bool rpcListening() const { return m_rpcListening; }
    bool demoActive() const { return m_controller->isDemoActive(); }
    QString machine() const { return SimController::toWire(m_controller->machine()); }
    bool paused() const { return m_controller->machine() == SimController::Machine::Paused; }
    QString profileName() const { return demoActive() ? m_controller->profileName() : QString(); }
    QString phaseId() const { return m_controller->currentPhaseId(); }
    QString phaseLabel() const { return m_controller->currentPhaseLabel(); }
    QString lastError() const { return m_controller->lastError(); }

    void setRpc(bool listening, int port);

signals:
    void stateChanged();
    void rpcChanged();

private:
    SimController *m_controller;
    bool m_allowed;
    QString m_reason;
    int m_rpcPort = 0;
    bool m_rpcListening = false;
};

class SimSubsystem : public QObject {
    Q_OBJECT
public:
    SimSubsystem(const CatalogInput &input, const SimOptions &options, bool allowed,
                 const QString &notAllowedReason, const QString &appVersion, QObject *parent = nullptr);
    ~SimSubsystem() override;

    bool allowed() const { return m_allowed; }
    SimController *controller() { return m_controller.get(); }
    SimStatusBridge *bridge() { return m_bridge.get(); }
    const Catalog &catalog() const { return m_build.catalog; }

    bool startRpc();
    QString rpcError() const { return m_server->errorString(); }

    // {name, step} текущего режима — только чтение, для sim.status.
    void setRegimeProvider(std::function<QJsonValue()> provider) { m_regime = std::move(provider); }

    std::unique_ptr<ISensorSource> createSensorSource();
    // valveCodes — имена клапанов (AR1, SL2…) в порядке битов DO.
    std::unique_ptr<IDoPort> createValvePort(const QStringList &valveCodes);

private:
    CatalogBuild m_build;
    SimOptions m_options;
    bool m_allowed;
    SteadyClock m_clock;
    std::function<QJsonValue()> m_regime;
    std::unique_ptr<SimController> m_controller;
    std::unique_ptr<SimRpcDispatcher> m_dispatcher;
    std::unique_ptr<SimRpcServer> m_server;
    std::unique_ptr<SimStatusBridge> m_bridge;
};

} // namespace sim
