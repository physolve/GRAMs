#include "ControllerInfo.h"

ControllerInfo::ControllerInfo(QString deviceName):
m_deviceName(deviceName)
{}
QString ControllerInfo::deviceName(){return m_deviceName;}


// void ControllerInfo::setNames(const QStringList& sensorNames){
// 	m_sensorNames = sensorNames;
// }
// QStringList ControllerInfo::getNames(){
// 	return m_sensorNames;
// }

AdvAIType::AdvAIType(QString deviceName): ControllerInfo (deviceName){

}
QVariantMap AdvAIType::getSettings(){
	QVariantMap settingPressure;
    settingPressure["channelCount"] = m_channelCount;
	settingPressure["channelStart"] = m_channelStart;
	settingPressure["valueRanges"] = m_valueRanges;
	settingPressure["profilePath"] = m_profilePath;
	return settingPressure;
}

void AdvAIType::setSettings(const QVariantMap& info){
	m_channelCount = info["indexChannelCount"].toInt();
	m_channelStart = info["indexChannelStart"].toInt();
	//m_valueRanges = info["valueRanges"].toStringList();
	m_valueRangeCh = info["indexValueRange"].toInt();
	m_profilePath = info["inProfilePath"].toString();
}

AdvDOType::AdvDOType(QString deviceName): ControllerInfo (deviceName){
}

void AdvDOType::setSettings(const QVariantMap& info){
	m_profilePath = info["profilePath"].toString();
}
