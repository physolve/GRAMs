#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent), m_acquisitionTimer(new QTimer), m_leakageMeasure(false), canReadFast(false), canReadSlow(true) 
{
    // GRAMsIntegrity["pressure"] = ControllerConnection::Offline;
    pressureController = false;
    // GRAMsIntegrity["temperature"] = ControllerConnection::Offline;
    temperatureController = false;
    // GRAMsIntegrity["valves"] = ControllerConnection::Offline;
    connect(m_acquisitionTimer, &QTimer::timeout, this, &DataAcquisition::processEvents);
    // add thread for acquisition
    // m_acquisitionTimer->moveToThread(new QThread());

    // for valvesCnt m_valves = nullptr
    // for pressureCnt m_pressureSensors = nullptr
    // for tempCnt m_tempSensors = nullptr
    m_elapsedTimer.start();
}

DataAcquisition::~DataAcquisition(){
    if(m_acquisitionTimer->isActive())
        m_acquisitionTimer->stop();
    delete m_acquisitionTimer;
}

RealSensorSource &DataAcquisition::real(){
    if(!m_real){
        auto source = std::make_unique<RealSensorSource>();
        m_real = source.get();
        m_source = std::move(source);
    }
    return *m_real;
}

void DataAcquisition::setSensorSource(std::unique_ptr<ISensorSource> source){
    m_real = nullptr;
    m_source = std::move(source);
}

void DataAcquisition::markControllersConnected(){
    pressureController = m_source != nullptr;
    temperatureController = m_source != nullptr;
}

bool DataAcquisition::getGRAMsIntegrity(){
    //auto l_integrity = [](const QList<ControllerConnection> a) { 
    // for(const auto& b:GRAMsIntegrity.values())
    //         if(b!=ControllerConnection::Online)
    //             return false;
    if(!pressureController || !temperatureController)
        return false;
    return true;
}

// void DataAcquisition::setValvePointers(const QVector<Valve*>& ptr){
//     m_valves = ptr;
// }

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

void DataAcquisition::setTurboVacuumPointer(DataCollection* ptr){
    m_vacuumSensorTurbo = ptr;
}

void DataAcquisition::setVacuumPointer(DataCollection* ptr){
    m_vacuumSensor = ptr;
}

void DataAcquisition::initDaqAIpres(const daqParameters &parameter){
    AdvAIType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    a.setDefaultType(parameter.m_defaultType);
    auto &reqSensorAI = real().pressureCard();
    reqSensorAI.setInfo(a);
    reqSensorAI.Initialization();
    reqSensorAI.ConfigureDeviceBuff();

    a = reqSensorAI.getInfo();
    // a.channelCount() or m_pressureSensorsCnt
    for(int i = 0; i < a.channelCount(); ++i){
        reqSensorAI.setVolageFilter(i, filterView.getNewFilterParameters());
    } 
    // if ok
    m_source->readPressure();
    // pass to filter
    const auto &readData = m_source->pressureVolts();
    if(m_filtersData.count() == m_pressureSensors.count()) qDebug() << "Filters to all USB4716 channels";
    for(int i = 0; i < m_pressureSensors.count(); ++i){
        m_pressureSensors[i]->addValue(readData[i], 0);
        m_filtersData[i]->setData(m_source->pressureBuffer(i));
    }
    pressureController = true;
}

void DataAcquisition::updateFilter(int chartIndex){
    filterView.readKalman();
    filterView.parseKalman();
    // other filters to update 
    if(!m_real)
        return;   // фильтр живёт в драйвере платы — в демо-режиме его нет
    m_real->pressureCard().setVolageFilter(0, filterView.getJsonMatrix());

    // auto parameters = filterView.getNewFilterParameters();
    // controller->setVolageFilter(0, parameters);
    // filterView.safeCheckOn();
}

void DataAcquisition::initDaqAItemp(const daqParameters &parameter){
    AdvAIType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    a.setDefaultType(parameter.m_defaultType);
    auto &reqTempAI = real().temperatureCard();
    reqTempAI.setInfo(a);
    reqTempAI.Initialization();
    reqTempAI.ConfigureDeviceTemp();
    // if ok
    m_source->readTemperature();
    // without filters
    const auto &readData = m_source->temperatures();
    for(int i = 0; i < m_tempSensors.count(); ++i){
        m_tempSensors[i]->addValue(readData[i]);
    }
    // GRAMsIntegrity["temperature"] = ControllerConnection::Online;
    temperatureController = true;
}

void DataAcquisition::initSerialVacuum(const vacuumParameters &parameterVacuum){
    SerialPortInfo a;
    a.portName = parameterVacuum.m_portName;
    a.description = parameterVacuum.m_description;
    a.baudRate = QSerialPort::BaudRate(parameterVacuum.m_baudRate);
    a.dataBits = QSerialPort::DataBits(parameterVacuum.m_dataBits);
    a.parity = QSerialPort::Parity(parameterVacuum.m_parity);
    a.stopBits = QSerialPort::StopBits(parameterVacuum.m_stopBits);
    a.timeout = parameterVacuum.m_timeout;
    auto &reqVacuum = real().foreGauge();
    reqVacuum.setSerialPortInfo(a);
    if(!reqVacuum.openSerialPort()){
        qWarning() << "ДВ301: порт" << a.portName << "не открылся — опрос не запущен";
        return;
    }
    // gauge is polled by the controller own 1 s timer
    reqVacuum.startReading();
}

// ДВ302 — вакуумметр второго тракта (турбо-область). Тот же прибор, что раньше
// стоял как ДВ301, поэтому обмен ведёт TurboVacuumController. Опрашивает себя
// сам, как и ДВ301: период не привязан к такту softTimer приложения.
void DataAcquisition::initSerialTurboVacuum(const vacuumParameters &parameterVacuum){
    if(parameterVacuum.m_portName.isEmpty()){
        qWarning() << "ДВ302 не сконфигурирован — второй вакуумметр не опрашивается";
        return;
    }
    SerialPortInfo a;
    a.portName    = parameterVacuum.m_portName;
    a.description = parameterVacuum.m_description;
    a.baudRate    = QSerialPort::BaudRate(parameterVacuum.m_baudRate);
    a.dataBits    = QSerialPort::DataBits(parameterVacuum.m_dataBits);
    a.parity      = QSerialPort::Parity(parameterVacuum.m_parity);
    a.stopBits    = QSerialPort::StopBits(parameterVacuum.m_stopBits);
    a.timeout     = parameterVacuum.m_timeout;
    auto &reqVacuumTurbo = real().turboGauge();
    reqVacuumTurbo.setSerialPortInfo(a);
    if(!reqVacuumTurbo.openSerialPort()){
        qWarning() << "ДВ302: порт" << a.portName << "не открылся — опрос не запущен";
        return;
    }
    reqVacuumTurbo.startReading();
}

void DataAcquisition::testVacuumQuery(){
    if(m_real)
        m_real->foreGauge().requestData();
}

// Разовый запрос к ДВ302 для пусконаладки — без запуска режима.
void DataAcquisition::testTurboVacuumQuery(){
    if(m_real)
        m_real->turboGauge().requestData();
}


void DataAcquisition::startAcquisition(){
    if(!getGRAMsIntegrity()){
        qDebug() << "Reading disabled";
        return;
    }
    m_acquisitionTimer->setTimerType(Qt::PreciseTimer);
    m_acquisitionTimer->setInterval(500); // make default value
    m_acquisitionTimer->start();
}

void DataAcquisition::stopAcquisition(){
    m_acquisitionTimer->stop();
    // clear additionally
}

void DataAcquisition::processEvents(){
    if(!m_source || !m_source->isTemperatureConnected() || !m_source->isPressureConnected()){
        qDebug() << "Stopping acquisition...";
        stopAcquisition();
        return;
    }
    m_source->readTemperature();
    m_time->addValue(m_elapsedTimer.elapsed()/1000.0);  
    const auto &readDataPres = m_source->pressureVolts();
    for(int i = 0; i < m_pressureSensors.count(); ++i){
        m_pressureSensors[i]->addValue(readDataPres[i]); // ,0 minimal value
        m_filtersData[i]->setData(m_source->pressureBuffer(i));
    }
    const auto &readDataTemp = m_source->temperatures(); // this data from last read
    for(int i = 0; i < m_tempSensors.count(); ++i){
        m_tempSensors[i]->addValue(readDataTemp[i]);
    }
    if(canReadSlow){
        m_source->readPressure();
    }
    else if(!canReadFast){
        qDebug() << "Set Reading Fast";
        canReadFast = true;
    }
    
    if(m_vacuumSensor)
        m_vacuumSensor->addPoint(m_source->gaugeTorr(ISensorSource::Gauge::Fore),
                                 m_source->gaugeQuality(ISensorSource::Gauge::Fore));
    if(m_vacuumSensorTurbo)
        m_vacuumSensorTurbo->addPoint(m_source->gaugeTorr(ISensorSource::Gauge::Turbo),
                                      m_source->gaugeQuality(ISensorSource::Gauge::Turbo));
    
    if(m_leakageMeasure){
        fillLeakageRQ();
    }
}

void DataAcquisition::beginAction(){
    canReadSlow = false;
}

void DataAcquisition::endAction(){
    canReadSlow = true;
    canReadFast = false;
}

void DataAcquisition::fillSupplyARQ(){
}


void DataAcquisition::fastBufferRead(){
    if(!canReadFast)
        return;
    // qDebug() << "Reading Fast";
    if(m_source)
        m_source->readPressure();
}
// msecs

void DataAcquisition::setSupplyPressurePtr(FilterData* high, FilterData* low){
    m_supplyPressureHigh = high;
    m_supplyPressureLow = low;
}

void DataAcquisition::runSupplyAction(){
    QVector<double> filteredReal;
    int index = 0;
    const auto& filteredVoltage_high = m_source->pressureBuffer(index);
    const auto& lin_A_high = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_high = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_high){
        filteredReal << lin_A_high * val + lin_B_high;
    }
    m_supplyPressureHigh->addData(filteredReal);
    filteredReal.clear();
    index = 1;
    const auto& filteredVoltage_low = m_source->pressureBuffer(index);
    const auto& lin_A_low = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_low = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_low){
        filteredReal << lin_A_low * val + lin_B_low;
    }
    m_supplyPressureLow->addData(filteredReal);
}

bool DataAcquisition::setLeakageMeasure(bool leakageMeasure){
    if(!m_source || !m_source->isTemperatureConnected() || !m_source->isPressureConnected()){
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
    const auto& filteredVoltage_high = m_source->pressureBuffer(index);
    const auto& lin_A_high = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_high = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_high){
        filteredReal << lin_A_high * val + lin_B_high;
    }
    m_leakagePressureHigh->addData(filteredReal);
    filteredReal.clear();
    index = 3;
    const auto& filteredVoltage_low = m_source->pressureBuffer(index);
    const auto& lin_A_low = m_pressureSensors[index]->getLin_A();
    const auto& lin_B_low = m_pressureSensors[index]->getLin_B();
    for(const auto& val : filteredVoltage_low){
        filteredReal << lin_A_low * val + lin_B_low;
    }
    m_leakagePressureLow->addData(filteredReal);
}