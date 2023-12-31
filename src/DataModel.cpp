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

void MyModel::appendProfileSensors(QVariantMap sensors){
    // for this moment we only know about profile and not controllers, so we create firstly using names
    // and then in initializeAcquisition we put mapping to link with controller channels
    int idx_shift = 0;
    for (auto i = sensors.begin(), end = sensors.end(); i != end; ++i){
        QString controllerName = i.key();
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

void MyModel::initializeAcquisition(){ //const QList<ControllerInfo>& controllersInfo
    
    m_time.start(); // Elapsed timer
    //clear weird data
    //m_controllersInfo = controllersInfo;

    //for(auto info : m_controllersInfo){
        // get Names from what?
        // if we have name in already existing sensors we just rearrange them
        //auto sensorNames = info.getNames();
        
        // if (sensorNames.count() >= info.m_channelCountCh-info.m_channelStartCh)
        //     qDebug() << "All channels are mapped";
        //else continue; // it means that we append m_sensors before!!!
        // those append for unexpected sensors not appeared in profile

        // channelMap is VariantMap with structure {name:channel}
        // BUT we should get this map for EVERY controller -> must Use channelMapList[controllerName] -> channelMap[array?]
        // CHECK COUNT and CHANNELS
        
        //for(int i = info.m_channelStartCh; i<info.m_channelCountCh; i++){ // default : m_channelStartCh = 0, m_channelCountCh = 8
            // check type of Sensor
            // check info providing ControllerInfo with sensors matching channel
        //}
    //}

    //testDataTimer = new QTimer(this);
    //connect(testDataTimer , &QTimer::timeout, this, &MyModel::testDataFoo);
    //testDataTimer->start(1000);
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