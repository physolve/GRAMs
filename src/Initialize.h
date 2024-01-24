#pragma once

#include <QVariant>

class Initialize : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap advantechDeviceMap MEMBER m_advantechDeviceMap NOTIFY advantechDeviceMapChanged)
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
    Q_PROPERTY(QVariantMap deviceSettings MEMBER m_deviceSettings NOTIFY deviceSettingsChanged)

    //Q_INVOKABLE void advModuleCheck();

public:
    Initialize(QObject *parent = 0);
signals:
    void advantechDeviceMapChanged();
    void profileNamesChanged();
    void deviceSettingsChanged();
private:
    bool readProfile(QString &rawData);
    bool jsonParser(QString &rawData);
    bool advantechDeviceCheck();
    bool advantechDeviceFill();

    QVariantMap  m_advantechDeviceMap;

    QVariantMap  m_profileJson;

    QStringList m_profileNames;

    QVariantMap m_deviceSettings;

};