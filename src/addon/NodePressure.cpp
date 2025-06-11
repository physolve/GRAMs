#include "NodePressure.h"
#include "../Constants.h"

double VolumeObject::getMoles() const{
    if(pressure<=0 || temperature == 0)
        return 1e-9; 
    return volume * pressure/(10 * Constants::gas_constant * (temperature + Constants::temperature_std_K));
}

VirtualVolume::VirtualVolume(VolumeObject* prior) : VolumeObject(), prior_volume{prior}{
    if(prior!=nullptr){
        qDebug() << "Created " << prior->name << " virtual volume";
        name = prior->name;
        volume = prior->volume;
        updateToPrior(); // ?
    }
}

VirtualVolume::VirtualVolume(const QString& newName, const double& newVolume, VolumeObject* prior) : VolumeObject(), prior_volume{prior}{
    if(prior!=nullptr){
        qDebug() << "Created " << newName << " virtual volume";
        name = newName;
        volume = newVolume;
        updateToPrior(); // ?
    }
}
VirtualVolume::~VirtualVolume(){
    qDebug() << "Virtual volume " << name << " deleted";
    prior_volume = nullptr;
}

VirtualVolume VirtualVolume::operator+(VirtualVolume const& obj){
    const QString& cName = name+obj.name;
    const double& cVolume = volume + obj.volume;
    //  res_C(prior_volume);
    return VirtualVolume(cName, cVolume, prior_volume);
}
VirtualVolume VirtualVolume::operator-(VirtualVolume const& obj){
    const QString& cName = name.remove(obj.name);
    const double& cVolume = volume - obj.volume; 
    // res_C(prior_volume);
    return VirtualVolume(cName, cVolume, prior_volume);
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


void NodePressure::setVolumeA(const VirtualVolume& A){
    m_A = A;
}

void NodePressure::setVolumeB(const VirtualVolume& B){
    m_B = B;
}

VirtualVolume NodePressure::getB() const{
    return m_B;
}

void NodePressure::update(){
    m_A.updateToPrior();
    m_B.updateToPrior();
}
double NodePressure::getEquilibrium() const{
    const double& absTempA = m_A.temperature + Constants::temperature_std_K;
    const double& absTempB = m_B.temperature + Constants::temperature_std_K;
    const double& pressureTotal = 10*Constants::gas_constant*(m_A.getMoles()*absTempA + m_B.getMoles()*absTempB)/(m_A.volume+m_B.volume);
    // temperature?
    return pressureTotal;
}

double NodePressure::getPressureA() const{
    return m_A.pressure;
}

double NodePressure::getPressureB() const{
    return m_B.pressure;
}

QString NodePressure::getNameA() const{
    return m_A.name;
}

QString NodePressure::getNameB() const{
    return m_B.name;
}

VirtualVolume NodePressure::collapse() {
    auto m_C = m_A+m_B;
    m_C.pressure = m_B.pressure = getEquilibrium();
    return m_C;
}

VirtualVolume NodePressure::split(const VirtualVolume& foldedVolume){
    auto m_C = m_A-foldedVolume;
    m_C.pressure = m_A.pressure;
    return m_C;
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

    double getMolesFromPressureChange(const double& pressureChange, const QList<VolumeObject> &volumeObjects) {
        double volume = 0;
        for(const auto& object : volumeObjects){
            volume+=object.volume;
        }
        const double& temp = volumeObjects[0].temperature + Constants::temperature_std_K;
        // chamber temperature? gradient
        return pressureChange*volume/(Constants::gas_constant*temp*10); // moles
    }
};