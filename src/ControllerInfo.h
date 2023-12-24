#pragma once

#include <QString>
#include <QVariant>
#include <QMetaType>
class ControllerInfo{
public:
    ControllerInfo() = default;
    ControllerInfo(QString deviceName);
    QString deviceName();
    //QStringList getNames();
    //void setChannelId(const QStringList& sensorNames);
private:
    QString m_deviceName;
};

class ControllerPrType : public ControllerInfo {
public:
    ControllerPrType() = default;
    ControllerPrType(QString deviceName);
    QString deviceName();
    QVariantMap getSettings();
    void setSettings(const QVariantMap& info);
    int m_channelCount; // share to qml
	int m_channelStart;
private:
    QString m_profilePath;
	QStringList m_valueRanges;
    int m_channelCountCh;
	int m_channelStartCh;
    int m_valueRangeCh;
    
    QStringList m_sensorNames;
};

Q_DECLARE_METATYPE(ControllerInfo)