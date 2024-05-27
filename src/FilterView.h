#pragma once
#include <QDebug>
#include "Sensor.h"

class FilterView : public QObject
{
    Q_OBJECT 
public:
    explicit FilterView(QObject *parent  = nullptr);
    ~FilterView();
    void setFilterSize(int channelCount);
    void setFilteredValue();
    Q_INVOKABLE QSharedPointer<Sensor> getChannelData(int channel);
    // function to update values somewhere
    // function to link Kalman parameters with View
private:
    // should be separate window with Custom plot and Kalman Filter's parameters
    QList<QSharedPointer<Sensor>> m_channelsData;
    
};