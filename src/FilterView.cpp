#include "FilterView.h"

FilterView::FilterView(QObject *parent) : QObject(parent)
{
}

FilterView::~FilterView(){
}

void FilterView::setFilterSize(int channelCount){
    // argument put count to sensor container
    for(int i = 0; i < channelCount; ++i){
        m_channelsData << QSharedPointer<Sensor>::create(QString("voltage_ch%1").arg(i));
    }
}

void FilterView::appendDataToView(int viewN, const QVector<qreal> &time, const QVector<double> &data){
    m_channelsData[viewN]->setData(time, data);
    emit updateView();
}

QSharedPointer<Sensor> FilterView::getChannelSensor(int channel){
    return m_channelsData[channel];
}