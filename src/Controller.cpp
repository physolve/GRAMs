#include "Controller.h"

#include <QDebug>

Controller::Controller(const ControllerInfo &info, QObject *parent) : 
	QObject(parent), m_info(info)
{}
Controller::~Controller(){
	qDebug() << "Controller deleted";
}
void Controller::Initialization(){

}

// temporal names should be erased and then reused in simulation controller //

AdvantechTest::AdvantechTest(const AdvAIType &info, QObject *parent) : 
	QObject(parent), m_info(info), m_instantAiCtrl(NULL), m_vector(8,0.0)
{
	auto tempDevice = m_info.deviceName();
	auto tempList = tempDevice.split(',');
	auto tempName = tempList.value(0);
	auto tempBID = tempList.value(1);
	if(tempName == "USB-4716" || "USB-4718"){
		tempName = "DemoDevice";
	}
	m_deviceName = tempName+','+tempBID;//m_info.deviceName();
}

AdvantechTest::~AdvantechTest(){
	qDebug() << QString("Oh no, %1 was deleted").arg(m_deviceName);
}

void AdvantechTest::Initialization() // fill info
{
	// replace in qml 
    std::wstring description = m_deviceName.toStdWString();//ui.cmbDevice->currentText().toStdWString();
    DeviceInformation selected(description.c_str());

    InstantAiCtrl *instantAiCtrl = InstantAiCtrl::Create();
	ErrorCode errorCode = instantAiCtrl->setSelectedDevice(selected);
	
	//ui.btnOK->setEnabled(true);
	
	if (errorCode != 0){
		QString str;
        QString des = QString::fromStdWString(description);
		//qDebug() << (this, "Warning Information", str);
		//ui.btnOK->setEnabled(false);
		return;
	}

	int channelCount = (instantAiCtrl->getChannelCount() < 16) ? 
		instantAiCtrl->getChannelCount() : 16;
	// pull to Info
	int logicChannelCount = instantAiCtrl->getChannelCount();
	// pull to Info
	m_info.m_channelStart = logicChannelCount;
	// for (int i = 0; i < logicChannelCount; i++)
	// {
	// 	//ui.cmbChannelStart->addItem(QString("%1").arg(i));
	// 	//iterate to qml in combobox ChannelStart
	// }
	m_info.m_channelCount = channelCount;
	// for (int i = 0; i < channelCount; i++)
	// {
	// 	//ui.cmbChannelCount->addItem(QString("%1").arg(i + 1));
	// 	//iterate to qml in combobox ChannelCount
	// }

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
		//ui.cmbValueRange->addItem(str);
		//iterate to qml in combobox ValueRange
	}

	instantAiCtrl->Dispose();
}

const AdvAIType& AdvantechTest::getInfo(){
	return m_info;
}

void AdvantechTest::ConfigureDeviceTest(){ // after accept
	//m_vector = QVector<double>(16,0.0);
	//qDebug() << m_vector;
	if (m_instantAiCtrl==NULL)
	{
      m_instantAiCtrl = InstantAiCtrl::Create();
	}

    std::wstring description = m_deviceName.toStdWString();
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
	resizeDataVector(m_info.m_channelCountCh);
}

void AdvantechTest::resizeDataVector(uint8_t size){
	this->m_vector.resize(size);
}

void AdvantechTest::CheckError(ErrorCode errorCode)
{
    if (BioFailed(errorCode))
	{
		QString message = tr("Sorry, there are some errors occurred, Error Code: 0x") +
			QString::number(errorCode, 16).right(8).toUpper();
		qDebug() << QString("Warning Information %1").arg(message);
	}
}

void AdvantechTest::readData(){
	ErrorCode errorCode = Success;
	//qDebug() << "controller Data count = " << m_vector.count();
	errorCode = m_instantAiCtrl->Read(m_info.m_channelStartCh, m_info.m_channelCountCh, m_vector.data());
	CheckError(errorCode);
	if (errorCode != Success)
	{
		return;
	}
}

QVector<double> AdvantechTest::getData(){ // const & ?
	//vector=scaledData;
	return m_vector;
}

AdvantechDO::AdvantechDO(const AdvDOType &info, QObject *parent) : 
	QObject(parent), m_info(info), m_instantDoCtrl(NULL), m_vector(8,0.0)
{
	auto tempDevice = m_info.deviceName();
	auto tempList = tempDevice.split(',');
	auto tempName = tempList.value(0);
	auto tempBID = tempList.value(1);
	if(tempName == "USB-4750"){
		tempName = "DemoDevice";
	}
	m_deviceName = tempName+','+tempBID;//m_info.deviceName();
}

AdvantechDO::~AdvantechDO(){ }

void AdvantechDO::ConfigureDeviceDO(){
	m_instantDoCtrl = InstantDoCtrl::Create();

    std::wstring description = m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

	ErrorCode errorCode = Success;
	errorCode = m_instantDoCtrl->setSelectedDevice(selected);
	CheckError(errorCode);
    std::wstring profile = m_info.m_profilePath.toStdWString();
    errorCode = m_instantDoCtrl->LoadProfile(profile.c_str());
    CheckError(errorCode);
	portCount = m_instantDoCtrl->getPortCount();

}

void AdvantechDO::applyFeatures(){
	
	DioFeatures * features = m_instantDoCtrl->getFeatures(); 
	Array<uint8>* portMasks = features->getDoDataMask();//getDataMask();

	quint8 *portStates = new quint8[portCount];
	ErrorCode errorCode = Success;
    errorCode = m_instantDoCtrl->Read(0, portCount, portStates);
    CheckError(errorCode);

	qDebug() << portMasks;
	//setting initial state
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

const AdvDOType& AdvantechDO::getInfo(){
	return m_info;
}

AdvantechAI::AdvantechAI(const ControllerInfo &info, QObject *parent) : QObject(parent), m_info(info){
    Initialization();
}

AdvantechAI::~AdvantechAI(){

}

void AdvantechAI::Initialization()
{
    InstantAiCtrl *instantAiCtrl = InstantAiCtrl::Create();
	Array<DeviceTreeNode>* supportedDevices = instantAiCtrl->getSupportedDevices();

	if (supportedDevices->getCount() == 0)
	{
		/*send to qml debug line*/
        // QMessageBox::information(this, tr("Warning Information"), 
		// 	tr("No device to support the currently demonstrated function!"));
		/*for security*/
        //QCoreApplication::quit();
	} 
	else
	{
		for (int i = 0; i < supportedDevices->getCount(); i++)
		{
			DeviceTreeNode const &node = supportedDevices->getItem(i);
			//qDebug("%d, %ls\n", node.DeviceNumber, node.Description);
			/*add to qml combo box*/
            //ui.cmbDevice->
            //addItem(QString::fromWCharArray(node.Description));
		}
		/*set to null index*/
        //ui.cmbDevice->setCurrentIndex(0);
	}

	instantAiCtrl->Dispose();
	supportedDevices->Dispose();
}