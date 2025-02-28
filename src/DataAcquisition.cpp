#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent), fastFilter(new QTimer)
{
    GRAMsIntegrity["pressure"] = ControllerConnection::Offline;
    GRAMsIntegrity["temperature"] = ControllerConnection::Offline;
    GRAMsIntegrity["valves"] = ControllerConnection::Offline;
    connect(fastFilter, &QTimer::timeout, this, &DataAcquisition::filterEvent);
}

bool DataAcquisition::getGRAMsIntegrity(){
    //auto l_integrity = [](const QList<ControllerConnection> a) { 
    for(auto b:GRAMsIntegrity.values())
            if(b!=ControllerConnection::Online)
                return false;
    return true;
}

void DataAcquisition::setValvePointers(Valve ptr[], int valvesCnt){
    for(int i = 0; i < valvesCnt; ++i){
        m_valves[i] = &ptr[i];
    }
    m_valvesCnt = valvesCnt;
}

void DataAcquisition::setPressurePointers(ControllerData ptr[], int pressureCnt){
    for(int i = 0; i < pressureCnt; ++i){
        m_pressureSensors[i] = &ptr[i];
    }
    m_pressureSensorsCnt = pressureCnt;
}

void DataAcquisition::setTempPointers(ControllerData ptr[], int tempCnt){
    for(int i = 0; i < tempCnt; ++i){
        m_tempSensors[i] = &ptr[i];
    }
    m_tempSensorsCnt = tempCnt;
}

void DataAcquisition::initDaqDO(const daqParameters &parameter){
    // pass real info from Initialize
    AdvDOType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    reqValveDO.setInfo(a);
    reqValveDO.ConfigureDeviceDO();
    reqValveDO.readData();
    // valve objects
    const auto &readData = reqValveDO.getData();
    // if ok
    for(int i = 0; i < m_valvesCnt; ++i){
        m_valves[i]->setState(readData[i]);
    }
    GRAMsIntegrity["valves"] = ControllerConnection::Online;
}

void DataAcquisition::initDaqAIpres(const daqParameters &parameter){
    AdvAIType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    a.setDefaultType(parameter.m_defaultType);
    reqSensorAI.setInfo(a);
    reqSensorAI.Initialization();
    reqSensorAI.ConfigureDeviceBuff();

    a = reqSensorAI.getInfo();
    // a.channelCount() or m_pressureSensorsCnt
    for(int i = 0; i < a.channelCount(); ++i){
        reqSensorAI.setVolageFilter(i, filterView.getNewFilterParameters());
    } 
    // if ok
    // pass to filter
    const auto &readData = reqSensorAI.getData();
    for(int i = 0; i < m_pressureSensorsCnt; ++i){
        m_pressureSensors[i]->addValue(readData[i]);
    }

    GRAMsIntegrity["pressure"] = ControllerConnection::Online;
}

void DataAcquisition::setNewFilter(){ // invokable
    // if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
    //     return;
    // auto controller = m_controllerList["pressure"].staticCast<AdvantechBuff>();

    // auto parameters = filterView.getNewFilterParameters();
    // controller->setVolageFilter(0, parameters);
    // filterView.safeCheckOn();
}

void DataAcquisition::initDaqAItemp(const daqParameters &parameter){
    AdvAIType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    a.setDefaultType(parameter.m_defaultType);
    reqTempAI.setInfo(a);
    reqTempAI.Initialization();
    // if ok
    // without filters
    const auto &readData = reqTempAI.getData();
    for(int i = 0; i < m_tempSensorsCnt; ++i){
        m_tempSensors[i]->addValue(readData[i]);
    }
}

bool DataAcquisition::setValveStates(){
    if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
        return false;
    QVector<bool> changedState;
    // if changedState > 8*portCount!
    for(int i = 0; i < m_valvesCnt; ++i){
        changedState << m_valves[i]->getState();
    }
    // handler to unsuccessful set (true / false)
    return reqValveDO.setData(changedState);
}

// void DataAcquisition::advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings){
//     QSharedPointer<AdvantechCtrl> controller;
//     if(type == "valves"){
//         AdvDOType a(description);
//         a.setSettings(deviceSettings);
//         auto valves = new AdvantechDO(a);
//         valves->ConfigureDeviceDO();
//         valves->applyFeatures();
//         controller = QSharedPointer<AdvantechCtrl>(valves); // create?
//     }
//     else if (type == "pressure"){
//         AdvAIType a(description);
//         a.setSettings(deviceSettings);
//         auto pressure = new AdvantechBuff(a); //AdvantechAI(a)
//         pressure->ConfigureDeviceTest();
//         controller = QSharedPointer<AdvantechCtrl>(pressure);
//         filterView.setFilterSize(a.m_channelCount);
//     }
//     else if (type == "temperature"){
//         AdvAIType a(description);
//         a.setSettings(deviceSettings);
//         auto temperature = new AdvantechAI(a);
//         temperature->ConfigureDeviceTest();
//         controller = QSharedPointer<AdvantechCtrl>(temperature);
//     }
//     m_controllerList.insert(type,controller);
//     GRAMsIntegrity[type] = ControllerConnection::Online;
// }

void DataAcquisition::processEvents(){ // rewrite as each one read
    // only [pressure] and [temperature] and [vacuum] and [furnace] and [ ] 
    // without always [valve] read, only after change 
    // for (auto i = m_controllerList.cbegin(), end = m_controllerList.cend(); i != end; ++i){
    //     i.value()->readData();
    // } // it's okay?
}

void DataAcquisition::processEvents(QString purpose){
    if(GRAMsIntegrity[purpose]==ControllerConnection::Online);
        // m_controllerList[purpose]->readData();
}

void DataAcquisition::turnOnFilterTimer(bool s){
    if(s){
        fastFilter->start(1000);
    }
    else{
        fastFilter->stop();
    }
}

void DataAcquisition::filterEvent(){
    // auto controller = m_controllerList["pressure"].staticCast<AdvantechBuff>();
    // // only for first channel
    // filterView.appendDataToView(0, controller->getTimeBuffer(), controller->getBufferedData(0));
    // filterView.appendDataToXhatS(0, controller->getTimeBuffer(), controller->getXhatS(0));
    // filterView.appendDataToXhatT(0, controller->getTimeBuffer(), controller->getXhatT(0));
    // if(filterView.getSafeCheck()){
    //     auto originalBuffer = controller->getOriginalData(0);
    //     if(originalBuffer.isEmpty())
    //         return;
    //     filterView.saveToFile(originalBuffer);
    // }
    // if(filterView.getAppendCheck()){
    //     auto originalBuffer = controller->getOriginalData(0);
    //     if(originalBuffer.isEmpty())
    //         return;
    //     filterView.appendToFile(originalBuffer); 
    // }
}

// QMap<QString,QVector<double>> DataAcquisition::getMeasures(){ // const & >
    // QMap<QString,QVector<double>> dataMap;
    // auto default_val = QVector<double>(8,0.0);
    // for(const QString &type : {"pressure", "temperature"}){
    //     if(GRAMsIntegrity[type]!=ControllerConnection::Online){
    //         dataMap.insert(type, default_val);
    //         continue;
    //     }
    //     auto controller = m_controllerList[type].staticCast<AdvantechBuff>(); //AdvantechAI  // type of static_cast from profile?
    //     // rewrite this somehow maybe using lambda or idk
    //     // if(controller.isNull()) 
    //     //     dataMap.insert(type, default_val); 
    //     dataMap.insert(type, controller->getData());
    // }
    // return dataMap;
// }

// QVector<bool> DataAcquisition::getValves(){
    // auto default_val = QVector<bool>(16,false);
    // if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
    //     return default_val; 
    // auto controller = m_controllerList["valves"].staticCast<AdvantechDO>(); // type of static_cast from profile?
    // return controller->getData();
    // return QVector<bool>(16,false);
// }

// void DataAcquisition::testRead(){ // died
//     if(getGRAMsIntegrity())
//         processEvents();
// }