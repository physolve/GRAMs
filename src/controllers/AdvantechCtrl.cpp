#include "AdvantechCtrl.h"

#include <QDebug>
#include <QBitArray>

AdvantechCtrl::AdvantechCtrl(const QString &name, QObject *parent) : 
	QObject(parent), m_name(name)
{
}
AdvantechCtrl::~AdvantechCtrl(){
	qDebug() << "Controller deleted";
}
void AdvantechCtrl::Initialization(){
    qDebug() << "Controller initialized";
}
void AdvantechCtrl::readData(){
	qDebug() << "I read data!";
}
/* Advantech Analog Input (pressure and temperature readings) */

AdvantechAI::AdvantechAI(const AdvAIType &info, QObject *parent) :
	AdvantechCtrl(info.m_deviceName,parent), m_info(info), m_instantAiCtrl(NULL), m_vector(8,0.0)
{
}

AdvantechAI::~AdvantechAI(){
    qDebug() << QString("Oh no, %1 was deleted").arg(m_info.m_deviceName);
}

void AdvantechAI::Initialization()
{
    std::wstring description = m_info.m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

    InstantAiCtrl *instantAiCtrl = InstantAiCtrl::Create();
	ErrorCode errorCode = instantAiCtrl->setSelectedDevice(selected);

	if (errorCode != 0){
		QString str;
        QString des = QString::fromStdWString(description);
		return;
	}

	int channelCount = (instantAiCtrl->getChannelCount() < 16) ? 
		instantAiCtrl->getChannelCount() : 16;

	int logicChannelCount = instantAiCtrl->getChannelCount();

	m_info.m_channelStart = logicChannelCount;

	m_info.m_channelCount = channelCount;

	Array<ValueRange>* ValueRanges = instantAiCtrl->getFeatures()->getValueRanges();
	wchar_t		 vrgDescription[128];
	MathInterval ranges;
	for (int i = 0; i < ValueRanges->getCount(); i++)
	{
		errorCode = AdxGetValueRangeInformation(ValueRanges->getItem(i), 
			sizeof(vrgDescription), vrgDescription, &ranges, NULL);
		CheckError(errorCode);
		QString str = QString::fromWCharArray(vrgDescription);
		m_info.m_valueRanges.append(str);
	}

	instantAiCtrl->Dispose();
}

const AdvAIType& AdvantechAI::getInfo(){
	return m_info;
}

void AdvantechAI::ConfigureDeviceTest(){ // after accept

	if (m_instantAiCtrl==NULL)
	{
      m_instantAiCtrl = InstantAiCtrl::Create();
	}

    std::wstring description = m_info.m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

    ErrorCode errorCode = m_instantAiCtrl->setSelectedDevice(selected);
	CheckError(errorCode);
	// add DIR ? profile
    std::wstring profile = m_info.m_profilePath.toStdWString();
    errorCode = m_instantAiCtrl->LoadProfile(profile.c_str());
    CheckError(errorCode);

	//Get channel max number. set value range for every channel.
	Array<AiChannel> *channels = m_instantAiCtrl->getChannels();

	Array<ValueRange>* valueRanges = m_instantAiCtrl->getFeatures()->getValueRanges();
	m_valueRange = valueRanges->getItem(m_info.m_valueRangeCh);
	
	for (int i = 0; i < channels->getCount(); i++)
	{
		channels->getItem(i).setValueRange(m_valueRange);
	}

	qDebug() << "INFO COUNT " << channels->getCount();
	resizeDataVector(m_info.m_channelCount); // ?
}

void AdvantechAI::resizeDataVector(uint8_t size){
	this->m_vector.resize(size);
}

void AdvantechAI::CheckError(ErrorCode errorCode)
{
    if (BioFailed(errorCode))
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		qDebug() << QString("Warning Information %1").arg(message);
	}
}

void AdvantechAI::readData(){
	ErrorCode errorCode = Success;
	//qDebug() << "controller Data count = " << m_vector.count();
	errorCode = m_instantAiCtrl->Read(m_info.m_channelStart, m_info.m_channelCount, m_vector.data());
	CheckError(errorCode);
	if (errorCode != Success)
	{
		return;
	}
}

const QVector<double> AdvantechAI::getData(){ // const & ?
	//vector=scaledData;
	return m_vector;
}

/* Advantech Buffered Analog Input (pressure and temperature readings) */

AdvantechBuff::AdvantechBuff(const AdvAIType &info, QObject *parent) :
	AdvantechCtrl(info.m_deviceName,parent), m_info(info), m_waveformAiCtrl(NULL), m_vector(8,0.0)
{
	m_waveformAiCtrl = WaveformAiCtrl::Create();
	m_waveformAiCtrl->addStoppedHandler(OnStoppedEvent, this);
}

AdvantechBuff::~AdvantechBuff(){
    qDebug() << QString("Oh no, %1 was deleted").arg(m_info.m_deviceName);
	if (m_waveformAiCtrl != NULL)
	{
		m_waveformAiCtrl->Dispose();
	}
}

void AdvantechBuff::Initialization()
{
    std::wstring description = m_info.m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());


    WaveformAiCtrl *waveformAiCtrl = WaveformAiCtrl::Create();
	ErrorCode errorCode = waveformAiCtrl->setSelectedDevice(selected);

	if (errorCode != 0){
		QString str;
        QString des = QString::fromStdWString(description);
		return;
	}

	int channelCount = (waveformAiCtrl->getChannelCount() < 16) ? 
		waveformAiCtrl->getChannelCount() : 16;

	int logicChannelCount = waveformAiCtrl->getChannelCount();

	m_info.m_channelStart = logicChannelCount;

	m_info.m_channelCount = channelCount;

	Array<ValueRange>* ValueRanges = waveformAiCtrl->getFeatures()->getValueRanges();
	wchar_t		 vrgDescription[128];
	MathInterval ranges;
	ValueUnit valueUnit;
	for (int i = 0; i < ValueRanges->getCount(); i++)
	{
		errorCode = AdxGetValueRangeInformation(ValueRanges->getItem(i), 
			sizeof(vrgDescription), vrgDescription, &ranges, &valueUnit);
		CheckError(errorCode);
		QString str = QString::fromWCharArray(vrgDescription);
		m_info.m_valueRanges.append(str);
	}

	waveformAiCtrl->Dispose();
}

const AdvAIType& AdvantechBuff::getInfo(){
	return m_info;
}

void AdvantechBuff::ConfigureDeviceTest(){ // after accept

	// if (m_waveformAiCtrl==NULL)
	// {
    //   m_waveformAiCtrl = WaveformAiCtrl::Create();
	// }

	int32 rawDataBufferLength = m_info.m_channelCount * m_sectionLength;
	// resize kalmanBuffer to rawDataBufferLength?

    std::wstring description = m_info.m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

    ErrorCode errorCode = m_waveformAiCtrl->setSelectedDevice(selected);
	CheckError(errorCode);
	// add DIR ? profile
    std::wstring profile = m_info.m_profilePath.toStdWString();
    //errorCode = m_waveformAiCtrl->LoadProfile(profile.c_str());
    //CheckError(errorCode);

	errorCode = m_waveformAiCtrl->getConversion()->setChannelCount(m_info.m_channelCount);
	CheckError(errorCode);
	errorCode = m_waveformAiCtrl->getConversion()->setChannelStart(m_info.m_channelStart);
	CheckError(errorCode);
	// clockRate > 1 && clockRate < 100000000 
	errorCode = m_waveformAiCtrl->getConversion()->setClockRate(500); //first try 32 kHz
	CheckError(errorCode);
	errorCode = m_waveformAiCtrl->getRecord()->setSectionLength(m_sectionLength);
	CheckError(errorCode);
	errorCode = m_waveformAiCtrl->getRecord()->setSectionCount(1);
	CheckError(errorCode);
	//Get channel max number. set value range for every channel.
	Array<AiChannel> *channels = m_waveformAiCtrl->getChannels();

	Array<ValueRange>* valueRanges = m_waveformAiCtrl->getFeatures()->getValueRanges();
	m_valueRange = valueRanges->getItem(m_info.m_valueRangeCh);
	
	for (int i = 0; i < channels->getCount(); i++)
	{
		channels->getItem(i).setValueRange(m_valueRange);
	}

	errorCode = m_waveformAiCtrl->Prepare();

	qDebug() << "INFO COUNT " << channels->getCount();
	resizeDataVector(m_info.m_channelCount); // ?
	resizeVoltageFilterList(m_info.m_channelCount);
}

void AdvantechBuff::resizeDataVector(uint8_t size){
	this->m_vector.resize(size);
}
void AdvantechBuff::resizeVoltageFilterList(uint8_t size){
	this->m_voltageFilters.resize(size);
}
void AdvantechBuff::CheckError(ErrorCode errorCode)
{
    if (BioFailed(errorCode))
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		qDebug() << QString("Warning Information %1").arg(message);
	}
}

void AdvantechBuff::readData(){
	ErrorCode errorCode = Success;
	//qDebug() << "controller Data count = " << m_vector.count();
	errorCode = m_waveformAiCtrl->Start();
	CheckError(errorCode);
}

void AdvantechBuff::OnStoppedEvent(void *sender, BfdAiEventArgs *args, void *userParam){
	// put this data to kalan filter to improve accuracy
	AdvantechBuff* uParam = (AdvantechBuff *)userParam;
	int32 remainingCount = args->Count;
	int32 getDataCout = 0, returnedCount = 0;
	int32 bufSize = uParam->m_sectionLength * uParam->m_info.m_channelCount;
	QVector<double> kalmanBuffer;
	kalmanBuffer.resize(bufSize);
	do{
		getDataCout = qMin(bufSize, remainingCount);
		((WaveformAiCtrl*)sender)->GetData(args->Count, kalmanBuffer.data(), 0, &returnedCount, NULL, NULL, NULL);
		remainingCount -= returnedCount;
		uParam->setVoltageToFilter(kalmanBuffer);
	} while (remainingCount > 0); //Usually get full bufSize of data (m_sectionLength * m_channelCount)
	uParam->doFilter();
}

void AdvantechBuff::setVoltageToFilter(const QVector<double> &voltageBuffer){
	for(int i = 0; i < m_sectionLength; i++){
		for(int j = 0; j < m_info.m_channelCount; j++){
			m_voltageFilters[j].appendToBuffer(voltageBuffer[i*m_info.m_channelCount + j]);
		}
	}
}

void AdvantechBuff::doFilter(){
	// pass to FilterView
	// update data in
	// for(int i = 0; i < m_voltageFilters.count(); i++){
		auto allVoltage = m_voltageFilters[0].getFilteredVoltage(false);
		// qDebug() << allVoltage;
		m_vector[0] = allVoltage.last();
	// }
}

const QVector<double> AdvantechBuff::getBufferedData(uint8_t channelN, bool debug){
	if(channelN >=m_voltageFilters.count())
		return QVector<double>();
	return m_voltageFilters[channelN].lastFiltered();
}

const QVector<qreal> AdvantechBuff::getTimeBuffer(){
	QVector<qreal> timeBuffer;
	for(auto i = 0; i < m_sectionLength; ++i){
		timeBuffer << i;
	}
	return timeBuffer;
}

const QVector<double> AdvantechBuff::getData(){ // const & ?
	// get one second data from Filter View
	return m_vector;
}
/*******************************/
/*******************************/
/*******************************/
/****        pass            ***/
/*******************************/
/*******************************/
/* Advantech Digital Output (pressure and temperature readings) */

AdvantechDO::AdvantechDO(const AdvDOType &info, QObject *parent) :
	AdvantechCtrl(info.m_deviceName,parent), m_info(info), m_instantDoCtrl(NULL), m_vector(16,false)
{
}

AdvantechDO::~AdvantechDO(){
    qDebug() << QString("Oh no, %1 was deleted").arg(m_info.m_deviceName);
}

void AdvantechDO::ConfigureDeviceDO(){
	m_instantDoCtrl = InstantDoCtrl::Create();

    std::wstring description = m_info.m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

	ErrorCode errorCode = Success;
	errorCode = m_instantDoCtrl->setSelectedDevice(selected);
	CheckError(errorCode);
    std::wstring profile = m_info.m_profilePath.toStdWString();
    errorCode = m_instantDoCtrl->LoadProfile(profile.c_str());
    CheckError(errorCode);
	portCount = m_instantDoCtrl->getPortCount();
	qDebug() << "INFO VALVE PORT COUNT " << portCount;
	resizeDataVector(portCount*8); // ?
}

void AdvantechDO::resizeDataVector(uint8_t size){
	this->m_vector.resize(size);
}

void AdvantechDO::applyFeatures(){
	
	DioFeatures * features = m_instantDoCtrl->getFeatures(); 
	Array<uint8>* portMasks = features->getDoDataMask();//getDataMask();

	quint8 *portStates = new quint8[portCount];
	ErrorCode errorCode = Success;
    errorCode = m_instantDoCtrl->Read(0, portCount, portStates);
    CheckError(errorCode);

	QVector<bool> vector;
	for(int i  = 0; i< portCount; ++i){
		for(int j = 0; j < 8; ++j){
			vector.append(portStates[i]>>j&0x1);
		}
	}
	qDebug() << m_name << " port states" << vector;
}

void AdvantechDO::readData(){
	uint8_t *portStates = new uint8_t[portCount];
    ErrorCode errorCode = Success;
	errorCode = m_instantDoCtrl->Read(0, portCount, portStates);
	CheckError(errorCode);
	if (errorCode != Success)
	{
		return;
	}
	QVector<bool> vector;
	for(int i  = 0; i< portCount; ++i){
		for(int j = 0; j < 8; ++j){
			// portStates == 00100000 
			//			0	0
			// 			1		0
			// 			2			1
			// 			3				0
			// 			4					0
			// 			5						0
			// 			6							0
			// 			7								0
			// vector = [0,0,1,0,0,0,0,0]
			vector.append(portStates[i]>>j&0x1);
		}
	}
	m_vector = vector;
}

QVector<bool> AdvantechDO::getData(){ // const & ?
	//vector=scaledData;
	return m_vector;
}

void AdvantechDO::setData(const QVector<bool> &changedState){
	uint8_t *portStates = new uint8_t[portCount];
	for(int i  = 0; i< portCount; ++i){
		uint8_t stack = 0;
		for(int j = 0; j < 8; ++j){
			if(changedState.at(j+8*i)) // changedState[j] 
				stack|=1u<<j; // this is just adding _1_ to ith [0,0,0,_i_,0,0,0,0]
		}
		portStates[i] = stack;
		qDebug() << portStates[i];
	}
	ErrorCode errorCode = Success;
	errorCode = m_instantDoCtrl->Write(0, portCount, portStates);
	CheckError(errorCode);
}

void AdvantechDO::CheckError(ErrorCode errorCode)
{
	if (errorCode >= 0xE0000000 && errorCode != Success)
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		qDebug() << QString("Warning Information %1").arg(message);
	}

	 if (BioFailed(errorCode))
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		qDebug() << QString("Warning Information %1").arg(message);
	}
}

AdvDOType AdvantechDO::getInfo(){
	return m_info;
}