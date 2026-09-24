#pragma once

// Параметры запуска демо-режима: --sim, --sim-rpc-port, --sim-token
// и переменные окружения GRAMS_SIM, GRAMS_SIM_RPC_PORT, GRAMS_SIM_TOKEN.
// Флаги командной строки важнее окружения.

#include <QByteArray>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace sim {

struct SimOptions {
    bool enabled = false;
    quint16 rpcPort = 8770;
    QByteArray token;
    QString error;   // непустая — ошибка разбора; демо-режим не включается

    static SimOptions parse(const QStringList &args,
                            const QProcessEnvironment &env = QProcessEnvironment::systemEnvironment());
};

} // namespace sim
