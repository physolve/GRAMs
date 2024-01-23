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

private:
    QList<AdvantechTest*> controllerList; // for read
    QList<AdvantechDO*> controllerDO; // for read
};