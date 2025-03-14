#pragma once

#include <QObject>
#include <QMap>
#include "../DataCollection.h"
#include "NodePressure.h"
#include "SupplyPort.h"
#include "LightPlotItem.h"
// Quartile object is used to store parameters from one of four volumes
class Quartile : public QObject
{
    Q_OBJECT // ?
public:
    explicit Quartile(QObject *parent = nullptr);
    virtual ~Quartile();
    void setVolume(double volume);
    void addVolume(const QString& name, double volume);
    void addValvePtrs(Valve ptr[], int valvesCnt);
    void addPressurePtrs(ControllerData ptr[], int pressureCnt);
    void addTemperaturePtrs(ControllerData ptr[], int temperatureCnt);
    virtual void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB);
    void updatePressureNodes();
    // pressure sensor pointers
protected:
    double m_volume;
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
    
    void fillSupplyPortData();
    void stopSupplyMeasure();
    Q_INVOKABLE int getLightPlotPtr(LightPlotItem* customPlotPointer);
    Q_INVOKABLE void setSupplyAdjustParameters(QVariantMap parameters);
    Q_INVOKABLE void startSupplyMeasure(bool measure);
private slots:
    void expEvent();

private:
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
};

class StorageQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit StorageQuartile(QObject *parent = nullptr);
    virtual ~StorageQuartile();
    void calculateTotalVolume();
    void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB) override;
private:
    // additional volumes not objects
    // active volume
    QuartileData* pressureStorageQuartile;
    // temperature
    // moles
};

class ReactionQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit ReactionQuartile(QObject *parent = nullptr);
    virtual ~ReactionQuartile();
    void calculateTotalVolume();
    void setChamber(const QString& chamber);
private:
    // chamber object
    QString profileChamber;
    // sample object
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