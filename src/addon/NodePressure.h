#pragma once

#include <QObject>
#include "../ValveModel.h"
// Quartile object is used to store parameters from one of four volumes


// This is automatically called when '+' is used with
    // between two Complex objects

struct VolumeObject{
    QString name;
    double volume; // cm3
    double pressure; // bar
    double temperature; // C
    double getMoles() const;
};

struct VirtualVolume : public VolumeObject{
    VirtualVolume(VolumeObject* prior = nullptr);
    ~VirtualVolume();
    void updateToPrior();
    double getPriorMoles() const;
    // VirtualVolume getFoldedVolume(const QString& name) const;
    VirtualVolume operator+(VirtualVolume const& obj);
    VirtualVolume operator-(VirtualVolume const& obj);
private:
    VolumeObject* prior_volume;
};


class NodePressure
{
public:
    explicit NodePressure();
    virtual ~NodePressure();
    void setVolumeA(const VirtualVolume& A);
    void setVolumeB(const VirtualVolume& B);
    void update();
    double getEquilibrium() const;
    QString getNameA() const;
    QString getNameB() const;
    double getPressureA() const;
    double getPressureB() const;
    QPair<VirtualVolume,VirtualVolume> collapse();
    VirtualVolume split(const VirtualVolume& foldedVolume);
    
private:
    VirtualVolume m_A;
    VirtualVolume m_B;
};

namespace CalcMoles{
    double getMolesSum(const QList<VolumeObject>& volumes);
    double getPressureFromMoles(const double& moles, const QList<VolumeObject> &volumeObjects);
};

