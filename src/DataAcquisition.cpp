#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent)
{
    GRAMsIntegrity["pressure"] = ControllerConnection::Offline;
    GRAMsIntegrity["temperature"] = ControllerConnection::Offline;
    GRAMsIntegrity["valves"] = ControllerConnection::Offline;
}

const bool DataAcquisition::getGRAMsIntegrity(){
    //auto l_integrity = [](const QList<ControllerConnection> a) { 
    for(auto b:GRAMsIntegrity.values())
            if(b!=ControllerConnection::Online)
                return false;
    return true;
}

void DataAcquisition::advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings){
    QSharedPointer<AdvantechCtrl> controller;
    if(type == "valves"){
        AdvDOType a(description);
        a.setSettings(deviceSettings);
        auto valves = new AdvantechDO(a);
        valves->ConfigureDeviceDO();
        valves->applyFeatures();
        controller = QSharedPointer<AdvantechCtrl>(valves);
    }
    else{
        AdvAIType a(description);
        a.setSettings(deviceSettings);
        auto pressure = new AdvantechAI(a);
        pressure->ConfigureDeviceTest();
        controller = QSharedPointer<AdvantechCtrl>(pressure);
    }
    m_controllerList.insert(type,controller);
    GRAMsIntegrity[type] = ControllerConnection::Online;
}

void DataAcquisition::processEvents(){
    for (auto i = m_controllerList.cbegin(), end = m_controllerList.cend(); i != end; ++i){
        i.value()->readData();
    } // it's okay?
}

const QMap<QString,QVector<double>> DataAcquisition::getMeasures(){ // const & >
    QMap<QString,QVector<double>> dataMap;
    auto defautl_val = QVector<double>(8,0.0);
    for(const QString &type : {"pressure", "temperature"}){
        if(GRAMsIntegrity[type]!=ControllerConnection::Online){
            dataMap.insert(type, defautl_val);
            continue;
        }
        auto controller = m_controllerList[type].staticCast<AdvantechAI>(); // type of static_cast from profile?
        // rewrite this somehow maybe using lambda or idk
        // if(controller.isNull()) 
        //     dataMap.insert(type, defautl_val); 
        dataMap.insert(type, controller->getData());
    }
    return dataMap;
}
const QVector<bool> DataAcquisition::getValves(){
    auto defautl_val = QVector<bool>(16,false);
    if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
        return defautl_val; 
    auto controller = m_controllerList["valves"].staticCast<AdvantechDO>(); // type of static_cast from profile?
    return controller->getData();
}

void DataAcquisition::testRead(){ // died
    if(getGRAMsIntegrity())
        processEvents();
}

