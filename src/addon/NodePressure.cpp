#include "NodePressure.h"
#include "../Constants.h"

double VolumeObject::getMoles() const{
    if(pressure<=0 || temperature == 0)
        return 1e-9; 
    return volume * pressure/(10 * Constants::gas_constant * (temperature + Constants::temperature_std_K));
}

NodePressure::NodePressure()
{
}

NodePressure::~NodePressure(){
}

void NodePressure::setVolumeA(VolumeObject* A){
    m_A = A;
}

void NodePressure::setVolumeB(VolumeObject* B){
    m_B = B;
}   

double NodePressure::getEquilibrium() const{
    const double& moleTempA = m_A->getMoles()*m_A->temperature;
    const double& moleTempB = m_B->getMoles()*m_B->temperature;
    const double& pressureTotal = Constants::gas_constant*(moleTempA + moleTempB)/(m_A->volume+m_B->volume);
    // temperature?
    return pressureTotal;
}

namespace CalcMoles{

    double getMolesSum(const QList<VolumeObject> &volumes){
        double moles = 0;
        for(const auto& volume : volumes){
            moles+=volume.getMoles();
        }
        return moles;
    }

    double getPressureFromMoles(const double& moles, const QList<VolumeObject> &volumeObjects) { // rename
        double volume = 0;
        for(const auto& object : volumeObjects){
            volume+=object.volume;
        }
        const double& temp = volumeObjects[0].temperature + Constants::temperature_std_K; // pass main
        // chamber temperature? gradient
        return moles*Constants::gas_constant*temp*10/volume; // bar, real
    }
};