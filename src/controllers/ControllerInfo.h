#pragma once

//#include <QString>
#include <QVariant>
// #include <QMetaType>

// this is fort different devices pasiing stuct of settings, from UI basically
class ControllerInfo{
public:
    ControllerInfo() = default;
    ControllerInfo(QString deviceName);
    QString deviceName() const;
    // vitual for get settings?
protected:
    QString m_deviceName;
};

class AdvAIType: public ControllerInfo {
public:
    AdvAIType() = default;
    AdvAIType(QString deviceName);
    QString advDescription() const;
    void setProfilePath(const QString &profilePath);
    void setChannelCount(const int& channelCount);
    void setChannelStart(const int& channelStart);
    void appendToValueRange(const QString &valueRange);
    void setDefaultType(const int& defaultType);
    QString profilePath() const;
    int channelCount() const;
    int channelStart() const;
    QStringList getValueRanges() const;
    int defaultType() const;
private:
    int m_channelCount; // share to qml
	int m_channelStart;
	QStringList m_valueRanges;
    QString m_profilePath;
    int m_defaultType;
};

class AdvDOType: public ControllerInfo {
public:
    AdvDOType() = default;
    AdvDOType(const QString &deviceName); // device name = description
    QString advDescription() const;
    void setProfilePath(const QString &profilePath);
    QString profilePath() const;
private:
    QString m_profilePath;
    // additional parameters
};

// Q_DECLARE_METATYPE(ControllerInfo)