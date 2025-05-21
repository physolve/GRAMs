#include "PlayPressure.h"

#include <QDebug>

PlayPressure::PlayPressure(QObject *parent) :
    QObject(parent)
{
    qDebug() << "PlayPressure class is created";

}

PlayPressure::~PlayPressure(){
    qDebug() << "PlayPressure class is destroyed";
}

void PlayPressure::setARQ(AddRemoveQuartile* arQ){
    m_arQ = arQ;
}

void PlayPressure::setSQ(StorageQuartile* sQ){
    m_sQ = sQ;
}

void PlayPressure::setRQ(ReactionQuartile* rQ){
    m_rQ = rQ;
}

void PlayPressure::play(){
    const double& chamberPressureTarget = 5;
    const auto& targetReaction = m_rQ->getChangeToTarget(chamberPressureTarget);
    // Reaction currentPressure and molesChange to target pressure 
    // targetReaction.molesChange;
    // targetReaction. 
    const double& storagePressureTarget = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange);
    const auto& targetStorage = m_sQ->getChangeToTarget(storagePressureTarget);
    qDebug() << "Current B" << targetStorage.currentPressure;
    qDebug() << "Target B" << targetStorage.targetPressure;
    qDebug() << "Moles change B" << targetStorage.molesChange;
}


