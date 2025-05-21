#include "StorageQuartile.h"

StorageQuartile::StorageQuartile(QObject *parent) :
    Quartile(parent){
        
}
StorageQuartile::~StorageQuartile(){

}

void StorageQuartile::calculateTotalVolume(){
    double totalVolume = 0;
    for(const auto& volume : m_volumeObjects){
        totalVolume += volume.volume;
    }
    setVolume(totalVolume);
}

void StorageQuartile::setQuartileDataPressure(QuartileData* quartileData){
    pressureStorageQuartile = quartileData;
}

void StorageQuartile::setQuartileDataTemperature(QuartileData* quartileData){
    temperatureStorageQuartile = quartileData;
}

void StorageQuartile::setCVolumePtr(const QVector<DataCollection*>& ptr){
    cVolumePressure = ptr;
}

void StorageQuartile::setBD1VolumePtr(DataCollection* ptrB, DataCollection* ptrD1){
    bVolumePressure = ptrB;
    d1VolumePressure = ptrD1;
}

void StorageQuartile::setMolesPtr(const QVector<MolesData*>& ptr){
    m_molesDataList = ptr;
}

void StorageQuartile::setIndexValveRange(int index){
    v_pressure_range = index;
}

void StorageQuartile::setIndexPressureHighLow(int indexHigh, int indexLow){
    s_pressure_high = indexHigh;
    s_pressure_low = indexLow;
}

void StorageQuartile::setIndexTemperatureMain(int index){
    s_temperature_main = index;
}

QStringList StorageQuartile::getUsedVolumes() const{
    QStringList usedVolumes;
    usedVolumes << m_volumeObjects[m_mainVolume].name;
    for(const auto& [valve, volumeName] : m_valveToVolumeMap.asKeyValueRange()){
        if(m_valves[valve]->getState()){
            usedVolumes << volumeName;
        }
    }
    return usedVolumes;
}

void StorageQuartile::updateQuartileData(){
    double current_pressure = 0.0;
    // write smooth transition
    if(m_valves[v_pressure_range]->getState()){
        current_pressure = m_pressureList[s_pressure_low]->getCurValue();
    }
    else{
        current_pressure = m_pressureList[s_pressure_high]->getCurValue();
    }
    // logic for smoothing to current_pressure between v_pressure_range
    pressureStorageQuartile->addPoint(current_pressure);
    temperatureStorageQuartile->addPoint(m_temperatureList[s_temperature_main]->getCurValue());
    // put m_volumeObjects valve state
    Valve* c_volume_valves[3] = {m_valves[0], m_valves[1], m_valves[2]}; // use pairs
    for(int i = 0; i < cVolumePressure.count(); ++i){
        if(c_volume_valves[i]->getState()){
            cVolumePressure[i]->addPoint(current_pressure);
        }
        else{
            cVolumePressure[i]->addPoint(cVolumePressure[i]->getCurValue());
        }
    }
    if(m_valves[v_pressure_range]->getState()){
        d1VolumePressure->addPoint(current_pressure);
    }
    else{
        d1VolumePressure->addPoint(m_pressureList[s_pressure_low]->getCurValue());
    }
    bVolumePressure->addPoint(current_pressure);
    updateVolumeObjects();
    updateMoles();
}

void StorageQuartile::updateVolumeObjects(){
    m_volumeObjects["C1"].pressure = cVolumePressure[0]->getCurValue();
    m_volumeObjects["C1"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["C2"].pressure = cVolumePressure[1]->getCurValue();
    m_volumeObjects["C2"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["C3"].pressure = cVolumePressure[2]->getCurValue();
    m_volumeObjects["C3"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["B"].pressure = bVolumePressure->getCurValue();
    m_volumeObjects["B"].temperature = temperatureStorageQuartile->getCurValue();
    m_volumeObjects["D1"].pressure = d1VolumePressure->getCurValue();
    m_volumeObjects["D1"].temperature = temperatureStorageQuartile->getCurValue();
}

void StorageQuartile::updateMoles(){
    // moles data? m_molesDataList
    for(auto moleSensor : m_molesDataList){
        moleSensor->addPoint(m_volumeObjects[moleSensor->m_name].getMoles());
    }
}

double StorageQuartile::getTargetFromMolesChange(const double& molesChange){
    QList<VolumeObject> storageVolumes;
    // not currently used, but will be used
    for(const QString& name : getUsedVolumes()){
        storageVolumes << getVolumeByName(name);
    }
    return CalcMoles::getPressureFromMoles(molesChange,storageVolumes);
}

changeToTarget StorageQuartile::getChangeToTarget(const double& targetPressure){
    changeToTarget a;
    a.targetPressure = targetPressure;
    a.currentPressure = pressureStorageQuartile->getCurValue();
    const double& pressureChange = a.targetPressure - a.currentPressure; 
    QList<VolumeObject> storageVolumes;
    // not currently used, but will be used
    for(const QString& name : getUsedVolumes()){
        storageVolumes << getVolumeByName(name);
    }
    a.molesChange = CalcMoles::getMolesFromPressureChange(pressureChange, storageVolumes);
    return a;
}