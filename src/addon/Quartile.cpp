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
    VolumeObject volumeObject{name, volume, 1, 300}; // default values
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

VolumeObject Quartile::getVolumeByName(const QString& name) const{
    return m_volumeObjects[name];
}

void Quartile::fillVolumePairs(const QMap<QString,QString>& volumeToValve){
    // same as in storage quartile, make as quartile class method
    for(const auto& [volume, valve] : volumeToValve.asKeyValueRange()){
        int index;
        for(index = 0; index < m_valves.count(); ++index){
            if(m_valves[index]->m_name == valve)
                break;
        }
        // m_valveToVolumeList.append({index, volume});
        m_valveToVolumeMap[index] = volume;
    }
}
