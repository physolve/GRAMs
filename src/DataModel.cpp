#include "DataModel.h"

#include <QByteArray>
#include <QTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QtMath>
#include <cstdlib>

MyModel::MyModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

void MyModel::appendProfileSensors(QVariantMap sensors){
    // for this moment we only know about profile and not controllers, so we create firstly using names
    // and then in initializeAcquisition we put mapping to link with controller channels
    
    //while creation link sensor to appenging data and function
    m_controllersToSensors.clear();
    for (auto i = sensors.begin(), end = sensors.end(); i != end; ++i){
        QStringList sensorNameList = i.value().toStringList();
        m_controllersToSensors.insert(i.key(),sensorNameList);
        for(auto sensorName : sensorNameList){
            m_sensors.insert(sensorName, QSharedPointer<Sensor>(new Sensor(sensorName)));
        }
    }
    // in initializeAcquisition we use mapping to converge profile names and controller
}

void MyModel::initializeAcquisition(){
    
    m_time.start(); // Elapsed timer
}

int MyModel::rowCount( const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_sensors.count();
}

QVariant MyModel::data(const QModelIndex &index, int role) const
{
    if ( !index.isValid() )
        return QVariant();
    
    QStringList allNames = m_controllersToSensors["pressure"] + m_controllersToSensors["temperature"];

    auto sensor = this->m_sensors[allNames.at(index.row())];

    if ( role == NameRole ){
        return sensor->m_name;
    }
    else if ( role == Time )
        return QVariant::fromValue(sensor->getTime());
    else if ( role == Value )
        return QVariant::fromValue(sensor->getValue());
    else if ( role == CurTime)
        return QVariant::fromValue(sensor->getCurTime());
    else if ( role == CurValue)
        return QVariant::fromValue(sensor->getCurValue());
    else
        return QVariant();
}

//--> slide
QHash<int, QByteArray> MyModel::roleNames() const
{
    static QHash<int, QByteArray> mapping {
        {NameRole, "name"},
        {Time, "x"},
        {Value, "y"},
        {CurTime, "ct"},
        {CurValue, "cv"}
    };
    return mapping;
}

QVariant  MyModel::getCurValues() const{
    QList<double> curValues;
    for(auto sensor : m_sensors)
        curValues << sensor->getCurValue();
    //qDebug() << curValues;
    return QVariant::fromValue(curValues); 
}

QVariantMap MyModel::getCurPressureValues() const{ // lets try to change from QVariant to QVariantMap
    //QVector<double> curPressureValues;
    QVariantMap curPressureValues;
    QStringList mappedNames = m_controllersToSensors["pressure"];
    for(auto name : mappedNames){
        curPressureValues[name] = m_sensors[name]->getCurValue();
    } // its bad, try lambda 
    return curPressureValues;//QVariant::fromValue(curPressureValues); 
}
QVariantMap MyModel::getCurTempValues() const{
    //QList<double> curTempValues;
    QVariantMap curTempValues;
    QStringList mappedNames = m_controllersToSensors["temperature"];
    for(auto name : mappedNames){
        curTempValues[name] = m_sensors[name]->getCurValue();
    }
    return curTempValues;//QVariant::fromValue(curTempValues); 
}

void MyModel::appendData(const QMap<QString, QVector<double>> & dataMap){ // not tested
    auto time = m_time.elapsed()/1000;
    for (auto i = dataMap.begin(), end = dataMap.end(); i != end; ++i){
        auto mappedNames = m_controllersToSensors[i.key()];
        auto values = i.value();
        for(auto j = 0; j<mappedNames.count(); ++j){
            m_sensors[mappedNames.at(j)]->appendData(time,values.at(j));
        }
    }
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_sensors.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value << CurTime << CurValue );
}