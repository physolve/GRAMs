#pragma once

#include <QTimer>
#include "Quartile.h"
//
class StorageQuartile;
class LightPlot;

struct ReactionStrategy{
    Q_GADGET
    Q_PROPERTY (double  reducerLimit    MEMBER m_reducerLimit) // mutable
    Q_PROPERTY (QString usePort         MEMBER m_usePort) // mutable
    Q_PROPERTY (double  pressureLimit   MEMBER m_pressureLimit) // mutable
    Q_PROPERTY (int     openTime        MEMBER m_openTime) // mutable
public:
    double  m_reducerLimit;     // bar
    QString m_usePort;          // [0-2]
    double  m_pressureLimit;    // bar in storage
    int     m_openTime;         // ms
};

class ReactionQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit ReactionQuartile(QObject *parent = nullptr);
    virtual ~ReactionQuartile();
    void calculateTotalVolume();

    void setChamber(const QString& chamber);
    void setChamberPointer(Chamber* chamberPtr);
    void setChamberStatus(bool status);
    void updateChamberToQuartile();
    void removeChamber();

    void setQuartileDataPressure(QuartileData* quartileData);
    void setQuartileDataTemperature(QuartileData* quartileData);
    void setED2VolumePtr(DataCollection* ptrE, DataCollection* ptrD2Atm, DataCollection* ptrD2Low);
    void setFVolumePtr(DataCollection* ptrF);
    void setMolesPtr(const QVector<MolesData*>& ptr);

    // can be filled as profile info
    void setIndexValveRange(int index);
    void setIndexPressureHighLow(int indexHigh, int pressureAtm, int indexLow);
    void setIndexTemperatureMain(int index);
    void setIndexTemperatureChamber(int index);
    
    void updateQuartileData();
    void updateVolumeObjects();
    void updateMoles();
    
    QStringList getUsedVolumes() const;
    double getQuartileModelPressureFromMoles(double model_moles, const QStringList& volumes) const;

    void setReactionPressurePtr(FilterData* high, FilterData* low); // filters
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);

    void updateLeakageState();

    Q_INVOKABLE void setReactionAdjustParameters(QVariantMap parameters);
    Q_INVOKABLE void startLeakageMeasure(bool measure);

    changeToTarget getChangeToIntermediateTarget(const double& targetPressureR, bool addChamber);
    changeToTarget getChangeToTarget(const double& targetPressure, const double& fromPressure);
private slots:
    void expEvent();
private:
    // chamber object
    QString profileChamber;
    // sample object
    // additional volumes not objects
    void fillGasLeakageData();
    void preCalculateLeakageTime(int portId, double turn);
    DataCollection* eVolumePressure;
    DataCollection* d2VolumePressureAtm;
    DataCollection* d2VolumePressureLow;
    // additional volumes not objects
    DataCollection* fVolumePressure;
    // active volume
    QuartileData* pressureReactionQuartile;
    QuartileData* temperatureReactionQuartile;
    // moles info
    QVector<MolesData*> m_molesDataList;
    // valve flow object
    int m_currentGasLeakage;
    GasLeakage m_gasLeakage[3];
    
    QList<LightPlot*> m_reactionGraphs;

    FilterData* m_reactionPressureHigh;
    FilterData* m_reactionPressureLow;
    QuartileData* m_storageQuartilePressure;
    StorageQuartile* m_storageQuartile;
    
    // index of pressure range valve
    int v_pressure_range;
    int s_pressure_high;
    int s_pressure_atm;
    int s_pressure_low;
    int s_temperature_main;
    int s_temperature_chamber;
    
    // suppose that chamber not apply to Volume List
    // but has it's own Volume object to update
    Chamber* m_chamber;
    bool m_chamber_connected;

    QTimer* m_expUpdate;
};
