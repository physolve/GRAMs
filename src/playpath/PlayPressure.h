#pragma once
#include "../addon/AddRemoveQuartile.h"
#include "../addon/StorageQuartile.h"
#include "../addon/ReactionQuartile.h"
#include <QObject>
#include <QFile>
#include <QElapsedTimer>
// let's make it as experiment object with file saves

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
private:
    AddRemoveQuartile* m_arQ;
    StorageQuartile* m_sQ;
    ReactionQuartile* m_rQ;
};