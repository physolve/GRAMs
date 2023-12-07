#pragma once

#include "Controller.h"

class DataAcquisition : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap connectedDevices MEMBER m_connectedDevices NOTIFY profileNamesChanged)
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
    Q_PROPERTY(QVariantMap deviceSettings MEMBER m_deviceSettings NOTIFY deviceSettingsChanged)
public:
    explicit DataAcquisition(QObject *parent = 0);
    QVariantMap profileJson () const;

    const QList<ControllerInfo> getControllersInfo();
    void processEvents();
    const QList<QVector<double>> getDataList();
Q_SIGNALS:

public slots:
    void saveStartDevice(); // no longer slot
    //void choosenGRAMInitialization();
    void setDeviceParameters(QString name, QVariantMap obj);
    //void setChannelMapping(QVariantMap obj);
signals:
    void profileNamesChanged();
    void deviceSettingsChanged();
private:
    //QList<QList<QPointF>> m_data; // unused
    
    QVariantMap  m_connectedDevices;

    QVariantMap  m_profileJson;

    QStringList m_profileNames;

    bool advantechDeviceCheck(QVariantMap& advantechDeviceMap) const;
    QList<ControllerInfo> m_deviceInfoList;
    QVariantMap m_deviceSettings;

    //QVariantMap m_sensorMapping;

    QList<AdvantechTest*> controllerList; // for read
    // Controller for Write!
    //void appendToControllerList(AdvantechTest& device);

    void readDataFromDevice(ControllerInfo info);
};