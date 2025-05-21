#pragma once

#include "Quartile.h"

class StorageQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit StorageQuartile(QObject *parent = nullptr);
    virtual ~StorageQuartile();
    void calculateTotalVolume();
    void setCVolumePtr(const QVector<DataCollection*>& ptr);
    void setBD1VolumePtr(DataCollection* ptrB, DataCollection* ptrD1);
    void setMolesPtr(const QVector<MolesData*>& ptr);
    void setQuartileDataPressure(QuartileData* quartileData);
    void setQuartileDataTemperature(QuartileData* quartileData);
    void setIndexValveRange(int index);
    void setIndexPressureHighLow(int indexHigh, int indexLow);
    void setIndexTemperatureMain(int index);
    void updateQuartileData();
    void updateVolumeObjects();
    void updateMoles();
    QStringList getUsedVolumes() const;
    double getTargetFromMolesChange(const double& molesChange);
    changeToTarget getChangeToTarget(const double& targetPressure);
private:
    // additional volumes not objects
    QVector<DataCollection*> cVolumePressure;
    DataCollection* bVolumePressure;
    DataCollection* d1VolumePressure;
    // moles info
    QVector<MolesData*> m_molesDataList;
    // active volume
    QuartileData* pressureStorageQuartile;
    QuartileData* temperatureStorageQuartile;
    // index of pressure range valve
    int v_pressure_range;
    int s_pressure_high;
    int s_pressure_low;
    int s_temperature_main;
};
