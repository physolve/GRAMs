#include "SupplyPort.h"

#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";

}

SupplyPort::~SupplyPort()
{
    m_supplyPressure = nullptr;
    pseudo_time = nullptr;
    qDebug() << "SupplyPort class is destroyed";
}

void SupplyPort::saveResultsToFile(){

}

