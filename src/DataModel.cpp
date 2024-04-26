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

void MyModel::appendProfileSensors(const QString &controllerPurpose, const QVariantList &sensors){
    // for this moment we only know about profile and not controllers, so we create firstly using names
    // and then in initializeAcquisition we put mapping to link with controller channels

    QStringList sensorNameList;
    //while creation link sensor to appenging data and function
    for(const auto &sensorObj : sensors){
        auto sensorObjMap = sensorObj.toMap();
        auto sensorName = sensorObjMap["name"].toString();
        QVariantMap param = sensorObjMap["parameters"].toMap();
        QMap<QString, double> m_param; // is it nessesary?
        m_param["A"] = param["A"].toDouble();
        m_param["B"] = param["B"].toDouble();
        m_param["R"] = param["R"].toDouble();
        m_sensors.insert(sensorName, Sensor(sensorName, m_param));
        sensorNameList << sensorName;
    }
    m_controllersToSensors.insert(controllerPurpose, sensorNameList);

}

void MyModel::initializeAcquisition(){
    // is there any other initializations?
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
        return sensor.m_name;
    }
    else if ( role == Time )
        return QVariant::fromValue(sensor.getTime());
    else if ( role == Value )
        return QVariant::fromValue(sensor.getValue());
    else if ( role == CurTime)
        return QVariant::fromValue(sensor.getCurTime());
    else if ( role == CurValue)
        return QVariant::fromValue(sensor.getCurValue());
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

Sensor* MyModel::getSensor(const QString &name){
    return &m_sensors[name];
}

// QVariantMap MyModel::getCurPressureValues() const{
//     //QVector<double> curPressureValues;
//     QVariantMap curPressureValues;
//     for(const auto &name : m_controllersToSensors["pressure"]){
//         curPressureValues[name] = m_sensors[name].getCurValue();
//     } // its bad, try lambda 
//     return curPressureValues;//QVariant::fromValue(curPressureValues); 
// }
// QVariantMap MyModel::getCurTempValues() const{
//     //QList<double> curTempValues;
//     QVariantMap curTempValues;
//     for(const auto &name : m_controllersToSensors["temperature"]){
//         curTempValues[name] = m_sensors[name].getCurValue();
//     }
//     // use lambda instead for!!! 
//     return curTempValues;//QVariant::fromValue(curTempValues); 
// }

void MyModel::appendData(const QMap<QString, QVector<double>> & dataMap){ // not tested
    if(m_sensors.isEmpty())
        return;
    qreal time = m_time.elapsed()/1000.0;
    for (auto i = dataMap.begin(), end = dataMap.end(); i != end; ++i){
        auto mappedNames = m_controllersToSensors[i.key()];
        auto values = i.value();
        for(auto j = 0; j<mappedNames.count(); ++j){
            m_sensors[mappedNames.at(j)].appendData(time, values.at(j));
        }
        // use lambda instead of count method!!! 
    }
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_sensors.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value << CurTime << CurValue );
}
