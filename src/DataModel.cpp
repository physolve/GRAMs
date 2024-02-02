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
    int idx_shift = 0;
    for (auto i = sensors.begin(), end = sensors.end(); i != end; ++i){
        QString controllerPurpose = i.key();
        //differentiate Sensor type using controllerName
        QStringList sensorNameList = i.value().toStringList();

        int idx = idx_shift;
        for(auto sensorName : sensorNameList){
            // might be Sensor type, but whatever
            auto sensor = new Sensor(sensorName, idx);
            this->m_sensors.append(sensor);
            ++idx;
        }
        idx_shift += sensorNameList.count();
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

    auto sensor = this->m_sensors.at(index.row());
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

QVariant MyModel::getCurPressureValues() const{
    QList<double> curPressureValues;
    auto idStart = 0;
    auto idEnd = 5; 
    for(auto i = idStart; i<=idEnd; i++){
        curPressureValues << m_sensors[i]->getCurValue();
    }
    return QVariant::fromValue(curPressureValues); 
}
QVariant MyModel::getCurTempValues() const{
    QList<double> curTempValues;
    auto idStart = 6;
    auto idEnd = 10; 
    for(auto i = idStart; i<=idEnd; i++){
        curTempValues << m_sensors[i]->getCurValue();
    }
    return QVariant::fromValue(curTempValues); 
}

void MyModel::appendData(const QList<QVector<double>> & dataList){ //dataMap to Sensor ???
    // order of controller names
    // for(auto &controllerData : dataList){
        
    // }
    //auto controllerData = dataList.at(0);
    int idx_shift = 0;
    for(auto& controllerData : dataList){
        
        const int count = controllerData.count();//m_sensors.count(); //  m_sensors of THE controller
        //qDebug() << "controller Data count = " << controllerData.count();
        // what count to use to identify data?
        auto time = m_time.elapsed()/1000;
        auto value = 0.0;
        for (int i = 0; i < count; ++i) {
            value = controllerData.at(i);
            m_sensors[i+idx_shift]->appendData(time,value);
        }
        idx_shift += count;
    }
    
    //const auto data = dataList.at(0);
    //qDebug() << QString("shift = %1, sensors count = %2").arg(idx_shift).arg(m_sensors.count());
    // we've just updated all rows...
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_sensors.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value << CurTime << CurValue );
}