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

    void setValvePointers(const QVector<Valve*>& ptr);
    bool setValveStates();
    void setTimePointer(ControllerData* ptr);
    void setPressurePointers(const QVector<ControllerData*>& ptr);
    void setTempPointers(const QVector<ControllerData*>& ptr);
    void setFiltersDataPointers(const QVector<FilterData*>& ptr);
    
    Q_INVOKABLE void updateFilter(int chartIndex); // move to DataAcquisition
    
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    Q_INVOKABLE bool setSupplyMeasure(bool supplyMeasure);
    
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
    QVector<Valve*> m_valves;
    // Valve* m_valves[16]; // to reqValveDO 
    // int m_valvesCnt;
    
    AdvantechBuff reqSensorAI;
    // AI pointers
        // pres
    QVector<ControllerData*> m_pressureSensors;
    // ControllerData* m_pressureSensors[8];
    // int m_pressureSensorsCnt;
    
    AdvantechAI reqTempAI;
    // AI pointers
        // temp
    QVector<ControllerData*> m_tempSensors;
    // ControllerData* m_tempSensors[8];
    // int m_tempSensorsCnt;
    
    FilterView filterView;
    QVector<FilterData*> m_filtersData;
    // FilterData* m_filtersData[8];
    // int m_filtersDataCnt;

    bool m_supplyMeasure;
    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    
};