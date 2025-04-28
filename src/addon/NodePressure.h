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
    VirtualVolume(VolumeObject* prior);
    void updateToPrior();
    double getPriorMoles() const;
    std::unique_ptr<VirtualVolume> operator+(VirtualVolume const& obj);
    std::unique_ptr<VirtualVolume> operator-(VirtualVolume const& obj);
private:
    VolumeObject* prior_volume;
};


class NodePressure
{
public:
    explicit NodePressure();
    virtual ~NodePressure();
    void setVolumeA(VolumeObject* A);
    void setVolumeB(VolumeObject* B);
    double getEquilibrium() const;
    // double getMolesChangeA() const;
    // double getMolesChangeB() const;
    QString getNameA() const;
    QString getNameB() const;
    double getPressureA() const;
    double getPressureB() const;
    std::unique_ptr<VirtualVolume> collapse();
private:
    VolumeObject* m_A;
    VolumeObject* m_B;
};

namespace CalcMoles{
    double getMolesSum(const QList<VolumeObject>& volumes);
    double getPressureFromMoles(const double& moles, const QList<VolumeObject> &volumeObjects);
};

