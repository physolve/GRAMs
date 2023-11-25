#include "Sensor.h"

Sensor::Sensor(QString name, int idx, QObject *parent) : QObject(parent),m_name(name),m_idx(idx),m_cX(0),m_cY(0)
{
}
void Sensor::appendData(quint64 x, double y){
    m_cX = x;
    m_cY = y;
    //filters
    m_x.append(m_cX);
    m_y.append(m_cY);
}
QList<quint64>& Sensor::getTime(){
    return m_x;
}
QList<double>& Sensor::getValue(){
    return m_y;
}
quint64 Sensor::getCurTime(){
    return m_cX;
}
double Sensor::getCurValue(){
    return m_cY;
}