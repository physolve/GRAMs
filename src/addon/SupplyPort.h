#pragma once

#include <QObject>

#include "../DataCollection.h"
// let's make it as experiment object with file saves

class SupplyPort : public QObject
{
    Q_OBJECT // ?
public:
    SupplyPort(QObject *parent = nullptr);
    ~SupplyPort();
    // info about port
    double m_turn;
    // data to file
    void saveResultsToFile();
private:

    // to save
    FilterData* m_supplyPressure;
    FilterData* pseudo_time;

};