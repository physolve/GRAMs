#include "PlayPressure.h"

#include <QDebug>

PlayPressure::PlayPressure(QObject *parent) :
    QObject(parent), m_guiPresTarget{5,1,0,1.0,0,0,0,0}
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
    m_guiPresTarget.m_c1PressureTarget = 0;
    m_guiPresTarget.m_c2PressureTarget = 0;
    m_guiPresTarget.m_c3PressureTarget = 0;
    // const double& chamberPressureTarget = 5;
    const double& chamberPressureTarget = m_guiPresTarget.m_chamberPressureTarget;
    
    const double& chamberInitialPressure = m_guiPresTarget.m_chamberInitialPressure; // will be
    const auto& intermediateChangeReaction = m_rQ->getChangeToIntermediateTarget(chamberInitialPressure);

    const auto& targetReaction = m_rQ->getChangeToTarget(chamberPressureTarget, chamberInitialPressure);
    // Случай получения только n запасов до chamberPressureTarget
    // m_guiPresTarget.m_nAccum = 5.1;
    // Чтобы получить 3 всего только из B подачи газа нужно 
    
    const double& storagePressureTarget = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange);
    const double& supplyMax = 50; // from profile 
    // Если целевое давление больше, чем supplyMax, то сначала распространяй по банкам
    // Если с максимальной банкой больше, чем supplyMax пробуй несколько банок
    // Иначе, определить как в n подач  
    // Выбираем одну или никакую
    // Условие меньший объем и до supplyMax
    const auto& targetStorage = m_sQ->getChangeToTarget(storagePressureTarget, chamberPressureTarget);
    
    // const auto& targetStorageWithSmall = m_sQ->getChangeToTarget(storagePressureTargetWithC1, chamberPressureTarget, addCVolume::Small);
    // const auto& targetStorageWithMedium = m_sQ->getChangeToTarget(storagePressureTargetWithC2, chamberPressureTarget, addCVolume::Medium);
    // const auto& targetStorageWithLarge = m_sQ->getChangeToTarget(storagePressureTargetWithC3, chamberPressureTarget, addCVolume::Large);

    // Накапливай в по очереди в каждый, когда n > 1, пока < supplyMax
    if(m_guiPresTarget.m_nAccum > 1.1){
        double nAccumLeft = m_guiPresTarget.m_nAccum;
        double nAccumB = 0;
        double nAccumC1 = 0;
        double nAccumC2 = 0;
        double nAccumC3 = 0;
        int i = 0;
        while(nAccumLeft>1){
            nAccumLeft--;
            switch(i){
                case 0: nAccumB++; break; // проверяй на возможность превышения
                case 1: nAccumC1++; break;
                case 2: nAccumC2++; break;
                case 3: nAccumC3++; break; 
            }
            i++;
            if(i==4){
                i=0;
            }
        }
        nAccumB+=nAccumLeft;
        // const auto& molesChangeWithAccum = m_guiPresTarget.m_nAccum*targetReaction.molesChange;
        const double& storagePressureTargetWithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange(nAccumB*targetReaction.molesChange);
        m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithAccum;
        // по очереди разделяй m_nAccum
        if(nAccumC1>0){
            const double& storagePressureTargetWithC1 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(nAccumC1*targetReaction.molesChange, addCVolume::Small);
            const auto& targetStorageWithSmall = m_sQ->getChangeToTarget(storagePressureTargetWithC1, chamberPressureTarget, addCVolume::Small);
            m_guiPresTarget.m_c1PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithSmall.molesChange, addCVolume::Small);
        }
        if(nAccumC2>0){
            const double& storagePressureTargetWithC2 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(nAccumC2*targetReaction.molesChange, addCVolume::Medium);
            const auto& targetStorageWithMedium = m_sQ->getChangeToTarget(storagePressureTargetWithC2, chamberPressureTarget, addCVolume::Medium);
            m_guiPresTarget.m_c2PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithMedium.molesChange, addCVolume::Medium);    
        }
        if(nAccumC3>0){
            const double& storagePressureTargetWithC3 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(nAccumC3*targetReaction.molesChange, addCVolume::Large);
            const auto& targetStorageWithLarge = m_sQ->getChangeToTarget(storagePressureTargetWithC3, chamberPressureTarget, addCVolume::Large);
            m_guiPresTarget.m_c3PressureTarget = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithLarge.molesChange, addCVolume::Large);
        }
        // количество напусков
        m_guiPresTarget.m_supplyCount = (int)(m_guiPresTarget.m_storagePressureTarget/supplyMax)+1;
    }
    else{
        if(storagePressureTarget > supplyMax){
            const double& storagePressureTargetWithC1 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Small);
            if(storagePressureTargetWithC1 > supplyMax){
                const double& storagePressureTargetWithC2 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Medium);
                if(storagePressureTargetWithC2 > supplyMax){
                    const double& storagePressureTargetWithC3 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Large);
                    if(storagePressureTargetWithC3 > supplyMax){
                        //impossible to get for one supply
                        m_guiPresTarget.m_storagePressureTarget = supplyMax;
                        m_guiPresTarget.m_c3PressureTarget = supplyMax;
                    }
                    else{
                        m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC3;
                        m_guiPresTarget.m_c3PressureTarget = storagePressureTargetWithC3;
                    }
                }
                else{
                    m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC2;
                    m_guiPresTarget.m_c2PressureTarget = storagePressureTargetWithC2;
                }
            }
            else{
                m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC1;
                m_guiPresTarget.m_c1PressureTarget = storagePressureTargetWithC1;
            } 
        }
        else{
            m_guiPresTarget.m_storagePressureTarget = storagePressureTarget;
        }
    }
    emit guiPresTargetChanged();
}

void PlayPressure::setGuiPresTarget(guiPressureTarget guiPresTarget){
    m_guiPresTarget = guiPresTarget;
}

guiPressureTarget PlayPressure::getGuiPresTarget(){
    return m_guiPresTarget;
}