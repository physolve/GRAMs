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

AddRemoveQuartile::AddRemoveQuartile(QObject *parent) :
    Quartile(parent){
        
}
AddRemoveQuartile::~AddRemoveQuartile(){   
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


