#include "Chamber.h"

#include <QDebug>

Chamber::Chamber(QObject *parent) :
    QObject(parent), statusOpen{false}
{
    qDebug() << "Chamber class is created";
    // take info on chamber
}

Chamber::~Chamber(){
    qDebug() << "Chamber class is destroyed";
}

void Chamber::setChamberVolume(const VolumeObject& chamberVolume){
    m_chamberVolume = chamberVolume;
    m_chamberParams.m_sensorName = m_chamberVolume.name;
    m_chamberParams.m_volume = m_chamberVolume.volume;
    emit chamberParamsChanged();
}

void Chamber::setStatusOpen(bool status){
    statusOpen = status;
}

bool Chamber::getStatusOpen() const{
    return statusOpen;
}

ChamberParameters Chamber::getChamberParams() const {
    return m_chamberParams;
}



