#include "Sensor.h"
#include <QDebug>


Sensor::Sensor(const QString &name, const QMap<QString,double> &parameters)://, QObject *parent) : QObject(parent),
m_name(name), m_A(parameters["A"]), m_B(parameters["B"]), m_R(parameters["R"])
{
    m_x.append(0);
    m_y.append(0);
}

Sensor::~Sensor(){
    qDebug() << m_name << " free";
}

void Sensor::appendData(qreal x, double y){
    auto m_cX = x;
    
    auto m_cY = y;
    filterData(m_cY);

    m_x.append(m_cX);
    m_y.append(m_cY);
}

void Sensor::setData(const QVector<qreal> &x, const QVector<double> &y){
    m_x = x;
    m_y = y;
}

void Sensor::filterData(double &data){
    data = m_A * data / m_R - m_B; // if not mA, then without m_R
}

QVector<qreal> Sensor::getTime() const{
    return m_x.toVector();
}
QVector<double> Sensor::getValue() const{
    return m_y.toVector();
}
qreal Sensor::getCurTime() const{
    return m_x.last();
}
double Sensor::getCurValue() const{
    return m_y.last();
}