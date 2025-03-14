#pragma once

#include <QTimer>
#include "controllers/AdvantechCtrl.h"
#include "FilterView.h"
#include "Initialize.h" 

#include "DataCollection.h"
#include "ValveModel.h"
#include "addon/Quartile.h"

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

    void startAcquisition();
    void stopAcquisition();

    bool getGRAMsIntegrity();

    void setValvePointers(Valve ptr[], int valvesCnt);
    bool setValveStates();
    void setTimePointer(ControllerData* ptr);
    void setPressurePointers(ControllerData ptr[], int pressureCnt);
    void setTempPointers(ControllerData ptr[], int tempCnt);
    void setFiltersDataPointers(FilterData ptr[], int filtersCnt);
    
    Q_INVOKABLE void updateFilter(int chartIndex); // move to DataAcquisition
    
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    Q_INVOKABLE void setSupplyMeasure(bool supplyMeasure);
    
private slots:
    void processEvents();
    
private:
    void fillSupplyARQ();
    // QMap<QString, QSharedPointer<AdvantechCtrl>> m_controllerList; // for read
    QMap<QString, ControllerConnection> GRAMsIntegrity;
    QElapsedTimer m_elapsedTimer;
    
    ControllerData* m_time;
    QTimer* m_acquisitionTimer;
    
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
    FilterData* m_filtersData[8];
    int m_filtersDataCnt;

    bool m_supplyMeasure;
    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    
};