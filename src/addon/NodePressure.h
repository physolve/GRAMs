#pragma once

#include <QObject>
#include "../ValveModel.h"
// Quartile object is used to store parameters from one of four volumes

static const double R_const = 8.31446;

struct VolumeObject{
    QString name;
    double volume;
    double pressure;
    double temperature;
    double getMoles(){
        return volume * pressure/(10 * R_const * temperature);
    }
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

