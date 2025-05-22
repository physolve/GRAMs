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
    const double& chamberInitialPressure = 1;
    const auto& intermediateChangeReaction = m_rQ->getChangeToIntermediateTarget(chamberInitialPressure);
    qDebug() << "Current EF " << intermediateChangeReaction.currentPressure; // EF?
    qDebug() << "Target intermediate EF " << intermediateChangeReaction.targetPressure; // EF?
    qDebug() << "Moles change intermediate EF " << intermediateChangeReaction.molesChange; // EF?
    
    const auto& targetReaction = m_rQ->getChangeToTarget(chamberPressureTarget, chamberInitialPressure);
    // Случай получения только одного запаса до chamberPressureTarget
    // Начиная с целевого 
    const double& storagePressureTarget = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange);
    // Какое количество запасных подач подать в storage чтобы хватило?
    // Какое давление целевое с использованием C1?
    qDebug() << "Target B " << storagePressureTarget;
    const double& storagePressureTargetWithC1 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Small);
    const double& storagePressureTargetWithC2 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Medium);
    const double& storagePressureTargetWithC3 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Large);
    qDebug() << "Target B with C1 " << storagePressureTargetWithC1;
    qDebug() << "Target B with C2 " << storagePressureTargetWithC2;
    qDebug() << "Target B with C3 " << storagePressureTargetWithC3;
    // Еси добавить molesChange в банки будет по x дополнительный запас
    // Попробуй: по 1 запас в B, С3, С2, С1 по очереди закрывая (C1, C2, C3)
    const auto& intermediateChangeOnlyStorage = m_sQ->getChangeToIntermediateTarget(chamberPressureTarget);
    qDebug() << "Current B " << intermediateChangeOnlyStorage.currentPressure;
    qDebug() << "Target intermediate B " << intermediateChangeOnlyStorage.targetPressure;
    qDebug() << "Moles change intermediate B " << intermediateChangeOnlyStorage.molesChange;
    const auto& intermediateChangeStorageWithSmall = m_sQ->getChangeToIntermediateTarget(chamberPressureTarget, addCVolume::Small);
    qDebug() << "Moles change intermediate C1 " << intermediateChangeStorageWithSmall.molesChange;
    const auto& intermediateChangeStorageWithMedium = m_sQ->getChangeToIntermediateTarget(chamberPressureTarget, addCVolume::Medium);
    qDebug() << "Moles change intermediate C2 " << intermediateChangeStorageWithMedium.molesChange;
    const auto& intermediateChangeStorageWithLarge = m_sQ->getChangeToIntermediateTarget(chamberPressureTarget, addCVolume::Large);
    qDebug() << "Moles change intermediate C3 " << intermediateChangeStorageWithLarge.molesChange;

    const auto& targetStorage = m_sQ->getChangeToTarget(storagePressureTarget, chamberPressureTarget);
    qDebug() << "Intermediate B " << targetStorage.currentPressure;
    qDebug() << "Target B " << targetStorage.targetPressure;
    qDebug() << "Moles change B " << targetStorage.molesChange;
    const auto& targetStorageWithSmall = m_sQ->getChangeToTarget(storagePressureTargetWithC1, chamberPressureTarget, addCVolume::Small);
    const auto& targetStorageWithMedium = m_sQ->getChangeToTarget(storagePressureTargetWithC2, chamberPressureTarget, addCVolume::Medium);
    const auto& targetStorageWithLarge = m_sQ->getChangeToTarget(storagePressureTargetWithC3, chamberPressureTarget, addCVolume::Large);

    const auto& c1PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithSmall.molesChange, addCVolume::Small);
    const auto& c2PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithMedium.molesChange, addCVolume::Medium);
    const auto& c3PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithLarge.molesChange, addCVolume::Large);
    qDebug() << "Target with C1 after intermediate " << c1PressureTarget;
    qDebug() << "Target with C2 after intermediate " << c2PressureTarget;
    qDebug() << "Target with C3 after intermediate " << c3PressureTarget;
}

void PlayPressure::playWithAccuum(){
    const double& chamberPressureTarget = 5;
    const double& chamberInitialPressure = 1;
    const auto& intermediateChangeReaction = m_rQ->getChangeToIntermediateTarget(chamberInitialPressure);
    // Reaction currentPressure and molesChange to target pressure 
    // targetReaction.molesChange;
    // targetReaction. 
    const auto& targetReaction = m_rQ->getChangeToTarget(chamberPressureTarget, chamberInitialPressure);
    // Случай получения только n запасов до chamberPressureTarget
    const auto& nAccum = 5.1;
    // Чтобы получить 3 всего только из B подачи газа нужно 
    const auto& molesChangeWithAccum = nAccum*targetReaction.molesChange;
    const double& storagePressureTarget = chamberPressureTarget + m_sQ->getTargetFromMolesChange(molesChangeWithAccum);
    qDebug() << "Target B with accum " << storagePressureTarget;
}