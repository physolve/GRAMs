#pragma once

// HTTP-транспорт JSON-RPC: POST /rpc на 127.0.0.1, один запрос — один ответ.

#include <QHttpServer>
#include <QObject>
#include <QPointer>

class QTcpServer;

namespace sim {

class SimRpcDispatcher;

class SimRpcServer : public QObject {
    Q_OBJECT
public:
    inline static constexpr quint16 kDefaultPort = 8770;
    inline static constexpr auto kTokenHeader = "X-Grams-Sim-Token";

    explicit SimRpcServer(const SimRpcDispatcher *dispatcher, QObject *parent = nullptr);

    // Слушает только петлевой интерфейс. port = 0 — выбрать свободный.
    bool listen(quint16 port = kDefaultPort);
    quint16 port() const;
    QString errorString() const { return m_error; }

private:
    const SimRpcDispatcher *m_dispatcher;
    QHttpServer m_http;
    QPointer<QTcpServer> m_tcp;
    QString m_error;
};

} // namespace sim
