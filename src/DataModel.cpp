#include "DataModel.h"

#include <QByteArray>
#include <QTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QtMath>
#include <cstdlib>

MyModel::MyModel(QObject *parent) :
    QAbstractListModel(parent)
{
    // initialize using Sensors count from json mapping
    // for(int i=0;i<5;i++){
    //     QString str  = QString("Sensor-%1").arg(i);
    //     auto sensor = new Sensor(str, i);
    //     this->m_sensors.append(sensor);
    //     //connect(sensor,&Sensor::valueChanged,this,&MyModel::updateDataChanged);
    // }
}

void MyModel::initializeAcquisition(const QList<ControllerInfo>& controllersInfo){
    //clear weird data
    m_controllersInfo = controllersInfo;
    for(auto info : m_controllersInfo){
        auto sensorNames = info.getNames();
        // CHECK COUNT and CHANNELS
        if (sensorNames.count() >= info.m_channelCountCh-info.m_channelStartCh)
            qDebug() << "All channels are mapped";
        else continue;
        for(int i = info.m_channelStartCh; i<info.m_channelCountCh; i++){ // default : m_channelStartCh = 0, m_channelCountCh = 8
            // check type of Sensor
            // use SensorMap
            auto sensor = new Sensor(sensorNames.at(i), i);
            this->m_sensors.append(sensor);   
        }
    }

    //testDataTimer = new QTimer(this);
    //connect(testDataTimer , &QTimer::timeout, this, &MyModel::testDataFoo);
    //testDataTimer->start(1000);

    m_time.start();
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
    return QVariant::fromValue(curValues); 
}
//<-- slide
// void MyModel::updateDataChanged(int idx)
// {
//      //qDebug() << Q_FUNC_INFO << " Update the UI" <<endl;
//     //  QModelIndex start = this->createIndex(0,0);
//     //  QModelIndex end = this->createIndex(9,0);
//     //const int count = m_sensors.count();
//     const QModelIndex startIndex = index(idx, 0);
//     const QModelIndex endIndex   = index(idx, 0);
//     emit dataChanged(startIndex,startIndex);
// }


// void MyModel::duplicateData(int row)
// {
//     if (row < 0 || row >= m_data.count())
//         return;

//     const Data data = m_data[row];
//     const int rowOfInsert = row + 1;

//     beginInsertRows(QModelIndex(), rowOfInsert, rowOfInsert);
//     m_data.insert(rowOfInsert, data);
//     endInsertRows();
// }

// void MyModel::removeData(int row)
// {
//     if (row < 0 || row >= m_data.count())
//         return;

//     beginRemoveRows(QModelIndex(), row, row);
//     m_data.removeAt(row);
//     endRemoveRows();
// }

void MyModel::appendData(const QList<QVector<double>> & dataList){ //dataMap to Sensor ???
    // order of controller names
    const auto data = dataList.at(0);
    const int count = m_sensors.count();
    auto time = m_time.elapsed()/1000;
    auto value = 0.0;
    for (int i = 0; i < count; ++i) {
        value = data.at(i);
        m_sensors[i]->appendData(time,value);
    }
}

// void MyModel::testDataFoo(){
//     static int t;
//     double U, V;
//     const int count = m_sensors.count();
//     for (int i = 0; i < count; ++i) {
//         //U = qCos(M_PI / 50 * t) + 0.5 + QRandomGenerator::global()->generateDouble();
//         V = (qSin(M_PI / 50 * t) + 0.5 + QRandomGenerator::global()->generateDouble())*10;
//         m_sensors[i]->appendData(m_time.elapsed()/1000,V);
//         // m_sensors[i]->x.append(m_time.elapsed()/1000);
//         // m_sensors[i]->y.append(V);
//     }
//     t++;

//     // READ FROM SENSORs

//     // we've just updated all rows...
//     const QModelIndex startIndex = index(0, 0);
//     const QModelIndex endIndex   = index(count - 1, 0);

//     // ...but only the population field
//     emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value << CurTime << CurValue );
// }

// void MyModel::appendPoints(QList<quint64> pointX, QList<double> pointY)
// {
//     const int count = m_data.count();
//     for (int i = 0; i < count; ++i) {
//         m_data[i].x = (pointX);
//         m_data[i].y = (pointY);
//     }
//     // we've just updated all rows...
//     const QModelIndex startIndex = index(0, 0);
//     const QModelIndex endIndex   = index(count - 1, 0);

//     // ...but only the population field
//     emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value);
// }