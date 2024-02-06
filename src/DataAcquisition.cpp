#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent)
{
}

void DataAcquisition::advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings){
    if(type == "pressure" || type == "temperature"){
        AdvAIType a(description);
        a.setSettings(deviceSettings);
        auto demoPressure = new AdvantechTest(a);
        demoPressure->ConfigureDeviceTest();
        controllerList.append(demoPressure);
    }
    else if(type == "valves"){
        AdvDOType a(description);
        a.setSettings(deviceSettings);
        auto demoValves = new AdvantechDO(a);
        demoValves->ConfigureDeviceDO();
        demoValves->applyFeatures();
        controllerDO.append(demoValves);
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

void DataAcquisition::testRead(){ // died
    processEvents();
    for(auto vector : getDataList()){
        qDebug() << vector;
    }
}