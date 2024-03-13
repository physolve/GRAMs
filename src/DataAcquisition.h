#pragma once

#include "controllers/AdvantechCtrl.h"

enum ControllerConnection{
    Offline,
    Online,
    Pending
};

class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
    //QVariantMap profileJson ();
    void processEvents(); // wierdly written
    QMap<QString,QVector<double>> getMeasures();
    QVector<bool> getValves();
    void setValves(const QVector<bool> &states);
    bool getGRAMsIntegrity();
    Q_INVOKABLE void advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings);
    Q_INVOKABLE void testRead();
private:
    QMap<QString,QSharedPointer<AdvantechCtrl>> m_controllerList; // for read
    QMap<QString, ControllerConnection> GRAMsIntegrity;
};