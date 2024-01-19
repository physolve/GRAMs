#include "DataAcquisition.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QTimer>

//Q_DECLARE_METATYPE(QAbstractSeries *)
//Q_DECLARE_METATYPE(QAbstractAxis *)


DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent)
{
    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    // later move THIS JSON SHIT somewhere else, like in GRAM constructor or individual class
    QFile file;
    QDir dir("profile");
    file.setFileName(dir.filePath("GRAMsPfp.json")); //???
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawData = file.readAll();
    file.close();
    QJsonDocument document   =   { QJsonDocument::fromJson(rawData.toUtf8()) };
    QJsonObject jsonObject = document.object();

    // TO CREATE PROFILE combo box
    QMap<int, QString> m;
    for(auto s : jsonObject.keys()) m[jsonObject[s].toObject()["profileId"].toInt()] = s;
    m_profileNames = QStringList(m.values());
    m_profileJson = jsonObject.toVariantMap();

    if(advantechDeviceCheck(m_advantechDeviceMap)){
        for(auto device : m_advantechDeviceMap.keys()){
            auto options = m_advantechDeviceMap.value(device).toStringList(); // {tempName, tempPurpose}
            auto description = options.at(0) +','+device;
            if(options.at(1)=="pressure"){
                ControllerPrType a(description);
                auto demoPressure = AdvantechTest(a);
                demoPressure.Initialization();
                a = demoPressure.getInfo();
                auto tempName = a.deviceName();
                m_deviceSettings[tempName] = a.getSettings();
            }
            else if(options.at(1)=="temperature"){
                ControllerPrType a(description);
                auto demoTemperature = AdvantechTest(a); // for now Thermocouples
                demoTemperature.Initialization();
                a = demoTemperature.getInfo();
                auto tempName = a.deviceName();
                m_deviceSettings[tempName] = a.getSettings();
        
            }
            else if(options.at(1)=="valve"){  
                ControllerValveType a(description);
                auto demoValves = AdvantechDO(a);
                a = demoValves.getInfo();
                auto tempName = a.deviceName();
                QVariantMap blank;
                blank["nothing"] = 0;
                m_deviceSettings[tempName] = blank; // demoPressure.m_deviceName
            }
            // make qml combobox with ret.m_channelCount and ret.m_channelStart and profile and etc.
        }  
    }
}

bool DataAcquisition::advantechDeviceCheck(QVariantMap& advantechDeviceMap) const{
    /*ADVANTECH Controller Initialization*/
    //QVariantMap advantechDevices;
    // FARPROC fn = GetProcAddress(DNL_Instance(), "AdxDaqNaviLibInitialize");
    // if(fn == NULL) return false; // работает

    static HMODULE instance = LoadLibrary(TEXT("biodaq.dll")); //???
    
    if(instance == NULL) return false; 

    // try DaqCtrlBase_Create(Scenario type)  - SceInstantAi works but badly
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
        //auto advantechName = advantechDescription[0]; // I use this because we split BoardID, might use later
        auto tempName = advantechDescription.value(0);
        auto tempBID = advantechDescription.value(1);
        QString tempPurpose;
        if(tempName == "DemoDevice"){
            if(tempBID == "BID#0"){
                tempName = "USB-4716";
                tempPurpose = "pressure";
            }
            else if(tempBID == "BID#1"){
                tempName = "USB-4718";
                tempPurpose = "temperature";
            }
            else {
                tempName = "USB-4750";
                tempPurpose = "valve";
            }
        }
        QVariantList val;
        val.append(tempName);
        val.append(tempPurpose);
        advantechDeviceMap.insert(tempBID, val); // подумай насчет номера в Map, тут только advantech номера
    }
    startCheckInstance->Dispose();
    allSupportedDevices->Dispose();
    return true;
    
    /*ADVANTECH Controller Initialization*/
    // use advantechDeviceMap to additional features like in Instant_AI
}

void DataAcquisition::setDeviceParameters(const QVariantMap& deviceParametersMap){ // rewrite to ONE call not in loop
    // check what deviceSetting name we have (short one?)
    for (auto i = deviceParametersMap.cbegin(), end = deviceParametersMap.cend(); i != end; ++i){
        QVariantMap deviceMap = m_deviceSettings[i.key()].toMap();
        auto obj = i.value().toMap();
        deviceMap["chChannelCount"] = obj["indexChannelCount"];
        deviceMap["chCannelStart"] = obj["indexChannelStart"];
        deviceMap["chValueRange"] = obj["indexValueRange"];
        deviceMap["profilePath"] = obj["inProfilePath"]; 
        m_deviceSettings[i.key()].setValue(deviceMap);
    }
}
// void DataAcquisition::setChannelMapping(QVariantMap obj){
//     // sensor mapping then used for setNames like position of controller output???
//     m_sensorMapping = obj;
// }

void DataAcquisition::saveStartDevice(){
    for(auto device : m_advantechDeviceMap.keys()){
       auto options = m_advantechDeviceMap.value(device).toStringList(); // {tempName, tempPurpose}
        auto description = options.at(0) +','+device;
        if(options.at(1)=="pressure"){
            ControllerPrType info(description);
            info.setSettings(m_deviceSettings[info.deviceName()].toMap());
            auto controllerPr = new AdvantechTest(info);
            controllerPr->ConfigureDeviceTest();
            controllerList.append(controllerPr);
        }
        else if(options.at(1)=="temperature"){
            ControllerPrType info(description);
            info.setSettings(m_deviceSettings[info.deviceName()].toMap());
            auto controllerTemp = new AdvantechTest(info);
            controllerTemp->ConfigureDeviceTest();
            controllerList.append(controllerTemp);
    
        }
        else if(options.at(1)=="valve"){  
            ControllerValveType info(description);
            auto controllerValves = new AdvantechDO(info);
            controllerDO.append(controllerValves);
        }
    }
    //m_timer->start(100);
}

void DataAcquisition::readDataFromDevice(ControllerInfo info){ // do we necessary need this?
    auto itr = std::find_if(controllerList.begin(), controllerList.end(), [&](AdvantechTest* someclass) { return someclass->m_deviceName == info.deviceName(); });
    if(itr != controllerList.end()) {
        (*itr)->readData();
    }
}

void DataAcquisition::processEvents(){
    for(auto controller : controllerList){
        controller->readData();
    }
}

const QList<QVector<double>> DataAcquisition::getDataList(){ // const & >
    QList<QVector<double>> dataList;
    for(auto controller : controllerList){ // data from controllerList in order of purpose
        dataList << controller->getData();
    }
    return dataList;
}

QVariantMap DataAcquisition::profileJson() const
{
  return m_profileJson;
}
