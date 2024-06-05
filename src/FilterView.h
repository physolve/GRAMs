#pragma once
#include <QDebug>
#include "Sensor.h"
#include "VoltageFilter.h"

class FilterView : public QObject
{
    Q_OBJECT 
public:
    explicit FilterView(QObject *parent  = nullptr);
    ~FilterView();
    void setFilterSize(int channelCount);
    Q_INVOKABLE QSharedPointer<Sensor> getChannelSensor(int channel);
    
    // function to update values somewhere
    // function to link Kalman parameters with View
    void appendDataToView(int viewN, const QVector<qreal> &time, const QVector<double> &data);

signals:
    void changeFilterMatrix();

    void updateView();

private:
    // should be separate window with Custom plot and Kalman Filter's parameters
    QList<QSharedPointer<Sensor>> m_channelsData;
    
};