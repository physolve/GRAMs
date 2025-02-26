#pragma once

#include <QTimer>
#include "controllers/AdvantechCtrl.h"
#include "FilterView.h"
#include "Initialize.h" 

#include "ValveModel.h"

enum ControllerConnection{
    Offline,
    Online,
    Pending
};

class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
    void initDaq(const QList<daqParameters> &parameters); 
    void processEvents(); // weirdly written
    void processEvents(QString purpose); // ?

    // QMap<QString,QVector<double>> getMeasures();
    bool getGRAMsIntegrity();

    void setValvePointers(Valve ptr[], int valvesCnt);
    void setValveStates();
    
    Q_INVOKABLE void turnOnFilterTimer(bool s);
    Q_INVOKABLE void setNewFilter();

private slots:
    void filterEvent();
private:
    // QMap<QString, QSharedPointer<AdvantechCtrl>> m_controllerList; // for read
    QMap<QString, ControllerConnection> GRAMsIntegrity;
    QTimer* fastFilter;
    FilterView filterView; // i don't like it pass as pointer from main class

    // SHOULD BE CONTROLLERS
    // Data Acquisition objects

    // let's Valve Daq start
    AdvantechDO reqValveDO;
    // valve pointers
    Valve* m_valves[16];
    int m_valvesCnt;
    void initDO(const daqParameters &parameter);

};