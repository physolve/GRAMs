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

AdvantechTest::AdvantechTest(const ControllerInfo &info, QObject *parent) : 
	QObject(parent), m_info(info)
{
	m_deviceName = m_info.deviceName();
	Initialization();
}

AdvantechTest::~AdvantechTest(){

}

void AdvantechTest::Initialization() // fill info
{
	//ui.cmbChannelCount->clear();
	//ui.cmbChannelStart->clear();
	//ui.cmbValueRange->clear();
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
	int logicChannelCount = instantAiCtrl->getChannelCount();

	m_channelStart = logicChannelCount;
	// for (int i = 0; i < logicChannelCount; i++)
	// {
	// 	//ui.cmbChannelStart->addItem(QString("%1").arg(i));
	// 	//iterate to qml in combobox ChannelStart
	// }
	m_channelCount = channelCount;
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
		m_valueRanges.append(str);
		//ui.cmbValueRange->addItem(str);
		//iterate to qml in combobox ValueRange
	}

	instantAiCtrl->Dispose();

	//Set the default value.
	//ui.cmbChannelStart->setCurrentIndex(0);
	//ui.cmbChannelCount->setCurrentIndex(1);
	//ui.cmbValueRange->setCurrentIndex(0);
}

QVariantMap AdvantechTest::getSettings(){
	QVariantMap settingPressure;
    settingPressure["channelCount"] = m_channelCount;
	settingPressure["channelStart"] = m_channelStart;
	settingPressure["valueRanges"] = m_valueRanges;
	settingPressure["profilePath"] = m_profilePath;
	return settingPressure;
}

void AdvantechTest::ConfigureDeviceTest(){ // after accept
	for (int i = 0; i < 16; i++)
	{
		scaledData[i] = 0;
	}
	
	if (!m_instantAiCtrl)
	{
      m_instantAiCtrl = InstantAiCtrl::Create();
	}

    std::wstring description = m_deviceName.toStdWString();
    DeviceInformation selected(description.c_str());

    ErrorCode errorCode = m_instantAiCtrl->setSelectedDevice(selected);
	CheckError(errorCode);

    std::wstring profile = m_profilePath.toStdWString();
    errorCode = m_instantAiCtrl->LoadProfile(profile.c_str());
    CheckError(errorCode);

	//Get channel max number. set value range for every channel.
	Array<AiChannel> *channels = m_instantAiCtrl->getChannels();
	for (int i = 0; i < channels->getCount(); i++)
	{
		channels->getItem(i).setValueRange(m_valueRange);
	}

	

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

AdvantechDO::AdvantechDO(const ControllerInfo &info, QObject *parent) : QObject(parent), m_info(info){
    Initialization();
}

AdvantechDO::~AdvantechDO(){  }

void AdvantechDO::Initialization()
{
    auto instantDoCtrl = InstantDoCtrl::Create();
    auto supportedDevices = instantDoCtrl->getSupportedDevices();

	if (supportedDevices->getCount() == 0)
	{
		/*send to qml debug line*/
        // QMessageBox::information(this, tr("Warning Information"), 
		// 	tr("No device to support the currently demonstrated function!"));
		/*for security*/
        //QCoreApplication::quit();
	} else {

		for (int i = 0; i < supportedDevices->getCount(); i++) {
			DeviceTreeNode const &node = supportedDevices->getItem(i);
			//qDebug("%d, %ls", node.DeviceNumber, node.Description);
            /*add to qml combo box*/
			
			//ui.cmbDevice->
            //addItem(QString::fromWCharArray(node.Description));
		}
		/*set to null index*/
        //ui.cmbDevice->setCurrentIndex(0);	
	}

	instantDoCtrl->Dispose();
	supportedDevices->Dispose();
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