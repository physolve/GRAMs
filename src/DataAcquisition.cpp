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
    }
}

const QList<QVector<double>> DataAcquisition::getMeasures(){ // const & >
    QList<QVector<double>> dataList;

    auto controller = m_controllerList["pressure"].staticCast<AdvantechAI>();
    // rewrite this somehow maybe using lambda or idk
    if(!controller.isNull()) 
        dataList.append(controller->getData());
    else dataList.append(QVector<double>(8,0.0));
    controller = m_controllerList["temperature"].staticCast<AdvantechAI>();

    if(!controller.isNull()) 
        dataList.append(controller->getData());
    else dataList.append(QVector<double>(8,0.0));

    return dataList;
}
const QVector<bool> DataAcquisition::getValves(){
    auto controller = m_controllerList["valves"].staticCast<AdvantechDO>();
    if(!controller.isNull())
        return controller->getData();
    else
        return QVector<bool>(8,false);
}
void DataAcquisition::testRead(){ // died
    processEvents();
    for(auto vector : getMeasures()){
        qDebug() << vector;
    }
    qDebug() << getValves();
}