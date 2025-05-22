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
    Q_PROPERTY (double m_chPresTg     MEMBER m_chamberPressureTarget)
    Q_PROPERTY (double m_chPresInit   MEMBER m_chamberInitialPressure)
public:
    double m_chamberPressureTarget;
    double m_chamberInitialPressure;
};

class PlayPressure : public QObject
{
    Q_OBJECT // ?
public:
    PlayPressure(QObject *parent = nullptr);
    ~PlayPressure();
    void setARQ(AddRemoveQuartile* arq);
    void setSQ(StorageQuartile* sq);
    void setRQ(ReactionQuartile* rq);
    void play();
    void playWithAccuum();
private:
    AddRemoveQuartile* m_arQ;
    StorageQuartile* m_sQ;
    ReactionQuartile* m_rQ;
};