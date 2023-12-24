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
            auto description = m_advantechDeviceMap.value(device).toString()+','+device;
            ControllerInfo a(description);
            m_deviceInfoList.append(a);
            // make qml combobox with ret.m_channelCount and ret.m_channelStart and profile and etc.
        }

        // not safe due to TWO DemoDevice (change later to iterator in m_deviceInfoList)
        auto demoPressure = AdvantechTest(m_deviceInfoList.value(0));
        demoPressure.Initialization();
        m_deviceInfoList[0] = demoPressure.getInfo();
        auto tempName = m_deviceInfoList[0].deviceName();
        auto a = (ControllerPrType*)&m_deviceInfoList[0];
        m_deviceSettings[tempName] = a->getSettings(); // demoPressure.m_deviceName
        auto demoTemperature = AdvantechTest(m_deviceInfoList.value(1)); // for now Thermocouples
        demoTemperature.Initialization();
        m_deviceInfoList[1] = demoTemperature.getInfo();
        tempName = m_deviceInfoList[1].deviceName();
        a = (ControllerPrType*)&m_deviceInfoList[1];
        m_deviceSettings[tempName] = a->getSettings(); //demoValves.m_deviceName
        
        //you don't need preliminary parameters for valves
        
        //auto demoValves = AdvantechDO(m_deviceInfoList.value(2)); // for now Valves
        //m_deviceInfoList[2] = demoValves.getInfo(); without changes here
        //auto tempName = m_deviceInfoList[2].deviceName();
        //m_deviceSettings[tempName] = m_deviceInfoList[2].getSettings(); // demoPressure.m_deviceName
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
        if(tempName == "DemoDevice"){
            if(tempBID == "BID#0")
                tempName = "USB-4716";
            else if(tempBID == "BID#1") 
                tempName = "USB-4718";
            else tempName = "USB-4750";
        }
        advantechDeviceMap.insert(tempBID, tempName); // подумай насчет номера в Map, тут только advantech номера
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
    for(ControllerInfo& info : m_deviceInfoList){
        info.setSettings(m_deviceSettings[info.deviceName()].toMap());
        // костыль для m_sensorMapping
        //auto tempShortName = info.deviceName().split(',');
        //qDebug() << m_sensorMapping[tempShortName.at(0)].toStringList();
        //info.setNames(m_sensorMapping[tempShortName.at(0)].toStringList());

        //auto demo = AdvantechTest(info);
        //demo.ConfigureDeviceTest();
        //appendToControllerList(demo);

        controllerList.append(new AdvantechTest(info));
        controllerList.last()->ConfigureDeviceTest();
    }
    //m_timer->start(100);
}

// void DataAcquisition::appendToControllerList(AdvantechTest& device){
//     controllerList.append(&device);
// }

const QList<ControllerInfo> DataAcquisition::getControllersInfo(){ // get Read controllers
    return m_deviceInfoList;
}

// get Write controllers (valves)


void DataAcquisition::readDataFromDevice(ControllerInfo info){ // do we necessary need this?
    auto itr = std::find_if(controllerList.begin(), controllerList.end(), [&](AdvantechTest* someclass) { return someclass->m_deviceName == info.deviceName(); });
    if(itr != controllerList.end()) {
        (*itr)->readData();
    }
}

void DataAcquisition::processEvents(){ // function to iterate over m_deviceInfoList every second (or frequently)
    // for(auto info : m_deviceInfoList){
    //     readDataFromDevice(info);
    // }
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
