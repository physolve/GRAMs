#include "DataCollection.h"
#include <QDebug>

DataCollection::DataCollection(const QString &name) : m_name(name), m_curValue(0)
{
    m_y.append(0);
}

DataCollection::~DataCollection(){
    qDebug() << "DataCollection destructor + " << m_name;
}
void DataCollection::clearPoints(){
    m_y.clear();
    m_y << 0;
}
void DataCollection::addPoint(const double &val_y){
    m_y.append(m_curValue = val_y);        
}

QVector<double> DataCollection::getValue() const {
    return m_y;
}

QVector<double> DataCollection::getLastToChart() const {
    if(m_y.count() > 120) // last 2 minutes only but stores all // changeble // but i want to use add to CustomPlot
        return m_y.last(120);
    return m_y;
}

double DataCollection::getCurValue() const{
    return m_curValue;
}

ControllerData::ControllerData(const QString &name, const double& A, const double& B) : DataCollection(name),
lin_A(A), lin_B(B)
{

}

ControllerData::~ControllerData(){
    qDebug() << "ControllerData destructor";
}

void ControllerData::setCoeffs(const double& A, const double& B){
    lin_A = A;
    lin_B = B;
}

double ControllerData::getLin_A() const{
    return lin_A;
}

double ControllerData::getLin_B() const{
    return lin_B;
}

void ControllerData::addValue(const double &val_y){
    const auto& val = lin_A*val_y + lin_B;
    addPoint(val);
}

void ControllerData::addValue(const double &val_y, const double &minimalValue){
    auto val = lin_A*val_y + lin_B;
    if(val < minimalValue)
        val = 0.0;
    addPoint(val);
}

FilterData::FilterData(const QString &name) : DataCollection(name)
{

}

FilterData::~FilterData(){
    qDebug() << "FilterData destructor";
}

void FilterData::setData(const QVector<double> &y){
    m_y = y;
    m_curValue = m_y.last();
}

void FilterData::addData(const QVector<double> &y){
    setData(y);
    cumulativeData << m_y;
    cumulativeCount++;
}

QVector<double> FilterData::getCumulativeData() const{
    return cumulativeData;
}

int FilterData::getCumulativeCount(){
    return cumulativeCount;
}

void FilterData::clearCumulative(){
    cumulativeData.clear();
    cumulativeCount = 0;
}

QuartileData::QuartileData(const QString &name) : DataCollection(name)
{

}

QuartileData::~QuartileData(){
    qDebug() << "QuartileData destructor";
}

// Sensor::Sensor(const QString &name, const QMap<QString,double> &parameters)://, QObject *parent) : QObject(parent),
// m_name(name), m_A(parameters["A"]), m_B(parameters["B"]), m_R(parameters["R"])
// {
//     m_x.append(0);
//     m_y.append(0);
// }

// Sensor::~Sensor(){
//     qDebug() << m_name << " free";
// }

// void Sensor::appendData(qreal x, double y){
//     auto m_cX = x;
    
//     auto m_cY = y;
//     filterData(m_cY);

//     m_x.append(m_cX);
//     m_y.append(m_cY);
// }

// void Sensor::setData(const QVector<qreal> &x, const QVector<double> &y){
//     m_x = x;
//     m_y = y;
// }

// void Sensor::filterData(double &data){
//     data = m_A * data / m_R - m_B; // if not mA, then without m_R
// }

// QVector<qreal> Sensor::getTime() const{
//     return m_x.toVector();
// }
// QVector<double> Sensor::getValue() const{
//     return m_y.toVector();
// }
// qreal Sensor::getCurTime() const{
//     return m_x.last();
// }
// double Sensor::getCurValue() const{
//     return m_y.last();
// }