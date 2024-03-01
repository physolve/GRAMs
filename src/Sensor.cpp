#include "Sensor.h"

Sensor::Sensor(const QString &name, const QMap<QString,double> &parameters, QObject *parent) : QObject(parent),
m_name(name), m_A(parameters["A"]), m_B(parameters["B"]), m_R(parameters["R"]), m_cX(0), m_cY(0)
{
}
void Sensor::appendData(quint64 x, double y){
    m_cX = x;
    
    m_cY = y;
    filterData(m_cY);

    m_x.append(m_cX);
    m_y.append(m_cY);
}

void Sensor::filterData(double &data){
    data = m_A * data / m_R - m_B;
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