#include "ControllerInfo.h"

ControllerInfo::ControllerInfo(QString deviceName):
m_deviceName(deviceName)
{}
QString ControllerInfo::deviceName(){return m_deviceName;}
