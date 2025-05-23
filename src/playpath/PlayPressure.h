#pragma once
#include "../addon/AddRemoveQuartile.h"
#include "../addon/StorageQuartile.h"
#include "../addon/ReactionQuartile.h"
#include <QObject>
#include <QFile>
#include <QElapsedTimer>
// let's make it as experiment object with file saves

struct guiPressureTarget{
    Q_GADGET
    Q_PROPERTY (double chPresTg     MEMBER m_chamberPressureTarget)
    Q_PROPERTY (double chPresInit   MEMBER m_chamberInitialPressure)
    Q_PROPERTY (double stPresTg     MEMBER m_storagePressureTarget)
    Q_PROPERTY (double nAccum       MEMBER m_nAccum)
    Q_PROPERTY (int    supplyCount  MEMBER m_supplyCount)
    Q_PROPERTY (double c1PresTg     MEMBER m_c1PressureTarget)
    Q_PROPERTY (double c2PresTg     MEMBER m_c2PressureTarget)
    Q_PROPERTY (double c3PresTg     MEMBER m_c3PressureTarget)
public:
    double m_chamberPressureTarget; 
    double m_chamberInitialPressure;
    double m_storagePressureTarget;
    double m_nAccum;
    int     m_supplyCount;
    double m_c1PressureTarget;
    double m_c2PressureTarget;
    double m_c3PressureTarget;
};

class PlayPressure : public QObject
{
    Q_OBJECT // ?
    Q_PROPERTY (guiPressureTarget guiPresTarget READ getGuiPresTarget WRITE setGuiPresTarget NOTIFY guiPresTargetChanged)
public:
    PlayPressure(QObject *parent = nullptr);
    ~PlayPressure();
    void setARQ(AddRemoveQuartile* arq);
    void setSQ(StorageQuartile* sq);
    void setRQ(ReactionQuartile* rq);
    void play();
    void playWithAccuum();
    guiPressureTarget getGuiPresTarget();
    void setGuiPresTarget(guiPressureTarget guiPresTarget);
signals:
    void guiPresTargetChanged();
private:
    AddRemoveQuartile* m_arQ;
    StorageQuartile* m_sQ;
    ReactionQuartile* m_rQ;
    guiPressureTarget m_guiPresTarget;
};