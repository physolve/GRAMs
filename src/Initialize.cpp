#include "Initialize.h"
#include "Controller.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

#include "../lib/bdaqctrl.h"
using namespace Automation::BDaq;

Initialize::Initialize(QObject *parent) :
    QObject(parent)
{
    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    bool checkPass = false;
    QString m_rawData;
    checkPass = readProfile(m_rawData);
    checkPass = jsonParser(m_rawData);
    
    //device map 
    //  -_ Advantech device map
    //if(profile advantech!!!)
    checkPass = advantechDeviceCheck();
}

bool Initialize::readProfile(QString &rawData){
    QDir dir("profile");
    if(!dir.exists()) return false;
    QFile file;
    file.setFileName(dir.filePath("GRAMsPfp.json"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    rawData = file.readAll();
    file.close();
    return true;
}

bool Initialize::jsonParser(QString &rawData){
    QJsonDocument document = { QJsonDocument::fromJson(rawData.toUtf8()) };
    QJsonObject jsonObject = document.object();
    QMap<int, QString> m;
    for(auto s : jsonObject.keys()) // it is just sorting thing
        m[jsonObject[s].toObject()["profileId"].toInt()] = s;
    m_profileNames = m.values();
    m_profileJson = jsonObject.toVariantMap();
    return true;
}

bool Initialize::advantechDeviceCheck(){
    auto startCheckInstance = InstantDoCtrl::Create(); // does it work with every Adv controller?
    auto allSupportedDevices = startCheckInstance->getSupportedDevices();
    if (allSupportedDevices->getCount() == 0)
    {
        qDebug() << "No advantech devices connected";
        return false;
    }
    for(int i = 0; i < allSupportedDevices->getCount(); i++){
        DeviceTreeNode const &node = allSupportedDevices->getItem(i);
        qDebug("%d, %ls", node.DeviceNumber, node.Description);
        auto advantechDescription = QString::fromWCharArray(node.Description).split(',');
        auto tempName = advantechDescription.value(0);
        auto tempBID = advantechDescription.value(1);
        m_advantechDeviceMap.insert(tempBID,tempName);
    }
    startCheckInstance->Dispose();
    allSupportedDevices->Dispose();
    return true;
}

void Initialize::advModuleAIType(){ // how to assemble description 
    auto options = m_advantechDeviceMap.value(device).toStringList(); // {tempName, tempPurpose}
    auto description = options.at(0) +','+device;
    AdvAIType a(description);
    auto demoAdvAI = AdvantechTest(a);
    demoPressure.Initialization();
    a = demoPressure.getInfo();
    auto tempName = a.deviceName();
    m_deviceSettings[tempName] = a.getSettings();
}

