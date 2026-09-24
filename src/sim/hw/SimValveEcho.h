#pragma once

// Демо-порт клапанов: записанная маска становится прочитанной (как регистр-
// защёлка платы), каждый изменившийся бит — фронт клапана для машины фаз.

#include "controllers/IDoPort.h"

#include <QObject>
#include <QPointer>
#include <QStringList>

namespace sim {

class SimController;

class SimValveEcho final : public QObject, public IDoPort {
    Q_OBJECT
public:
    // valveIds — id каталога (K176…) в порядке битов valveMap.
    SimValveEcho(QStringList valveIds, SimController *controller, QObject *parent = nullptr);

    bool write(const QVector<bool> &states) override;
    bool refresh() override { return true; }
    QVector<bool> data() override { return m_states; }

signals:
    void valveChanged(const QString &valveId, bool open);

private:
    QStringList m_valveIds;
    QPointer<SimController> m_controller;
    QVector<bool> m_states;
};

} // namespace sim
