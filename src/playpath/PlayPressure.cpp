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

void PlayPressure::testPlayPressure(){
    // play();
    playWithAccuum();
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
    m_guiPresTarget.m_storagePressureTarget = 0;
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
    const double& storagePressureTargetWithC1 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Small);
    const double& storagePressureTargetWithC2 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Medium);
    const double& storagePressureTargetWithC3 = chamberPressureTarget + m_sQ->getTargetFromMolesChange(targetReaction.molesChange, addCVolume::Large);

    const double& supplyMaxMain = 52; // from profile 
    const double& supplyMax = 50; // from profile 
    // Если целевое давление больше, чем supplyMax, то сначала распространяй по банкам
    // Если с максимальной банкой больше, чем supplyMax пробуй несколько банок
    // Иначе, определить как в n подач  
    // Выбираем одну или никакую
    // Условие меньший объем и до supplyMax

    // const auto& targetStorage = m_sQ->getChangeToTarget(storagePressureTarget, chamberPressureTarget);
    // const auto& targetStorageWithSmall = m_sQ->getChangeToTarget(storagePressureTargetWithC1, chamberPressureTarget, addCVolume::Small);
    // const auto& targetStorageWithMedium = m_sQ->getChangeToTarget(storagePressureTargetWithC2, chamberPressureTarget, addCVolume::Medium);
    // const auto& targetStorageWithLarge = m_sQ->getChangeToTarget(storagePressureTargetWithC3, chamberPressureTarget, addCVolume::Large);

    double nAccumLeft = m_guiPresTarget.m_nAccum;
    double nAccumB = 0;
    double nAccumC1 = 0;
    double nAccumC2 = 0;
    double nAccumC3 = 0;
    
    double change = 1;

    double storagePressureTargetWithAccum = 0;
    double storagePressureTargetC1WithAccum = 0;
    double storagePressureTargetC2WithAccum = 0;
    double storagePressureTargetC3WithAccum = 0;

    while(nAccumLeft>0){
        if(nAccumLeft<2){
            change = nAccumLeft;
        }
        storagePressureTargetWithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumB+change)*targetReaction.molesChange);
        if(storagePressureTargetWithAccum <= supplyMaxMain){
            nAccumB+=change;
            nAccumLeft-=change;
            m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithAccum;
            continue;
        }

        if(nAccumB==0){
            storagePressureTargetC1WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC1+change)*targetReaction.molesChange, addCVolume::Small);
        }
        else{
            storagePressureTargetC1WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC1+change)*targetReaction.molesChange, addCVolume::Small);
            const auto& targetStorageWithSmall = m_sQ->getChangeToTarget(storagePressureTargetC1WithAccum, chamberPressureTarget, addCVolume::Small);
            storagePressureTargetC1WithAccum = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithSmall.molesChange, addCVolume::Small);
        }
        if(storagePressureTargetC1WithAccum <= supplyMax){
            nAccumC1+=change;
            nAccumLeft-=change;
            m_guiPresTarget.m_c1PressureTarget = storagePressureTargetC1WithAccum;
            continue;
        }

        if(nAccumB==0 && nAccumC1==0){
            storagePressureTargetC2WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC2+change)*targetReaction.molesChange, addCVolume::Medium);
        }
        else{
            storagePressureTargetC2WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC2+change)*targetReaction.molesChange, addCVolume::Medium);
            const auto& targetStorageWithMedium = m_sQ->getChangeToTarget(storagePressureTargetC2WithAccum, chamberPressureTarget, addCVolume::Medium);
            storagePressureTargetC2WithAccum = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithMedium.molesChange, addCVolume::Medium);
        }
        if(storagePressureTargetC2WithAccum <= supplyMax){
            nAccumC2+=change;
            nAccumLeft-=change;
            m_guiPresTarget.m_c2PressureTarget = storagePressureTargetC2WithAccum;
            continue;           
        }

        if(nAccumB==0 && nAccumC1==0 && nAccumC2==0){
            storagePressureTargetC3WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC3+change)*targetReaction.molesChange, addCVolume::Large);
        }
        else{
            storagePressureTargetC3WithAccum = chamberPressureTarget + m_sQ->getTargetFromMolesChange((nAccumC3+change)*targetReaction.molesChange, addCVolume::Large);
            const auto& targetStorageWithLarge = m_sQ->getChangeToTarget(storagePressureTargetC3WithAccum, chamberPressureTarget, addCVolume::Large);
            storagePressureTargetC3WithAccum = chamberPressureTarget + m_sQ->getTargetCVolumeFromMolesChange(targetStorageWithLarge.molesChange, addCVolume::Large);
        }
        if(storagePressureTargetC3WithAccum <= supplyMax){
            nAccumC3+=change;
            nAccumLeft-=change;
            m_guiPresTarget.m_c3PressureTarget = storagePressureTargetC3WithAccum;
            continue;    
        }
        else{
            // incorrect
            // const auto& supplyMaxEnd = m_sQ->getChangeToTarget(supplyMax, chamberPressureTarget, addCVolume::Large);
            // const auto& nAccumByMaxSuppply = supplyMaxEnd.molesChange/targetReaction.molesChange; 
            // nAccumLeft+= nAccumC3;
            // nAccumLeft-= nAccumByMaxSuppply;
            // nAccumC3 = nAccumByMaxSuppply;
            // m_guiPresTarget.m_c3PressureTarget = supplyMax;
            break;
        } 
    }
    if(nAccumB==0 && nAccumC1==0 && nAccumC2==0){
        m_guiPresTarget.m_storagePressureTarget = m_guiPresTarget.m_c3PressureTarget;
    }
    else if(nAccumB==0 && nAccumC1==0){
        m_guiPresTarget.m_storagePressureTarget = m_guiPresTarget.m_c2PressureTarget;
    }
    else if(nAccumB==0){
        m_guiPresTarget.m_storagePressureTarget = m_guiPresTarget.m_c1PressureTarget;
    }
    // количество напусков
    if(nAccumLeft>0){
        qDebug() << m_guiPresTarget.m_nAccum*targetReaction.molesChange << " нужно";
        qDebug() << nAccumB*targetReaction.molesChange << " в B макс";
        qDebug() << nAccumC1*targetReaction.molesChange << " в C1 макс";
        qDebug() << nAccumC2*targetReaction.molesChange << " в C2 макс";
        qDebug() << nAccumC3*targetReaction.molesChange << " в C3 макс";
        m_guiPresTarget.m_supplyCount = (int)(m_guiPresTarget.m_nAccum/(nAccumB+nAccumC1+nAccumC2+nAccumC3))+1;
        qDebug() << m_guiPresTarget.m_supplyCount << " напусков всего нужно";
    }
    else{
        m_guiPresTarget.m_supplyCount = 1;
    }
    emit guiPresTargetChanged();
    // if(m_guiPresTarget.m_nAccum > 1.1){
    // }
    // else{
    //     if(storagePressureTarget <= supplyMax){
    //         m_guiPresTarget.m_storagePressureTarget = storagePressureTarget;
    //     }
    //     else if(storagePressureTargetWithC1 <= supplyMax){
    //         m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC1;
    //         m_guiPresTarget.m_c1PressureTarget = storagePressureTargetWithC1;
    //     }
    //     else if(storagePressureTargetWithC2 <= supplyMax){
    //         m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC2;
    //         m_guiPresTarget.m_c2PressureTarget = storagePressureTargetWithC2;
    //     }
    //     else if(storagePressureTargetWithC3 <= supplyMax){
    //         m_guiPresTarget.m_storagePressureTarget = storagePressureTargetWithC3;
    //         m_guiPresTarget.m_c3PressureTarget = storagePressureTargetWithC3;
    //     }
    //     else{
    //         m_guiPresTarget.m_storagePressureTarget = supplyMax;
    //         m_guiPresTarget.m_c3PressureTarget = supplyMax;
    //     }
    // }
}

void PlayPressure::setGuiPresTarget(guiPressureTarget guiPresTarget){
    m_guiPresTarget = guiPresTarget;
}

guiPressureTarget PlayPressure::getGuiPresTarget(){
    return m_guiPresTarget;
}