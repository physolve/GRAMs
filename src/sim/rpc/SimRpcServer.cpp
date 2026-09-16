#include "SimRpcServer.h"

#include "SimRpcDispatcher.h"

#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QTcpServer>

namespace sim {

SimRpcServer::SimRpcServer(const SimRpcDispatcher *dispatcher, QObject *parent)
    : QObject(parent), m_dispatcher(dispatcher)
{
    m_http.route(QStringLiteral("/rpc"), QHttpServerRequest::Method::Post, this,
                 [this](const QHttpServerRequest &request) {
                     const QByteArrayView name(kTokenHeader);
                     std::optional<QByteArray> token;
                     if (request.headers().contains(name))
                         token = request.headers().value(name).toByteArray();
                     const QJsonObject reply = m_dispatcher->handle(request.body(), token);
                     return QHttpServerResponse(QByteArrayLiteral("application/json"),
                                                QJsonDocument(reply).toJson(QJsonDocument::Compact));
                 });
}

bool SimRpcServer::listen(quint16 port)
{
    auto *tcp = new QTcpServer(this);
    if (!tcp->listen(QHostAddress::LocalHost, port)) {
        m_error = tcp->errorString();
        delete tcp;
        return false;
    }
    if (!m_http.bind(tcp)) {
        m_error = QStringLiteral("QHttpServer::bind не удался");
        delete tcp;
        return false;
    }
    m_tcp = tcp;
    return true;
}

quint16 SimRpcServer::port() const
{
    return m_tcp ? m_tcp->serverPort() : 0;
}

} // namespace sim
