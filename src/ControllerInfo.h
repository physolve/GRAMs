#pragma once

//#include <QString>
#include <QVariant>
#include <QMetaType>
class ControllerInfo{
public:
    ControllerInfo() = default;
    ControllerInfo(QString deviceName);
    QString deviceName();
    // vitual for get settings?
private:
    QString m_deviceName;
};

class AdvAIType: public ControllerInfo {
public:
    AdvAIType() = default;
    AdvAIType(QString deviceName);
    QVariantMap getSettings();
    void setSettings(const QVariantMap& info);
    int m_channelCount; // share to qml
	int m_channelStart;
	QStringList m_valueRanges;
    QString m_profilePath;
    int m_channelCountCh;
	int m_channelStartCh;
    int m_valueRangeCh;
// private:
//     QStringList m_sensorNames;
};

class AdvDOType: public ControllerInfo {
public:
    AdvDOType() = default;
    AdvDOType(QString deviceName);
    QString m_profilePath;
};

Q_DECLARE_METATYPE(ControllerInfo)