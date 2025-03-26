#include "Quartile.h"

#include <QDebug>

Quartile::Quartile(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Quartile class is created";
}

Quartile::~Quartile(){
    qDebug() << "Quartile class is destroyed";
}

void Quartile::setVolume(double volume){
    m_volume = volume;
}

void Quartile::setMainVolume(const QString& name){
    m_mainVolume = name;
}

void Quartile::addVolume(const QString& name, double volume){
    VolumeObject volumeObject{name, volume, 1, 300};
    // volumeObject.name = name;
    // volumeObject.volume = volume;
    m_volumeObjects[name] = volumeObject;
}

void Quartile::addValvePtrs(const QVector<Valve*>& ptr){
    m_valves = ptr;
}

void Quartile::addPressurePtrs(const QVector<ControllerData*>& ptr){
    m_pressureList = ptr;
}

void Quartile::addTemperaturePtrs(const QVector<ControllerData*>& ptr){
    m_temperatureList = ptr;
}

void Quartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){

}

AddRemoveQuartile::AddRemoveQuartile(QObject *parent) : Quartile(parent), m_expUpdate(new QTimer){
    for(int i{2}; i >= 0; --i){
        m_supplyPort[i].setInitialParametersSupply(i,0,1);
    }
    connect(m_expUpdate, &QTimer::timeout, this, &AddRemoveQuartile::expEvent);
    m_expUpdate->setInterval(500);
}
AddRemoveQuartile::~AddRemoveQuartile(){
    m_supplyPressureHigh = nullptr;
    m_supplyPressureLow = nullptr;
}

void AddRemoveQuartile::setSupplyPressurePtr(FilterData* high, FilterData* low){
    m_supplyPressureHigh = high;
    m_supplyPressureLow = low;
}

void AddRemoveQuartile::setStorageQuartilePtr(Quartile* storageQuartile){
    m_storageQuartile = storageQuartile;
}

void AddRemoveQuartile::setStorageQuartilePressure(QuartileData* storageQuartilePressure){
    m_storageQuartilePressure = storageQuartilePressure;
}

int AddRemoveQuartile::getLightPlotPtr(LightPlotItem* lightPlotPointer){
    QVector<FilterData*> chartPtrs;
    switch(m_supplyPressurePlots.count()){
        // i don't have quartile filter data
        case 0:
        {
            chartPtrs.append(m_supplyPressureHigh);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        case 1:
        {
            chartPtrs.append(m_supplyPressureLow);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        default:
            qDebug() << "default\n"; // no error
            break;
    }
    lightPlotPointer->initCustomPlot();
    lightPlotPointer->placeGraph();
    m_supplyPressurePlots << lightPlotPointer;
    return m_supplyPressurePlots.count() - 1;
}

void AddRemoveQuartile::setSupplyAdjustParameters(QVariantMap parameters){
    m_currentSupplyPort = 2 - parameters["supplyPort"].toInt();
    const auto& turn = parameters["turn"].toDouble();
    const auto& portPressure = parameters["portPressure"].toDouble();
    m_supplyPort[m_currentSupplyPort].setInitialParametersSupply(m_currentSupplyPort, turn, portPressure);
    qDebug() << "Begin supply with parameters:" << QString("%1 %2 %3").arg(m_currentSupplyPort).arg(turn).arg(portPressure);
}

void AddRemoveQuartile::startSupplyMeasure(bool measure){
    if(measure){
        qDebug() << "Current supply port state: " << m_valves[m_currentSupplyPort]->getState();
        m_supplyPort[m_currentSupplyPort].initResultFile();
        // initial_flow = quartile_storage->moles to std cm3
        const auto& initial_flow = static_cast<StorageQuartile*>(m_storageQuartile)->getQuartileMoleVolume();
        m_supplyPort[m_currentSupplyPort].startCalc(m_storageQuartilePressure->getCurValue(), initial_flow);
        m_supplyPressurePlots[0]->initPlotData();
        m_expUpdate->start();
    }
    else{
        m_expUpdate->stop();
        //saves
        m_supplyPort[m_currentSupplyPort].saveResultsToFile();
        //clear
        m_supplyPort[m_currentSupplyPort].endCalc();

        m_supplyPressurePlots[0]->savePlotData();
        m_supplyPressurePlots[0]->clearPlotData();
        m_currentSupplyPort = -1;
    }
}

void AddRemoveQuartile::expEvent(){
    fillSupplyPortData();
}

void AddRemoveQuartile::updatePortState(){
    // update all ports
    for(int i = 0; i < m_valves.count(); ++i){ // if drain ptr!
        m_supplyPort[i].setPortOpen(m_valves[i]->getState());
    }
}

void AddRemoveQuartile::fillSupplyPortData(){
    switch(m_currentSupplyPort){
        // this is supply port for low pressure
        // better rewrite using node pressure
        case 2:
        {
            // but graph update values from m_supplyPressureLow
            m_supplyPressurePlots[1]->dataUpdated();
            break;
        }
        case 1: // this is supply port for high pressure
        {
            // but this graph update values from m_supplyPressureHigh
            m_supplyPressurePlots[0]->dataUpdated();
            break;
        }
        default:{
            break;
        }
    }
    m_supplyPort[m_currentSupplyPort].setPortOpen(m_valves[m_currentSupplyPort]->getState());
    m_supplyPort[m_currentSupplyPort].addMeasure(m_storageQuartilePressure->getCurValue());
}

StorageQuartile::StorageQuartile(QObject *parent) :
    Quartile(parent){
        
}
StorageQuartile::~StorageQuartile(){

}

void StorageQuartile::calculateTotalVolume(){
    double totalVolume = 0;
    for(const auto& volume : m_volumeObjects){
        totalVolume += volume.volume;
    }
    setVolume(totalVolume);
}

void StorageQuartile::setQuartileDataPressure(QuartileData* quartileData){
    pressureStorageQuartile = quartileData;
}

void StorageQuartile::setQuartileDataTemperature(QuartileData* quartileData){
    temperatureStorageQuartile = quartileData;
}

void StorageQuartile::setCVolumePtr(const QVector<DataCollection*>& ptr){
    cVolumePressure = ptr;
}

void StorageQuartile::setBD1VolumePtr(DataCollection* ptrB, DataCollection* ptrD1){
    bVolumePressure = ptrB;
    d1VolumePressure = ptrD1;
}

void StorageQuartile::setMolesPtr(const QVector<MolesData*>& ptr){
    m_molesDataList = ptr;
}

void StorageQuartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){
    // gas store valves   
    m_pressureNodes.insert(nodeName, NodePressure());
    m_pressureNodes[nodeName].setVolumeA(&m_volumeObjects[volA]);
    m_pressureNodes[nodeName].setVolumeB(&m_volumeObjects[volB]);
    // recalculate node? and moles?
}

void StorageQuartile::setIndexValveRange(int index){
    v_pressure_range = index;
}

void StorageQuartile::setIndexPressureHighLow(int indexHigh, int indexLow){
    s_pressure_high = indexHigh;
    s_pressure_low = indexLow;
}

void StorageQuartile::setIndexTemperatureMain(int index){
    s_temperature_main = index;
}

void StorageQuartile::fillVolumePairs(const QMap<QString,QString>& volumeToValve){
    for(const auto& [volume, valve] : volumeToValve.asKeyValueRange()){
        int index;
        for(index = 0; index < m_valves.count(); ++index){
            if(m_valves[index]->m_name == valve)
                break;
        }
        m_valveToVolumeList.append({index, volume});
    }
}

void StorageQuartile::updateQuartileData(){
    double current_pressure = 0.0;
    // write smooth transition
    if(m_valves[v_pressure_range]->getState()){
        current_pressure = m_pressureList[s_pressure_low]->getCurValue();
    }
    else{
        current_pressure = m_pressureList[s_pressure_high]->getCurValue();
    }
    pressureStorageQuartile->addPoint(current_pressure);
    temperatureStorageQuartile->addPoint(m_temperatureList[s_temperature_main]->getCurValue());
    // put m_volumeObjects valve state
    Valve* c_volume_valves[3] = {m_valves[0], m_valves[1], m_valves[2]}; // use pairs
    for(int i = 0; i < cVolumePressure.count(); ++i){
        if(c_volume_valves[i]->getState()){
            cVolumePressure[i]->addPoint(current_pressure);
        }
        else{
            cVolumePressure[i]->addPoint(cVolumePressure[i]->getCurValue());
        }
    }
    if(m_valves[v_pressure_range]->getState()){
        d1VolumePressure->addPoint(current_pressure);
    }
    else{
        d1VolumePressure->addPoint(m_pressureList[s_pressure_low]->getCurValue());
    }
    bVolumePressure->addPoint(current_pressure);
    updateVolumeObjects();
    updateMoles();
}

void StorageQuartile::updateVolumeObjects(){
    // valve states to set volumeObject pressure
    m_volumeObjects["C1"].pressure = cVolumePressure[0]->getCurValue();
    m_volumeObjects["C1"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["C2"].pressure = cVolumePressure[1]->getCurValue();
    m_volumeObjects["C2"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["C3"].pressure = cVolumePressure[2]->getCurValue();
    m_volumeObjects["C3"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["B"].pressure = bVolumePressure->getCurValue();
    m_volumeObjects["B"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["D1"].pressure = d1VolumePressure->getCurValue();
    m_volumeObjects["D1"].temperature = temperatureStorageQuartile->getCurValue();
}

void StorageQuartile::updateMoles(){
    for(auto moleSensor : m_molesDataList){
        moleSensor->addPoint(m_volumeObjects[moleSensor->m_name].getMoles());
    }
}

double StorageQuartile::getQuartileMole() const{
    double moles = 0;
    moles+=m_volumeObjects[m_mainVolume].getMoles();
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            moles+=m_volumeObjects[valveToVolume.second].getMoles();
        }
    }
    return moles;
}

double StorageQuartile::getQuartileMoleVolume() const{
    const auto& moles = getQuartileMole();
    return moles*temperature_std_K*gas_constant*10/pressure_std_bar; // moles * K *Jl/(mol*K) / (10^5*bar) -> m3 -> * 10^6 -> cm3
}

ReactionQuartile::ReactionQuartile(QObject *parent) :
    Quartile(parent){       
}
ReactionQuartile::~ReactionQuartile(){

}

void ReactionQuartile::calculateTotalVolume(){
    double totalVolume = 0;
    for(const auto& volume : m_volumeObjects){
        totalVolume += volume.volume;
    }
    setVolume(totalVolume);
}

void ReactionQuartile::setChamber(const QString& chamber){
    profileChamber = chamber;
}

SecondLineQuartile::SecondLineQuartile(QObject *parent) :
    Quartile(parent){
        
}
SecondLineQuartile::~SecondLineQuartile(){

}


