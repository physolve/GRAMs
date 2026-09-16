#include "test_support.h"

#include "core/SimController.h"
#include "core/SimErrors.h"
#include "rpc/SimRpcDispatcher.h"
#include "rpc/SimRpcServer.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QTimer>

using namespace sim;
using namespace simtest;

namespace {

struct HttpReply {
    int status = 0;
    QByteArray body;
    QJsonObject json;
};

// Настоящий HTTP-клиент на QTcpSocket. Сервер живёт в том же потоке,
// поэтому ждём через цикл событий, а не waitForReadyRead.
HttpReply httpRequest(quint16 port, const QByteArray &method, const QByteArray &body,
                      const std::optional<QByteArray> &token = std::nullopt, const QByteArray &path = "/rpc")
{
    QTcpSocket socket;
    QByteArray received;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, &loop, [&] {
        received += socket.readAll();
        const qsizetype headerEnd = received.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;
        const QByteArray headers = received.left(headerEnd).toLower();
        const qsizetype cl = headers.indexOf("content-length:");
        if (cl < 0)
            return;
        const qsizetype eol = headers.indexOf("\r\n", cl);
        const int length = headers.mid(cl + 15, eol < 0 ? -1 : eol - cl - 15).trimmed().toInt();
        if (received.size() >= headerEnd + 4 + length)
            loop.quit();
    });
    QObject::connect(&socket, &QTcpSocket::connected, &loop, [&] {
        QByteArray req = method + ' ' + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                         "Content-Type: application/json\r\nConnection: close\r\n";
        if (token)
            req += QByteArray(SimRpcServer::kTokenHeader) + ": " + *token + "\r\n";
        req += "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
        socket.write(req);
    });
    socket.connectToHost(QHostAddress::LocalHost, port);
    timeout.start(5000);
    loop.exec();

    HttpReply reply;
    const qsizetype headerEnd = received.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        ADD_FAILURE() << "нет ответа HTTP";
        return reply;
    }
    reply.status = received.mid(9, 3).toInt();   // "HTTP/1.1 200"
    reply.body = received.mid(headerEnd + 4);
    reply.json = QJsonDocument::fromJson(reply.body).object();
    return reply;
}

void settle()
{
    for (int i = 0; i < 20; ++i)
        QCoreApplication::processEvents();
}

class SimRpc : public ::testing::Test {
protected:
    ManualClock clock;
    SimController ctl{gram50().catalog, &clock};
    std::unique_ptr<SimRpcDispatcher> dispatcher;
    std::unique_ptr<SimRpcServer> server;

    void start(RpcConfig cfg)
    {
        server.reset();
        dispatcher = std::make_unique<SimRpcDispatcher>(&ctl, std::move(cfg));
        server = std::make_unique<SimRpcServer>(dispatcher.get());
        ASSERT_TRUE(server->listen(0)) << server->errorString().toStdString();
        ASSERT_NE(server->port(), 0);
    }

    void SetUp() override
    {
        RpcConfig cfg;
        cfg.simAllowed = true;
        cfg.appVersion = "1.0.0";
        cfg.regime = [] { return QJsonValue(QJsonObject{{"name", "Вакуум"}, {"step", "11.5"}}); };
        start(cfg);
    }

    QJsonObject call(const QString &method, const QJsonObject &params = {},
                     const std::optional<QByteArray> &token = std::nullopt)
    {
        QJsonObject req{{"jsonrpc", "2.0"}, {"id", 7}, {"method", method}};
        if (!params.isEmpty())
            req.insert("params", params);
        const HttpReply r = httpRequest(server->port(), "POST", QJsonDocument(req).toJson(), token);
        EXPECT_EQ(r.status, 200);
        settle();
        return r.json;
    }

    static int errorCode(const QJsonObject &reply)
    {
        return reply.value("error").toObject().value("code").toInt();
    }

    static QJsonObject profileJson(const char *relative)
    {
        return readJson(profilePath(QString::fromLatin1(relative))).object();
    }
};

} // namespace

TEST_F(SimRpc, HelloAllowed)
{
    const QJsonObject r = call("sim.hello");
    EXPECT_EQ(r.value("jsonrpc").toString(), "2.0");
    EXPECT_EQ(r.value("id").toInt(), 7);
    const QJsonObject res = r.value("result").toObject();
    EXPECT_EQ(res.value("api").toString(), "grams.sim/1");
    EXPECT_EQ(res.value("appVersion").toString(), "1.0.0");
    EXPECT_TRUE(res.value("simEnabled").toBool());
    EXPECT_TRUE(res.value("simAllowed").toBool());
    EXPECT_TRUE(res.value("reason").isNull());
}

TEST_F(SimRpc, NotAllowedHelloAndControlMethods)
{
    RpcConfig cfg;
    cfg.simAllowed = false;
    cfg.notAllowedReason = "подключено железо: USB-4716";
    start(cfg);
    const QJsonObject hello = call("sim.hello").value("result").toObject();
    EXPECT_FALSE(hello.value("simAllowed").toBool());
    EXPECT_EQ(hello.value("reason").toString(), "подключено железо: USB-4716");

    const QJsonObject run = call("sim.run", {{"profile", profileJson("minimal_valid.json")}});
    EXPECT_EQ(errorCode(run), RpcError::SimNotAllowed);
    EXPECT_EQ(run.value("error").toObject().value("data").toObject().value("reason").toString(),
              "подключено железо: USB-4716");
    EXPECT_EQ(errorCode(call("sim.pause", {{"runId", "x"}})), RpcError::SimNotAllowed);
    EXPECT_EQ(ctl.machine(), SimController::Machine::Idle);
    // Чтение по-прежнему доступно.
    EXPECT_TRUE(call("sim.status").contains("result"));
    EXPECT_TRUE(call("sim.catalog").contains("result"));
}

TEST_F(SimRpc, Catalog)
{
    const QJsonObject res = call("sim.catalog").value("result").toObject();
    const QJsonArray channels = res.value("channels").toArray();
    ASSERT_EQ(channels.size(), 18);
    const QJsonObject first = channels.first().toObject();
    EXPECT_EQ(first.value("id").toString(), "P.DD311");
    EXPECT_EQ(first.value("unit").toString(), "Pa");
    EXPECT_EQ(first.value("scale").toString(), "linear");
    EXPECT_TRUE(first.contains("value"));
    EXPECT_EQ(res.value("valves").toArray().size(), 16);
    EXPECT_EQ(res.value("zones").toArray().size(), 4);
    EXPECT_EQ(res.value("kinds").toArray().size(), 4);
}

TEST_F(SimRpc, Validate)
{
    QJsonObject res = call("sim.validate", {{"profile", profileJson("minimal_valid.json")}}).value("result").toObject();
    EXPECT_TRUE(res.value("ok").toBool());
    EXPECT_TRUE(res.value("errors").toArray().isEmpty());

    res = call("sim.validate", {{"profile", profileJson("invalid/exp_no_tau.json")}}).value("result").toObject();
    EXPECT_FALSE(res.value("ok").toBool());
    EXPECT_EQ(res.value("errors").toArray().first().toObject().value("code").toString(), "EXP_NO_TAU");

    EXPECT_EQ(errorCode(call("sim.validate", {{"other", 1}})), RpcError::InvalidParams);
}

TEST_F(SimRpc, RunLifecycle)
{
    const QJsonObject profile = profileJson("minimal_valid.json");

    QJsonObject r = call("sim.run", {{"profile", profileJson("invalid/bad_kind.json")}});
    EXPECT_EQ(errorCode(r), RpcError::InvalidProfile);
    EXPECT_EQ(r.value("error").toObject().value("data").toObject().value("errors").toArray()
                  .first().toObject().value("code").toString(), "BAD_KIND");

    EXPECT_EQ(errorCode(call("sim.run", {{"profile", profile}, {"startPhaseId", "zzz"}})), RpcError::UnknownPhase);

    r = call("sim.run", {{"profile", profile}});
    const QString runId = r.value("result").toObject().value("runId").toString();
    ASSERT_FALSE(runId.isEmpty()) << QJsonDocument(r).toJson().toStdString();
    EXPECT_EQ(errorCode(call("sim.run", {{"profile", profile}})), RpcError::Busy);

    QJsonObject status = call("sim.status").value("result").toObject();
    EXPECT_EQ(status.value("machine").toString(), "active");
    EXPECT_EQ(status.value("runId").toString(), runId);
    EXPECT_EQ(status.value("phase").toObject().value("id").toString(), "a");
    EXPECT_EQ(status.value("regime").toObject().value("name").toString(), "Вакуум");

    EXPECT_EQ(errorCode(call("sim.pause", {{"runId", "wrong"}})), RpcError::NoActiveRun);
    EXPECT_EQ(errorCode(call("sim.goto", {{"runId", runId}, {"phaseId", "zzz"}})), RpcError::UnknownPhase);
    EXPECT_EQ(errorCode(call("sim.goto", {{"runId", runId}})), RpcError::InvalidParams);
    EXPECT_EQ(errorCode(call("sim.stop", {})), RpcError::InvalidParams);

    EXPECT_TRUE(call("sim.pause", {{"runId", runId}}).value("result").toObject().value("ok").toBool());
    EXPECT_EQ(call("sim.status").value("result").toObject().value("machine").toString(), "paused");
    EXPECT_TRUE(call("sim.resume", {{"runId", runId}}).value("result").toObject().value("ok").toBool());
    EXPECT_EQ(call("sim.status").value("result").toObject().value("machine").toString(), "active");
    EXPECT_TRUE(call("sim.goto", {{"runId", runId}, {"phaseId", "b"}}).value("result").toObject().value("ok").toBool());
    status = call("sim.status").value("result").toObject();
    EXPECT_EQ(status.value("phase").toObject().value("id").toString(), "b");
    EXPECT_EQ(status.value("history").toArray().last().toObject().value("reason").toString(), "manual");

    EXPECT_TRUE(call("sim.stop", {{"runId", runId}}).value("result").toObject().value("ok").toBool());
    status = call("sim.status").value("result").toObject();
    EXPECT_EQ(status.value("machine").toString(), "idle");
    EXPECT_TRUE(status.value("runId").isNull());
    EXPECT_EQ(errorCode(call("sim.resume", {{"runId", runId}})), RpcError::NoActiveRun);
}

TEST_F(SimRpc, StandardErrors)
{
    const quint16 port = server->port();
    HttpReply r = httpRequest(port, "POST", "{not json");
    EXPECT_EQ(errorCode(r.json), RpcError::ParseError);
    EXPECT_TRUE(r.json.value("id").isNull());

    r = httpRequest(port, "POST", R"([{"jsonrpc":"2.0","id":1,"method":"sim.hello"}])");
    EXPECT_EQ(errorCode(r.json), RpcError::InvalidRequest);

    r = httpRequest(port, "POST", R"({"id":1,"method":"sim.hello"})");
    EXPECT_EQ(errorCode(r.json), RpcError::InvalidRequest);
    EXPECT_EQ(r.json.value("id").toInt(), 1);

    r = httpRequest(port, "POST", R"({"jsonrpc":"2.0","id":"abc","method":"sim.nope"})");
    EXPECT_EQ(errorCode(r.json), RpcError::MethodNotFound);
    EXPECT_EQ(r.json.value("id").toString(), "abc");

    r = httpRequest(port, "POST", R"({"jsonrpc":"2.0","id":2,"method":"sim.hello","params":[1]})");
    EXPECT_EQ(errorCode(r.json), RpcError::InvalidParams);

    EXPECT_EQ(errorCode(call("sim.run", {{"startPhaseId", "a"}})), RpcError::InvalidParams);
}

TEST_F(SimRpc, TokenRequiredWhenConfigured)
{
    RpcConfig cfg;
    cfg.simAllowed = true;
    cfg.token = "s3cret";
    start(cfg);
    EXPECT_EQ(errorCode(call("sim.hello")), RpcError::Unauthorized);
    EXPECT_EQ(errorCode(call("sim.hello", {}, QByteArray("wrong"))), RpcError::Unauthorized);
    EXPECT_EQ(errorCode(call("sim.status", {}, QByteArray("wrong"))), RpcError::Unauthorized);
    EXPECT_TRUE(call("sim.hello", {}, QByteArray("s3cret")).contains("result"));
}

TEST_F(SimRpc, TokenIgnoredWhenNotConfigured)
{
    EXPECT_TRUE(call("sim.hello", {}, QByteArray("anything")).contains("result"));
}

TEST_F(SimRpc, OnlyPostRpcIsServed)
{
    EXPECT_NE(httpRequest(server->port(), "GET", {}).status, 200);
    EXPECT_NE(httpRequest(server->port(), "POST", "{}", std::nullopt, "/other").status, 200);
}

TEST_F(SimRpc, ListensOnLoopbackOnly)
{
    SimRpcServer other(dispatcher.get());
    ASSERT_TRUE(other.listen(0));
    // На петлевом адресе порт открыт, на внешнем IPv4 (если он есть) — нет.
    QTcpSocket probe;
    probe.connectToHost(QHostAddress::LocalHost, other.port());
    EXPECT_TRUE(probe.waitForConnected(2000));
    probe.abort();
    const QList<QHostAddress> local = QNetworkInterface::allAddresses();
    for (const QHostAddress &addr : local) {
        if (addr.isLoopback() || addr.protocol() != QAbstractSocket::IPv4Protocol)
            continue;
        QTcpSocket ext;
        ext.connectToHost(addr, other.port());
        EXPECT_FALSE(ext.waitForConnected(500)) << addr.toString().toStdString();
        break;
    }
}
