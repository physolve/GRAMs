#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent), filterView(), fastFilter(new QTimer)
{
    GRAMsIntegrity["pressure"] = ControllerConnection::Offline;
    GRAMsIntegrity["temperature"] = ControllerConnection::Offline;
    GRAMsIntegrity["valves"] = ControllerConnection::Offline;
    connect(fastFilter, &QTimer::timeout, this, &DataAcquisition::filterEvent);
    connect(&filterView, &FilterView::matrixChanged, this, &DataAcquisition::setNewFilter);
}

FilterView* DataAcquisition::getFilterView(){
    return &filterView;
}

bool DataAcquisition::getGRAMsIntegrity(){
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
        controller = QSharedPointer<AdvantechCtrl>(valves); // create?
    }
    else if (type == "pressure"){
        AdvAIType a(description);
        a.setSettings(deviceSettings);
        auto pressure = new AdvantechBuff(a); //AdvantechAI(a)
        pressure->ConfigureDeviceTest();
        controller = QSharedPointer<AdvantechCtrl>(pressure);
        filterView.setFilterSize(a.m_channelCount);
    }
    else if (type == "temperature"){
        AdvAIType a(description);
        a.setSettings(deviceSettings);
        auto temperature = new AdvantechAI(a);
        temperature->ConfigureDeviceTest();
        controller = QSharedPointer<AdvantechCtrl>(temperature);
    }
    m_controllerList.insert(type,controller);
    GRAMsIntegrity[type] = ControllerConnection::Online;
}

void DataAcquisition::processEvents(){
    // only [pressure] and [temperature] and [vacuum] and [furnace] and [ ] 
    // without always [valve] read, only after change 
    for (auto i = m_controllerList.cbegin(), end = m_controllerList.cend(); i != end; ++i){
        i.value()->readData();
    } // it's okay?
}

void DataAcquisition::processEvents(QString purpose){
    if(GRAMsIntegrity[purpose]==ControllerConnection::Online)
        m_controllerList[purpose]->readData();
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
    auto controller = m_controllerList["pressure"].staticCast<AdvantechBuff>();
    // only for first channel
    filterView.appendDataToView(0, controller->getTimeBuffer(), controller->getBufferedData(0, true));
}

QMap<QString,QVector<double>> DataAcquisition::getMeasures(){ // const & >
    QMap<QString,QVector<double>> dataMap;
    auto defautl_val = QVector<double>(8,0.0);
    for(const QString &type : {"pressure", "temperature"}){
        if(GRAMsIntegrity[type]!=ControllerConnection::Online){
            dataMap.insert(type, defautl_val);
            continue;
        }
        auto controller = m_controllerList[type].staticCast<AdvantechBuff>(); //AdvantechAI  // type of static_cast from profile?
        // rewrite this somehow maybe using lambda or idk
        // if(controller.isNull()) 
        //     dataMap.insert(type, defautl_val); 
        dataMap.insert(type, controller->getData());
    }
    return dataMap;
}

QVector<bool> DataAcquisition::getValves(){
    auto defautl_val = QVector<bool>(16,false);
    if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
        return defautl_val; 
    auto controller = m_controllerList["valves"].staticCast<AdvantechDO>(); // type of static_cast from profile?
    return controller->getData();
}

void DataAcquisition::setValves(const QVector<bool> &states){
    if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
        return;
    auto controller = m_controllerList["valves"].staticCast<AdvantechDO>();
    controller->setData(states);
}

// void DataAcquisition::testRead(){ // died
//     if(getGRAMsIntegrity())
//         processEvents();
// }

void DataAcquisition::setNewFilter(){
    // if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
    //     return;
    auto controller = m_controllerList["pressure"].staticCast<AdvantechBuff>();

    auto parameters = filterView.getNewFilterParameters();

    controller->setVolageFilter(0, parameters);
}
