#pragma once

#include <QObject>
#include <QMap>
#include "../DataCollection.h"
#include "NodePressure.h"
#include "SupplyPort.h"
#include "GasLeakage.h"
#include "LightPlotItem.h"
#include "../measure/Chamber.h"
// Quartile object is used to store parameters from one of four volumes

struct changeToTarget{
    double currentPressure;
    double targetPressure;
    double molesChange;
};

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
    // pressure sensor pointers
    VolumeObject getVolumeByName(const QString& name) const;
    void fillVolumePairs(const QMap<QString,QString>& volumeToValve);
    void addPressureNode(const QString& nodeName, const QString& volA, const QString& volB);
    QMap<QString, NodePressure> getPressureNodes(); 
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
    // QList<NodeData*> m_nodeDataList;
    QMap<QString, NodePressure> m_pressureNodes; // those are stable and will not change otherwise.
    QMap<int, QString> m_valveToVolumeMap;
};
