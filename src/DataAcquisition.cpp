#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent)
{
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
}

void DataAcquisition::processEvents(){
    for (auto i = m_controllerList.cbegin(), end = m_controllerList.cend(); i != end; ++i){
        i.value()->readData();
    } // it's okay?
}

const QMap<QString,QVector<double>> DataAcquisition::getMeasures(){ // const & >
    QMap<QString,QVector<double>> dataMap;
    auto defautl_val = QVector<double>(8,0.0);
    if(!m_controllerList.contains("pressure"))
        dataMap.insert("pressure", defautl_val);
    else{
        auto controller = m_controllerList["pressure"].staticCast<AdvantechAI>();
        // rewrite this somehow maybe using lambda or idk
        if(controller.isNull()) 
            dataMap.insert("pressure", defautl_val); 
        dataMap.insert("pressure", controller->getData());
    }
    
    if(!m_controllerList.contains("temperature"))
        dataMap.insert("temperature", defautl_val);
    else{
        auto controller = m_controllerList["temperature"].staticCast<AdvantechAI>();
        if(!controller.isNull()) 
            dataMap.insert("temperature", defautl_val);
        dataMap.insert("temperature", controller->getData());
    }
    return dataMap;
}
const QVector<bool> DataAcquisition::getValves(){
    auto defautl_val = QVector<bool>(16,false);
    if(!m_controllerList.contains("valves"))
        return defautl_val;   
    auto controller =  m_controllerList["valves"].staticCast<AdvantechDO>();
    if(controller.isNull())
        return defautl_val;
    return controller->getData();
}

void DataAcquisition::testRead(){ // died
    if(m_controllerList.isEmpty())
        return;
    processEvents();
    // for(auto vector : getMeasures()){
    //     qDebug() << vector;
    // }
    // qDebug() << getValves();
}