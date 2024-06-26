#pragma once

#include <QVariant>

class Initialize : public QObject
{
    Q_OBJECT
public:
    Initialize(QObject *parent = 0);
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
    Q_PROPERTY(QStringList advantechDeviceMap MEMBER m_advantechDeviceMap NOTIFY advantechDeviceMapChanged)
    Q_INVOKABLE QVariantMap advantechDeviceFill(const QString &description, const QString &type);
    
signals:
    void advantechDeviceMapChanged();
    //void advantechDeviceSettingsChanged();
    
private:
    bool readProfile(QString &rawData);
    bool jsonParser(QString &rawData);
    bool advantechDeviceCheck();

    QVariantMap m_profileJson;

    QStringList m_profileNames;

    QStringList m_advantechDeviceMap;

    //QVariantMap m_advantechDeviceSettings;
};