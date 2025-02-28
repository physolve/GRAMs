#include "ControllerInfo.h"

ControllerInfo::ControllerInfo(QString deviceName):
m_deviceName(deviceName)
{}
//QString ControllerInfo::deviceName(){return m_deviceName;}


// void ControllerInfo::setNames(const QStringList& sensorNames){
// 	m_sensorNames = sensorNames;
// }
// QStringList ControllerInfo::getNames(){
// 	return m_sensorNames;
// }

AdvAIType::AdvAIType(QString deviceName): ControllerInfo (deviceName){

}
QString AdvAIType::advDescription() const{
	return m_deviceName;
}
void AdvAIType::setProfilePath(const QString &profilePath){
	m_profilePath = profilePath;
}
QString AdvAIType::profilePath() const{
	return m_profilePath;
}

void AdvAIType::setChannelStart(const int &channelStart){
	m_channelStart = channelStart;
}

int AdvAIType::channelStart() const{
	return m_channelStart;
}

void AdvAIType::setChannelCount(const int &channelCount){
	m_channelCount = channelCount;
}

int AdvAIType::channelCount() const{
	return m_channelCount;
}

void AdvAIType::setDefaultType(const int &defaultType){
	m_defaultType = defaultType;
}

int AdvAIType::defaultType() const{
	return m_defaultType;
}

void AdvAIType::appendToValueRange(const QString &valueRange){ // replace to set?
	m_valueRanges << valueRange;
}

QStringList AdvAIType::getValueRanges() const{
	return m_valueRanges;
}

// QVariantMap AdvAIType::getSettings(){
// 	QVariantMap settingPressure;
//     settingPressure["channelCount"] = m_channelCount;
// 	settingPressure["channelStart"] = m_channelStart;
// 	settingPressure["valueRanges"] = m_valueRanges;
// 	settingPressure["profilePath"] = m_profilePath;
// 	return settingPressure;
// }

// void AdvAIType::setSettings(const QVariantMap& info){
// 	m_channelCount = info["indexChannelCount"].toInt();
// 	m_channelStart = info["indexChannelStart"].toInt();
// 	//m_valueRanges = info["valueRanges"].toStringList();
// 	m_valueRangeCh = info["indexValueRange"].toInt();
// 	m_profilePath = info["inProfilePath"].toString();
// }

AdvDOType::AdvDOType(const QString & deviceName): ControllerInfo (deviceName){
}

QString AdvDOType::advDescription() const{
	return m_deviceName;
}
void AdvDOType::setProfilePath(const QString &profilePath){
	m_profilePath = profilePath;
}
QString AdvDOType::profilePath() const{
	return m_profilePath;
}