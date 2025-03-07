#pragma once

#include <QObject>
#include <QMap>
#include "../DataCollection.h"
#include "NodePressure.h"

// Quartile object is used to store parameters from one of four volumes
class Quartile : public QObject
{
    Q_OBJECT // ?
public:
    explicit Quartile(QObject *parent = nullptr);
    virtual ~Quartile();
    void setVolume(double volume);
    void addVolume(const QString& name, double volume);
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
private:
    double m_supplySpeed;
    double m_drainSpeed;
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