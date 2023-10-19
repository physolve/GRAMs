#pragma once

#include <QString>
#include <QVariant>
#include <QMetaType>

class ControllerInfo{
public:
    ControllerInfo() = default;
    ControllerInfo(QString deviceName);
    QString deviceName();
    private:
    QString m_deviceName;
    
};

Q_DECLARE_METATYPE(ControllerInfo)