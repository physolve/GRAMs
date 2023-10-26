#pragma once

#include <QString>
#include <QVariant>
#include <QMetaType>

class ControllerInfo{
public:
    ControllerInfo() = default;
    ControllerInfo(QString deviceName);
    QString deviceName();
    QVariantMap getSettings();
    void setSettings(const QVariantMap& info);
    int m_channelCount; // share to qml
	int m_channelStart;
    QString m_profilePath;
	QStringList m_valueRanges;
    int m_channelCountCh;
	int m_channelStartCh;
    int m_valueRangeCh;
    private:
    QString m_deviceName;
};

Q_DECLARE_METATYPE(ControllerInfo)