#include "DataAcquisition.h"

DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent)
{
}

void DataAcquisition::processEvents(){
    for(auto controller : controllerList){
        controller->readData();
    }
}

const QList<QVector<double>> DataAcquisition::getDataList(){ // const & >
    QList<QVector<double>> dataList;
    for(auto controller : controllerList){ // data from controllerList in order of purpose
        dataList << controller->getData();
    }
    return dataList;
}
