#include "Controller.h"

#include <QDebug>

Controller::Controller(QString deviceName, QObject *parent) : QObject(parent), deviceName(deviceName){

}

Controller::~Controller(){
	qDebug() << "Controller deleted";
}

void Controller::Initialization(){

}

AdvantechDO::AdvantechDO(QString deviceName, QObject *parent) : Controller(deviceName){
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

AdvantechAI::AdvantechAI(QString deviceName, QObject *parent) : Controller(deviceName){
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