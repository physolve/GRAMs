#include "SimOptions.h"

namespace sim {

namespace {

bool parsePort(const QString &text, quint16 *port)
{
    bool ok = false;
    const uint value = text.toUInt(&ok);
    if (!ok || value == 0 || value > 65535)
        return false;
    *port = quint16(value);
    return true;
}

bool truthy(const QString &v)
{
    const QString s = v.trimmed().toLower();
    return s == u"1" || s == u"true" || s == u"yes" || s == u"on";
}

} // namespace

SimOptions SimOptions::parse(const QStringList &args, const QProcessEnvironment &env)
{
    SimOptions o;

    if (env.contains("GRAMS_SIM"))
        o.enabled = truthy(env.value("GRAMS_SIM"));
    if (env.contains("GRAMS_SIM_RPC_PORT") && !parsePort(env.value("GRAMS_SIM_RPC_PORT"), &o.rpcPort))
        o.error = QStringLiteral("GRAMS_SIM_RPC_PORT: неверный порт");
    if (env.contains("GRAMS_SIM_TOKEN"))
        o.token = env.value("GRAMS_SIM_TOKEN").toUtf8();

    // Первый элемент — имя программы.
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString &a = args[i];
        auto takeValue = [&](const QString &flag, QString *value) {
            if (a.startsWith(flag + u'=')) {
                *value = a.mid(flag.size() + 1);
                return true;
            }
            if (a == flag) {
                if (i + 1 >= args.size()) {
                    o.error = QStringLiteral("%1: нет значения").arg(flag);
                    return false;
                }
                *value = args[++i];
                return true;
            }
            return false;
        };

        QString value;
        if (a == u"--sim") {
            o.enabled = true;
        } else if (takeValue(QStringLiteral("--sim-rpc-port"), &value)) {
            if (!parsePort(value, &o.rpcPort))
                o.error = QStringLiteral("--sim-rpc-port: неверный порт «%1»").arg(value);
        } else if (takeValue(QStringLiteral("--sim-token"), &value)) {
            o.token = value.toUtf8();
        }
    }

    if (!o.error.isEmpty())
        o.enabled = false;
    return o;
}

} // namespace sim
