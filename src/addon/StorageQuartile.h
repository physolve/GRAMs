#pragma once

#include "Quartile.h"

struct StorageStrategy{  
    const double  m_storagePressureTarget;
    const double  m_c1PressureTarget;
    const double  m_c2PressureTarget;
    const double  m_c3PressureTarget;
    const int     m_c1OpenTime; // ms
    const int     m_c2OpenTime; // ms
    const int     m_c3OpenTime; // ms};
};

enum addCVolume{
    None,
    Small,
    Medium,
    Large
};


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
    
    double getTargetFromMolesChange(const double& molesChange, addCVolume cVolume = addCVolume::None);
    changeToTarget getChangeToIntermediateTarget(const double& targetPressureS, addCVolume cVolume = addCVolume::None);
    changeToTarget getChangeToIntermediateTarget(const double& targetPressureS, const QVector<addCVolume> &cVolumes);
    changeToTarget getChangeToTarget(const double& targetPressureS, const double& fromTargetPressureS, addCVolume cVolume = addCVolume::None);
    double getTargetCVolumeFromMolesChange(const double& molesChange, addCVolume cVolume);
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

    StorageStrategy m_storageStrategy;
};
