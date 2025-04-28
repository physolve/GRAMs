#include "NodePressure.h"
#include "../Constants.h"

double VolumeObject::getMoles() const{
    if(pressure<=0 || temperature == 0)
        return 1e-9; 
    return volume * pressure/(10 * Constants::gas_constant * (temperature + Constants::temperature_std_K));
}

VirtualVolume::VirtualVolume(VolumeObject* prior) : VolumeObject(), prior_volume{prior}{
    name = prior->name;
    volume = prior->volume;
    updateToPrior();
}

std::unique_ptr<VirtualVolume> VirtualVolume::operator+(VirtualVolume const& obj){
    auto res_C = std::make_unique<VirtualVolume>(prior_volume);
    // VirtualVolume* res_C = new VirtualVolume(prior_volume);
    res_C->name = name+obj.name;
    res_C->volume = volume + obj.volume;
    return res_C;
    // return res_C;
}

std::unique_ptr<VirtualVolume> VirtualVolume::operator-(VirtualVolume const& obj){
    auto res_C = std::make_unique<VirtualVolume>(prior_volume);
    res_C->name = name.remove(obj.name);
    res_C->volume = volume - obj.volume;
    return res_C;
}

void VirtualVolume::updateToPrior(){
    pressure = prior_volume->pressure;
    temperature = prior_volume->temperature;
}

double VirtualVolume::getPriorMoles() const{
    if(prior_volume->pressure<=0 || prior_volume->temperature == 0)
        return 1e-9; 
    return volume * prior_volume->pressure/(10 * Constants::gas_constant * (prior_volume->temperature + Constants::temperature_std_K));
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

double NodePressure::getPressureA() const{
    return m_A->pressure;
}

double NodePressure::getPressureB() const{
    return m_B->pressure;
}

QString NodePressure::getNameA() const{
    return m_A->name;
}

QString NodePressure::getNameB() const{
    return m_B->name;
}

std::unique_ptr<VirtualVolume> NodePressure::collapse(){
    return VirtualVolume(m_A) + VirtualVolume(m_B); 
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