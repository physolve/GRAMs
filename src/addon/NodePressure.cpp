#include "NodePressure.h"

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
    const double& pressureTotal = R_const*(moleTempA + moleTempB)/(m_A->volume+m_B->volume);
    // temperature?
    return pressureTotal;
}