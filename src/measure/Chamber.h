#pragma once

#include <QObject>
#include "../DataCollection.h"
#include "../addon/NodePressure.h"

struct ChamberParameters{
    Q_GADGET
    Q_PROPERTY (QString chamberName MEMBER m_chamberName)
    Q_PROPERTY (double volume MEMBER m_volume)
    Q_PROPERTY (double craneToChamber MEMBER m_craneToChamber)
    Q_PROPERTY (int chamberMax MEMBER m_chamberMax)
    Q_PROPERTY (bool status MEMBER m_status)
public:
    QString m_chamberName;
    double m_volume;
    double m_craneToChamber;
    int m_chamberMax;
    bool m_status;
};

class Chamber : public QObject
{
    Q_OBJECT // ?
    Q_PROPERTY(ChamberParameters chamberParams READ getChamberParams NOTIFY chamberParamsChanged)

public:
    Chamber(QObject *parent = nullptr);
    ~Chamber();
    void setChamberVolume(const VolumeObject& chamberVolume);
    void setCraneToChamber(double craneToChamber);
    void setChamberMax(int chamberMax);
    void setStatusOpen(bool status);
    bool getStatusOpen() const;
    ChamberParameters getChamberParams() const;
    VolumeObject getVolumeObject() const;
    VolumeObject getCraneObject() const;
signals:
    void chamberParamsChanged();
private:
    VolumeObject m_chamberVolume;
    VolumeObject m_craneVolume;
    ChamberParameters m_chamberParams;
};