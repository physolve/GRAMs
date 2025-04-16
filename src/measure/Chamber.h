#pragma once

#include <QObject>
#include "../DataCollection.h"
#include "../addon/NodePressure.h"

struct ChamberParameters{
    Q_GADGET
    Q_PROPERTY (QString chamberName MEMBER m_chamberName)
    Q_PROPERTY (double volume MEMBER m_volume)
public:
    QString m_sensorName;
    double m_volume;
};

class Chamber : public QObject
{
    Q_OBJECT // ?
    Q_PROPERTY(ChamberParameters chamberParams READ getChamberParams NOTIFY chamberParamsChanged)

public:
    Chamber(QObject *parent = nullptr);
    ~Chamber();
    void setChamberVolume(const VolumeObject& chamberVolume);
    void setStatusOpen(bool status);
    bool getStatusOpen() const;
    ChamberParameters getChamberParams() const;
signals:
    void chamberParamsChanged();
private:
    VolumeObject m_chamberVolume;
    ChamberParameters m_chamberParams;
    bool statusOpen;
};