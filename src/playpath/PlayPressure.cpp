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

    // Случай получения только одного запаса до chamberPressureTarget
    // Начиная с целевого 
    const double& storagePressureTarget = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange);
    // Какое количество запасных подач подать в storage чтобы хватило?
    // Какое давление целевое с использованием C1?
    const double& storagePressureTargetWithC1 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Small);
    const double& storagePressureTargetWithC2 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Medium);
    const double& storagePressureTargetWithC3 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Large);
    // Еси добавить molesChange в банки будет по x дополнительный запас
    // Попробуй: по 1 запас в B, С3, С2, С1 по очереди закрывая (C1, C2, C3)
    const auto& targetStorage = m_sQ->getChangeToTarget(storagePressureTarget);
    qDebug() << "Current B" << targetStorage.currentPressure;
    qDebug() << "Target B" << targetStorage.targetPressure;
    qDebug() << "Moles change B" << targetStorage.molesChange;
}


