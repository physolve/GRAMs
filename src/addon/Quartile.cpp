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

void Quartile::addVolume(const QString& name, double volume){
    VolumeObject volumeObject{name, volume, 1, 300};
    // volumeObject.name = name;
    // volumeObject.volume = volume;
    m_volumeObjects[name] = volumeObject;
}

void Quartile::addValvePtrs(Valve ptr[], int valvesCnt){
    for(int i = 0; i < valvesCnt; ++i){
        m_valves << &ptr[i];
    }
    // m_valvesCnt = valvesCnt;
}

void Quartile::addPressurePtrs(ControllerData ptr[], int pressureCnt){
    for(int i = 0; i < pressureCnt; ++i){
        m_pressureList << &ptr[i];
    }
}

void Quartile::addTemperaturePtrs(ControllerData ptr[], int temperatureCnt){
    for(int i = 0; i < temperatureCnt; ++i){
        m_temperatureList << &ptr[i];
    }
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

int AddRemoveQuartile::getLightPlotPtr(LightPlotItem* lightPlotPointer){
    switch(m_supplyPressurePlots.count()){
        // better rewrite using node pressure
        case 0:
        {
            FilterData* chartPtrs[1] = {m_supplyPressureHigh};
            lightPlotPointer->setDataPointers(chartPtrs, 1);
            break;
        }
        case 1:
        {
            FilterData* chartPtrs[1] = {m_supplyPressureLow};
            lightPlotPointer->setDataPointers(chartPtrs, 1);
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
        m_supplyPort[m_currentSupplyPort].startCalc(m_supplyPressureHigh->getCurValue()); // replace to quartile_pressure
        
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
    m_supplyPort[m_currentSupplyPort].addMeasure(m_supplyPressureHigh->getCurValue()); // replace to quartile_pressure
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

void StorageQuartile::setCVolumePtr(DataCollection ptr[], int cVolumeCnt){
    for(int i = 0; i < cVolumeCnt; ++i){
        cVolumePressure << &ptr[i];
    }
}

void StorageQuartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){
    // gas store valves   
    m_pressureNodes.insert(nodeName, NodePressure());
    m_pressureNodes[nodeName].setVolumeA(&m_volumeObjects[volA]);
    m_pressureNodes[nodeName].setVolumeB(&m_volumeObjects[volB]);
    // recalculate node? and moles?
}

void StorageQuartile::setIndexPressureRange(int index){
    v_pressure_range = index;
}

void StorageQuartile::setIndexPressureHighLow(int indexHigh, int indexLow){
    s_pressure_high = indexHigh;
    s_pressure_low = indexLow;
}

void StorageQuartile::setIndexTemperatureMain(int index){
    s_temperature_main = index;
}

void StorageQuartile::updateQuartileData(){
    double current_pressure = 0.0;
    if(m_valves[v_pressure_range]->getState()){
        current_pressure = m_pressureList[s_pressure_high]->getCurValue();    
    }
    else{
        current_pressure = m_pressureList[s_pressure_low]->getCurValue();
    }
    pressureStorageQuartile->addPoint(current_pressure);
    temperatureStorageQuartile->addPoint(m_temperatureList[s_temperature_main]->getCurValue());
    // put m_volumeObjects valve state
}

void StorageQuartile::updatePressureNodes(){
    // valve states to set volumeObject pressure
    for(auto& volume : m_volumeObjects){
        if()
        volume.pressure = pressureStorageQuartile->getCurValue();
        volume.temperature = temperatureStorageQuartile->getCurValue();
    }
    for(auto& node : m_pressureNodes){
        node.getEquilibrium
    }
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


