#include "Chamber.h"

#include <QDebug>

Chamber::Chamber(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Chamber class is created";
    m_chamberParams.m_chamberName = "unknown";
    m_chamberParams.m_volume = 0;
    m_chamberParams.m_status = false;
    // take info on chamber
}

Chamber::~Chamber(){
    qDebug() << "Chamber class is destroyed";
}

void Chamber::setChamberVolume(const VolumeObject& chamberVolume){
    m_chamberVolume = chamberVolume;
    m_chamberParams.m_chamberName = m_chamberVolume.name;
    m_chamberParams.m_volume = m_chamberVolume.volume;
    m_chamberParams.m_status = true; // by default
    emit chamberParamsChanged();
}

void Chamber::setCraneToChamber(double craneToChamber){
    m_chamberParams.m_craneToChamber = craneToChamber;
    m_craneVolume.name = "crane";
    m_craneVolume.volume = craneToChamber;
}

void Chamber::setStatusOpen(bool status){
    m_chamberParams.m_status = status;
    emit chamberParamsChanged();
}

bool Chamber::getStatusOpen() const{
    return m_chamberParams.m_status;
}

ChamberParameters Chamber::getChamberParams() const {
    return m_chamberParams;
}

VolumeObject Chamber::getVolumeObject() const{
    return m_chamberVolume;
}

VolumeObject Chamber::getCraneObject() const{
    return m_craneVolume;
}

