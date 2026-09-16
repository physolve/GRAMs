#pragma once

#include <QTimer>
#include <memory>
#include "controllers/AdvantechCtrl.h"
#include "controllers/SerialCtrl.h"
#include "controllers/ISensorSource.h"
#include "controllers/RealSensorSource.h"
#include "FilterView.h"
#include "Initialize.h" 

#include "DataCollection.h"
#include "ValveModel.h"
#include "addon/Quartile.h"

// enum ControllerConnection{
//     Offline,
//     Online,
//     Pending
// };

class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
    virtual ~DataAcquisition();
    // void initDaqDO(const daqParameters &parameterDO); 
    void initDaqAIpres(const daqParameters &parameterAIpres);
    void initDaqAItemp(const daqParameters &parameterAItemp);
    void initSerialVacuum(const vacuumParameters &parameterVacuum);
    // ДВ302 (второй тракт, турбо). Опционален: без него всё работает как раньше.
    void initSerialTurboVacuum(const vacuumParameters &parameterVacuum);

    void startAcquisition();
    void stopAcquisition();

    bool getGRAMsIntegrity();

    // Источник показаний. По умолчанию — железо (RealSensorSource создаётся при
    // первом init*). Демо-режим ставит свой до инициализации и помечает
    // карты подключёнными: путь данных выше источника не меняется.
    void setSensorSource(std::unique_ptr<ISensorSource> source);
    void markControllersConnected();
    ISensorSource *sensorSource() const { return m_source.get(); }

    // void setValvePointers(const QVector<Valve*>& ptr);
    // bool setValveStates();
    void setTimePointer(ControllerData* ptr);
    void setPressurePointers(const QVector<ControllerData*>& ptr);
    void setTempPointers(const QVector<ControllerData*>& ptr);
    void setFiltersDataPointers(const QVector<FilterData*>& ptr);
    void setVacuumPointer(DataCollection* ptr);
    void setTurboVacuumPointer(DataCollection* ptr);
    Q_INVOKABLE void updateFilter(int chartIndex); // move to DataAcquisition
    
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    void setLeakagePressurePtr(FilterData* high, FilterData* low);

    Q_INVOKABLE bool setLeakageMeasure(bool leakageMeasure);
    Q_INVOKABLE void testVacuumQuery();
    Q_INVOKABLE void testTurboVacuumQuery();

    void beginAction();
    void endAction();
    void fastBufferRead();
    void runSupplyAction();
    
private slots:
    void processEvents();
private:
    void fillSupplyARQ();
    void fillLeakageRQ();

    // QMap<QString, ControllerConnection> GRAMsIntegrity;

    bool pressureController;
    bool temperatureController;

    QElapsedTimer m_elapsedTimer;
    
    ControllerData* m_time;
    QTimer* m_acquisitionTimer;
    // QTimer* m_fastBufferAcquisition;
    bool canReadSlow;
    bool canReadFast;
    // AdvantechDO reqValveDO;
    // valve pointers
    // QVector<Valve*> m_valves;
    
    RealSensorSource &real();
    std::unique_ptr<ISensorSource> m_source;
    RealSensorSource *m_real = nullptr;   // не владеет; nullptr в демо-режиме

    // AI pointers
        // pres
    QVector<ControllerData*> m_pressureSensors;
    // AI pointers
        // temp
    QVector<ControllerData*> m_tempSensors;

    DataCollection* m_vacuumSensor = nullptr;
    DataCollection* m_vacuumSensorTurbo = nullptr;
    
    FilterView filterView;
    QVector<FilterData*> m_filtersData;

    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    bool m_leakageMeasure;
    FilterData* m_leakagePressureHigh;
    FilterData* m_leakagePressureLow;
    
};