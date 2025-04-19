#pragma once

#include <QObject>
#include "../DataCollection.h"
#include "../addon/NodePressure.h"

struct ChamberParameters{
    Q_GADGET
    Q_PROPERTY (QString chamberName MEMBER m_chamberName)
    Q_PROPERTY (double volume MEMBER m_volume)
    Q_PROPERTY (double craneToChamber MEMBER m_craneToChamber)
    Q_PROPERTY (bool status MEMBER m_status)
public:
    QString m_chamberName;
    double m_volume;
    double m_craneToChamber;
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
    void setStatusOpen(bool status);
    bool getStatusOpen() const;
    double getVolume() const;
    ChamberParameters getChamberParams() const;
signals:
    void chamberParamsChanged();
private:
    VolumeObject m_chamberVolume;
    ChamberParameters m_chamberParams;
};