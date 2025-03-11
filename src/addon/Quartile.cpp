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

AddRemoveQuartile::AddRemoveQuartile(QObject *parent) : Quartile(parent){

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
        case 0:
        {
            DataCollection* chartPtrs[2] = {&pseudo_time, m_supplyPressureHigh};
            lightPlotPointer->setDataPointers(chartPtrs, 2);
            break;
        }
        case 1:
        {
            DataCollection* chartPtrs[2] = {&pseudo_time, m_supplyPressureLow};
            lightPlotPointer->setDataPointers(chartPtrs, 2);
            break;
        }
        default:
            qDebug() << "default\n"; // no error
            break;
    }
    lightPlotPointer->initCustomPlot();
    lightPlotPointer->placeGraph();
    lightPlotPointer->dataSetUpdated();
    m_supplyPressurePlots << lightPlotPointer;
    return m_supplyPressurePlots.count() - 1;
}
void AddRemoveQuartile::startSupplyMeasure(int currentSupplyPort){
    m_currentSupplyPort = currentSupplyPort;

}
void AddRemoveQuartile::fillSupplyPortData(){
    const auto& time = pseudo_time.getCurValue();
    QVector<double> indexTime;
    for(int i = 1; i <= 512; i++){
        indexTime << time + i; 
    }
    pseudo_time.addData(indexTime);
    switch(m_currentSupplyPort){
        case 0:
        {
            m_supplyPressurePlots[0]->dataUpdated();

            break;
        }
        case 1:
        {
            m_supplyPressurePlots[1]->dataUpdated();

            break;
        }
    }
}
void AddRemoveQuartile::stopSupplyMeasure(){
    
    //saves
    m_supplyPort[m_currentSupplyPort].saveResultsToFile();
    //clear
    m_currentSupplyPort = -1;
    pseudo_time.clearCumulative();
    m_supplyPressureHigh->clearCumulative();
    m_supplyPressureLow->clearCumulative();
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

void StorageQuartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){
    // gas store valves   
    m_pressureNodes.insert(nodeName, NodePressure());
    m_pressureNodes[nodeName].setVolumeA(&m_volumeObjects[volA]);
    m_pressureNodes[nodeName].setVolumeB(&m_volumeObjects[volB]);
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


