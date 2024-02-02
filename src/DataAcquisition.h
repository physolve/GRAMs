#pragma once

#include "Controller.h"

class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
    QVariantMap profileJson () const;
    void processEvents(); // wierdly written
    const QList<QVector<double>> getDataList();
    Q_INVOKABLE void advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings);
private:
    QList<AdvantechTest*> controllerList; // for read
    QList<AdvantechDO*> controllerDO; // for read | only one is enough?
};