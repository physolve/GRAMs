#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent), m_acquisitionTimer(new QTimer), m_supplyMeasure(false)
{
    GRAMsIntegrity["pressure"] = ControllerConnection::Offline;
    GRAMsIntegrity["temperature"] = ControllerConnection::Offline;
    GRAMsIntegrity["valves"] = ControllerConnection::Offline;
    connect(m_acquisitionTimer, &QTimer::timeout, this, &DataAcquisition::processEvents);
    // for valvesCnt m_valves = nullptr
    // for pressureCnt m_pressureSensors = nullptr
    // for tempCnt m_tempSensors = nullptr
    m_elapsedTimer.start();
}

// DataAcquisition::~DataAcquisition(){
//     // qDebug() ?
// }

bool DataAcquisition::getGRAMsIntegrity(){
    //auto l_integrity = [](const QList<ControllerConnection> a) { 
    for(const auto& b:GRAMsIntegrity.values())
            if(b!=ControllerConnection::Online)
                return false;
    return true;
}

void DataAcquisition::setValvePointers(const QVector<Valve*>& ptr){
    m_valves = ptr;
}

void DataAcquisition::setTimePointer(ControllerData* timeAnalog){
    m_time = timeAnalog;
}

void DataAcquisition::setPressurePointers(const QVector<ControllerData*>& ptr){
    m_pressureSensors = ptr;
}

void DataAcquisition::setTempPointers(const QVector<ControllerData*>& ptr){
    m_tempSensors = ptr;
}

void DataAcquisition::setFiltersDataPointers(const QVector<FilterData*>& ptr){
    m_filtersData = ptr;
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
    for(int i = 0; i < m_valves.count(); ++i){
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
    reqSensorAI.readData();
    // pass to filter
    const auto &readData = reqSensorAI.getData();
    if(m_filtersData.count() == m_pressureSensors.count()) qDebug() << "Filters to all USB4716 channels";
    for(int i = 0; i < m_pressureSensors.count(); ++i){
        m_pressureSensors[i]->addValue(readData[i], 0);
        m_filtersData[i]->setData(reqSensorAI.getBufferedData(i));
    }
    GRAMsIntegrity["pressure"] = ControllerConnection::Online;
}

void DataAcquisition::updateFilter(int chartIndex){
    filterView.readKalman();
    filterView.parseKalman();
    // other filters to update 
    reqSensorAI.setVolageFilter(0, filterView.getJsonMatrix());
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
    reqTempAI.ConfigureDeviceTemp();
    // if ok
    reqTempAI.readData();
    // without filters
    const auto &readData = reqTempAI.getData();
    for(int i = 0; i < m_tempSensors.count(); ++i){
        m_tempSensors[i]->addValue(readData[i]);
    }

    GRAMsIntegrity["temperature"] = ControllerConnection::Online;
}

bool DataAcquisition::setValveStates(){
    if(GRAMsIntegrity["valves"]!=ControllerConnection::Online)
        return false;
    QVector<bool> changedState;
    // if changedState > 8*portCount!
    for(int i = 0; i < m_valves.count(); ++i){
        changedState << m_valves[i]->getState();
    }
    // handler to unsuccessful set (true / false)
    return reqValveDO.setData(changedState);
}

void DataAcquisition::startAcquisition(){
    if(!getGRAMsIntegrity()){
        qDebug() << "Reading disabled";
        return;
    }
    m_acquisitionTimer->setInterval(500); // make default value
    m_acquisitionTimer->start();
}

void DataAcquisition::stopAcquisition(){
    m_acquisitionTimer->stop();
    // clear additionally
}


void DataAcquisition::processEvents(){
    if(!reqTempAI.isConnected()||!reqSensorAI.isConnected()){
        qDebug() << "Stopping acquisition...";
        stopAcquisition();
        return;
    }
    reqTempAI.readData();
    m_time->addValue(m_elapsedTimer.elapsed()/1000.0);  
    const auto &readDataPres = reqSensorAI.getData();
    for(int i = 0; i < m_pressureSensors.count(); ++i){
        m_pressureSensors[i]->addValue(readDataPres[i]); // ,0 minimal value
        m_filtersData[i]->setData(reqSensorAI.getBufferedData(i));
    }
    const auto &readDataTemp = reqTempAI.getData(); // this data from last read
    for(int i = 0; i < m_tempSensors.count(); ++i){
        m_tempSensors[i]->addValue(readDataTemp[i]);
    }
    reqSensorAI.readData();

    if(m_supplyMeasure){
        fillSupplyARQ();
    }
    if(m_leakageMeasure){
        fillLeakageRQ();
    }
}

bool DataAcquisition::setSupplyMeasure(bool supplyMeasure){
    if(!reqTempAI.isConnected()||!reqSensorAI.isConnected()){
        return false;
    }
    m_supplyMeasure = supplyMeasure;
    return true;
}

void DataAcquisition::setSupplyPressurePtr(FilterData* high, FilterData* low){
    m_supplyPressureHigh = high;
    m_supplyPressureLow = low;
}

void DataAcquisition::fillSupplyARQ(){
    QVector<double> filteredReal;
    int index = 0;
    const auto& filteredVoltage_high = reqSensorAI.getBufferedData(index);
    const auto& lin_A_high = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_high = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_high){
        filteredReal << lin_A_high * val + lin_B_high;
    }
    m_supplyPressureHigh->addData(filteredReal);
    filteredReal.clear();
    index = 1;
    const auto& filteredVoltage_low = reqSensorAI.getBufferedData(index);
    const auto& lin_A_low = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_low = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_low){
        filteredReal << lin_A_low * val + lin_B_low;
    }
    m_supplyPressureLow->addData(filteredReal);
}

bool DataAcquisition::setLeakageMeasure(bool leakageMeasure){
    if(!reqTempAI.isConnected()||!reqSensorAI.isConnected()){
        return false;
    }
    m_leakageMeasure = leakageMeasure;
    return true;
}

void DataAcquisition::setLeakagePressurePtr(FilterData* high, FilterData* low){
    m_leakagePressureHigh = high;
    m_leakagePressureLow = low;
}

void DataAcquisition::fillLeakageRQ(){
    QVector<double> filteredReal;
    int index = 2;
    const auto& filteredVoltage_high = reqSensorAI.getBufferedData(index);
    const auto& lin_A_high = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_high = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_high){
        filteredReal << lin_A_high * val + lin_B_high;
    }
    m_leakagePressureHigh->addData(filteredReal);
    filteredReal.clear();
    index = 3;
    const auto& filteredVoltage_low = reqSensorAI.getBufferedData(index);
    const auto& lin_A_low = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_low = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_low){
        filteredReal << lin_A_low * val + lin_B_low;
    }
    m_leakagePressureLow->addData(filteredReal);
}