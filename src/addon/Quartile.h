#pragma once

#include <QObject>
#include <QMap>
#include "../DataCollection.h"
#include "NodePressure.h"
#include "SupplyPort.h"
#include "GasLeakage.h"
#include "LightPlotItem.h"
// Quartile object is used to store parameters from one of four volumes
class Quartile : public QObject
{
    Q_OBJECT // ?
public:
    explicit Quartile(QObject *parent = nullptr);
    virtual ~Quartile();
    void setVolume(double volume);
    void setMainVolume(const QString& name);
    void addVolume(const QString& name, double volume);
    void addValvePtrs(const QVector<Valve*>& ptr);
    void addPressurePtrs(const QVector<ControllerData*>& ptr);
    void addTemperaturePtrs(const QVector<ControllerData*>& ptr);
    virtual void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB);
    // pressure sensor pointers
protected:
    double m_volume;
    QString m_mainVolume;
    QMap<QString, VolumeObject> m_volumeObjects;
    // pressure sensor pointers
    QList<ControllerData*> m_pressureList;
    // temperature sensor pointers
    QList<ControllerData*> m_temperatureList;
    // valve states
    QList<Valve*> m_valves;
    // pressure nodes
    QList<NodeData*> m_nodeDataList;
    QMap<QString, NodePressure> m_pressureNodes;
};

class AddRemoveQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit AddRemoveQuartile(QObject *parent = nullptr);
    virtual ~AddRemoveQuartile();
    void setSupplyPressurePtr(FilterData* high, FilterData* low);
    
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartilePtr(Quartile* storageQuartile);
    void updatePortState();

    Q_INVOKABLE int getLightPlotPtr(LightPlotItem* customPlotPointer);
    Q_INVOKABLE void setSupplyAdjustParameters(QVariantMap parameters);
    Q_INVOKABLE void startSupplyMeasure(bool measure);
private slots:
    void expEvent();

private:
    void fillSupplyPortData();
    void preCalculateSupplyTime(int portId, double turn, double portPressure);
    double m_supplySpeed; // current
    double m_drainSpeed;
    int m_currentSupplyPort;
    SupplyPort m_supplyPort[3]; // settings different but process only one!
    // custom plot graph local
    FilterData* m_supplyPressureHigh;
    FilterData* m_supplyPressureLow;
    QList<LightPlotItem*> m_supplyPressurePlots;
    // quartile pressure from StorageQuartile
    QTimer* m_expUpdate;
    QuartileData* m_storageQuartilePressure;
    Quartile* m_storageQuartile; // try more header files and just ask another
};

class StorageQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit StorageQuartile(QObject *parent = nullptr);
    virtual ~StorageQuartile();
    void calculateTotalVolume();
    void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB) override;
    void setCVolumePtr(const QVector<DataCollection*>& ptr);
    void setBD1VolumePtr(DataCollection* ptrB, DataCollection* ptrD1);
    void setMolesPtr(const QVector<MolesData*>& ptr);
    void setQuartileDataPressure(QuartileData* quartileData);
    void setQuartileDataTemperature(QuartileData* quartileData);
    void setIndexValveRange(int index);
    void setIndexPressureHighLow(int indexHigh, int indexLow);
    void setIndexTemperatureMain(int index);
    void fillVolumePairs(const QMap<QString,QString>& volumeToValve);
    void updateQuartileData();
    void updateVolumeObjects();
    void updateMoles();
    double getQuartileMoleVolume() const;
    double getQuartileMole() const; // move to Quartile base class
    double getQuartileModelPressure(double model_flow);
    double getQuartileModelPressureFromMoles(double model_moles) const;
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
    QList<QPair<int,QString>> m_valveToVolumeList;
};

class ReactionQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit ReactionQuartile(QObject *parent = nullptr);
    virtual ~ReactionQuartile();
    void calculateTotalVolume();
    void setChamber(const QString& chamber);
    void setQuartileDataPressure(QuartileData* quartileData);
    void setQuartileDataTemperature(QuartileData* quartileData);
    void setED2VolumePtr(DataCollection* ptrE, DataCollection* ptrD2Atm, DataCollection* ptrD2Low);
    void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB) override;
    void setMolesPtr(const QVector<MolesData*>& ptr);
    void setIndexValveRange(int index);
    void setIndexPressureHighLow(int indexHigh, int pressureAtm, int indexLow);
    void setIndexTemperatureMain(int index);
    void fillVolumePairs(const QMap<QString,QString>& volumeToValve);
    void updateQuartileData();
    void updateVolumeObjects();
    void updateMoles();
    double getQuartileMoleVolume() const;
    double getQuartileModelPressure(double model_flow);
    double getQuartileModelPressureFromMoles(double model_moles) const;

    
    void setReactionPressurePtr(FilterData* high, FilterData* low); // filters
    void setStorageQuartilePressure(QuartileData* storageQuartilePressure);
    void setStorageQuartilePtr(StorageQuartile* storageQuartile);

    void updateLeakageState();

    Q_INVOKABLE int getLightPlotPtr(LightPlotItem* customPlotPointer);
    Q_INVOKABLE void setReactionAdjustParameters(QVariantMap parameters);
    Q_INVOKABLE void startLeakageMeasure(bool measure);
private slots:
    void expEvent();
private:
    // chamber object
    QString profileChamber;
    // sample object
    // additional volumes not objects
    double getQuartileMole() const;
    void fillGasLeakageData();
    void preCalculateLeakageTime(int portId, double turn);
    DataCollection* eVolumePressure;
    DataCollection* d2VolumePressureAtm;
    DataCollection* d2VolumePressureLow;
    // active volume
    QuartileData* pressureReactionQuartile;
    QuartileData* temperatureReactionQuartile;
    // moles info
    QVector<MolesData*> m_molesDataList;
    // valve flow object
    int m_currentGasLeakage;
    GasLeakage m_gasLeakage[3];
    //
    FilterData* m_reactionPressureHigh;
    FilterData* m_reactionPressureLow;
    QuartileData* m_storageQuartilePressure;
    StorageQuartile* m_storageQuartile;
    QList<LightPlotItem*> m_reactionPressurePlots;
    // index of pressure range valve
    int v_pressure_range;
    int s_pressure_high;
    int s_pressure_atm;
    int s_pressure_low;
    int s_temperature_main;
    QList<QPair<int,QString>> m_valveToVolumeList;

    QTimer* m_expUpdate;
};

class SecondLineQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit SecondLineQuartile(QObject *parent = nullptr);
    virtual ~SecondLineQuartile();

private:
    QList<VolumeObject> m_volumeObjects;
    // pressure sensor pointers
};