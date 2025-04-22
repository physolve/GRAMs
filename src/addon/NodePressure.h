#pragma once

#include <QObject>
#include "../ValveModel.h"
// Quartile object is used to store parameters from one of four volumes


struct VolumeObject{
    QString name;
    double volume; // cm3
    double pressure; // bar
    double temperature; // C
    double getMoles() const;
};


class NodePressure
{
public:
    explicit NodePressure();
    virtual ~NodePressure();
    void setVolumeA(VolumeObject* A);
    void setVolumeB(VolumeObject* B);
    double getEquilibrium() const;
    double getMolesChangeA() const;
    double getMolesChangeB() const;
private:
    VolumeObject* m_A;
    VolumeObject* m_B;
};

namespace CalcMoles{
    double getMolesSum(const QList<VolumeObject>& volumes);
    double getPressureFromMoles(const double& moles, const QList<VolumeObject> &volumeObjects);
};

