#include "ControllerInfo.h"

ControllerInfo::ControllerInfo(QString deviceName):
m_deviceName(deviceName),m_channelCountCh(0), m_channelStartCh(0), m_valueRangeCh(0)
{}
QString ControllerInfo::deviceName(){return m_deviceName;}

QVariantMap ControllerInfo::getSettings(){
	QVariantMap settingPressure;
    settingPressure["channelCount"] = m_channelCount;
	settingPressure["channelStart"] = m_channelStart;
	settingPressure["valueRanges"] = m_valueRanges;
	settingPressure["profilePath"] = m_profilePath;
    settingPressure["chChannelCount"] = m_channelCountCh;
	settingPressure["chCannelStart"] = m_channelStartCh;
	settingPressure["chValueRange"] = m_valueRangeCh;
	return settingPressure;
}

void ControllerInfo::setSettings(const QVariantMap& info){
	m_channelCount = info["channelCount"].toInt();
	m_channelStart = info["channelStart"].toInt();
	m_valueRanges = info["valueRanges"].toStringList();
	m_profilePath = info["profilePath"].toString();
    m_channelCountCh = info["chChannelCount"].toInt();
	m_channelStartCh = info["chCannelStart"].toInt();
	m_valueRangeCh = info["chValueRange"].toInt();
}