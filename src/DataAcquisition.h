#pragma once

#include <QTimer>
#include "controllers/AdvantechCtrl.h"
#include "FilterView.h"
#include "Initialize.h" 

#include "DataCollection.h"
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
    void initDaqDO(const daqParameters &parameterDO); 
    void initDaqAIpres(const daqParameters &parameterAIpres);
    void initDaqAItemp(const daqParameters &parameterAItemp);
    void processEvents(); // weirdly written
    void processEvents(QString purpose); // ?

    // QMap<QString,QVector<double>> getMeasures();
    bool getGRAMsIntegrity();

    void setValvePointers(Valve ptr[], int valvesCnt);
    bool setValveStates();

    void setPressurePointers(ControllerData ptr[], int pressureCnt);
    void setTempPointers(ControllerData ptr[], int tempCnt);
    
    Q_INVOKABLE void turnOnFilterTimer(bool s);
    Q_INVOKABLE void setNewFilter();

private slots:
    void filterEvent();
private:
    // QMap<QString, QSharedPointer<AdvantechCtrl>> m_controllerList; // for read
    QMap<QString, ControllerConnection> GRAMsIntegrity;
    
    ControllerData* time;
    QTimer* fastFilter;
    
    AdvantechDO reqValveDO;
    // valve pointers
    Valve* m_valves[16]; // to reqValveDO 
    int m_valvesCnt;
    
    AdvantechBuff reqSensorAI;
    // AI pointers
        // pres
    ControllerData* m_pressureSensors[8];
    int m_pressureSensorsCnt;
    
    AdvantechAI reqTempAI;
    // AI pointers
        // temp
    ControllerData* m_tempSensors[8];
    int m_tempSensorsCnt;
    

    FilterView filterView;
};