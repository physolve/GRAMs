#include "ReactionQuartile.h"

ReactionQuartile::ReactionQuartile(QObject *parent) : Quartile(parent), m_expUpdate(new QTimer),
m_chamber_connected(false){
    for(int i{2}; i >= 0; --i){
        m_gasLeakage[i].setInitialParametersLeakage(i,0,1,1);
    }
    connect(m_expUpdate, &QTimer::timeout, this, &ReactionQuartile::expEvent);
    m_expUpdate->setInterval(500);  
}
ReactionQuartile::~ReactionQuartile(){

}

void ReactionQuartile::calculateTotalVolume(){
    double totalVolume = 0;
    for(const auto& volume : m_volumeObjects){
        totalVolume += volume.volume;
    }
    setVolume(totalVolume);
}

void ReactionQuartile::setChamber(const QString& chamber){
    profileChamber = chamber; // m_chamber->getChamberName
}

void ReactionQuartile::setChamberPointer(Chamber* chamberPtr){
    m_chamber = chamberPtr;
}

void ReactionQuartile::setChamberStatus(bool statusOpen){
    m_chamber->setStatusOpen(statusOpen);
}

void ReactionQuartile::updateChamberToQuartile(){
    m_volumeObjects["F"] = m_chamber->getVolumeObject();
    m_volumeObjects["EF"] = m_chamber->getCraneObject();

    // m_pressureNodes; updating the nodes

}

void ReactionQuartile::removeChamber(){
    // logic to remove from volume objects
}

void ReactionQuartile::setQuartileDataPressure(QuartileData* quartileData){
    pressureReactionQuartile = quartileData;
}

void ReactionQuartile::setQuartileDataTemperature(QuartileData* quartileData){
    temperatureReactionQuartile = quartileData;
}

void ReactionQuartile::setED2VolumePtr(DataCollection* ptrE, DataCollection* ptrD2Atm, DataCollection* ptrD2Low){
    eVolumePressure = ptrE;
    d2VolumePressureAtm = ptrD2Atm;
    d2VolumePressureLow = ptrD2Low;
}

void ReactionQuartile::setFVolumePtr(DataCollection* ptrF){
    fVolumePressure = ptrF;
}

void ReactionQuartile::setMolesPtr(const QVector<MolesData*>& ptr){
    m_molesDataList = ptr;
}

void ReactionQuartile::setIndexValveRange(int index){
    v_pressure_range = index;
}
void ReactionQuartile::setIndexPressureHighLow(int indexHigh, int pressureAtm, int indexLow){
    s_pressure_high = indexHigh;
    s_pressure_atm = pressureAtm;
    s_pressure_low = indexLow;
}
void ReactionQuartile::setIndexTemperatureMain(int index){
    s_temperature_main = index;
}

void ReactionQuartile::setIndexTemperatureChamber(int index){
    s_temperature_chamber = index;
}

QStringList ReactionQuartile::getUsedVolumes() const{
    QStringList usedVolumes;
    usedVolumes << m_volumeObjects[m_mainVolume].name;
    for(const auto& [valve, volumeName] : m_valveToVolumeMap.asKeyValueRange()){
        if(m_valves[valve]->getState()){
            usedVolumes << volumeName;
        }
    }
    if(m_chamber_connected){
        usedVolumes << "EF";
        if(m_chamber->getStatusOpen())
            usedVolumes << "F";
    }
    return usedVolumes;
}

void ReactionQuartile::updateQuartileData(){
    double current_pressure = 0.0;
    // write smooth transition
    if(m_valves[v_pressure_range]->getState()){
        current_pressure = m_pressureList[s_pressure_atm]->getCurValue();
    // profile second valve exists?
        // ladder volume objects
        // logic for choosing low pressure
    }
    else{
        current_pressure = m_pressureList[s_pressure_high]->getCurValue();
    }
    // logic for smoothing to current_pressure between v_pressure_range
    pressureReactionQuartile->addPoint(current_pressure);
    temperatureReactionQuartile->addPoint(m_temperatureList[s_temperature_main]->getCurValue());
    // put m_volumeObjects valve state
    if(m_valves[v_pressure_range]->getState()){
        d2VolumePressureAtm->addPoint(current_pressure);
        // or else d2VolumePressureLow
    }
    else{
        d2VolumePressureAtm->addPoint(m_pressureList[s_pressure_atm]->getCurValue());
        // or else d2VolumePressureLow
    }
    eVolumePressure->addPoint(current_pressure);
    // chamber logic
    if(m_chamber_connected){
        if(m_chamber->getStatusOpen())
            fVolumePressure->addPoint(current_pressure);
        else
            fVolumePressure->addPoint(fVolumePressure->getCurValue()); // assumption
    }

    updateVolumeObjects();
    
    updateMoles();
}

void ReactionQuartile::updateVolumeObjects(){
    // valve states to set volumeObject pressure
    m_volumeObjects["E"].pressure = eVolumePressure->getCurValue();
    m_volumeObjects["E"].temperature = temperatureReactionQuartile->getCurValue();
    m_volumeObjects["D2"].pressure = d2VolumePressureAtm->getCurValue();
    // or else d2VolumePressureLow
    m_volumeObjects["D2"].temperature = temperatureReactionQuartile->getCurValue();
    // chamber object
    if(m_chamber_connected){
        m_volumeObjects["EF"].pressure = eVolumePressure->getCurValue();
        m_volumeObjects["EF"].temperature = temperatureReactionQuartile->getCurValue();
        
        m_volumeObjects["F"].pressure = fVolumePressure->getCurValue();
        m_volumeObjects["F"].temperature = m_temperatureList[s_temperature_chamber]->getCurValue();
    }
}
void ReactionQuartile::updateMoles(){
    // same as in storage quartile, make as quartile class method
        // chamer object difference
    for(auto moleSensor : m_molesDataList){
        moleSensor->addPoint(m_volumeObjects[moleSensor->m_name].getMoles());
    }
}

// temperature gradient m_volumeObjects (E-D2-F) moles function


void ReactionQuartile::setReactionPressurePtr(FilterData* high, FilterData* low){
    m_reactionPressureHigh = high;
    m_reactionPressureLow = low;
}
void ReactionQuartile::setStorageQuartilePressure(QuartileData* storageQuartilePressure){
    m_storageQuartilePressure = storageQuartilePressure;
}
void ReactionQuartile::setStorageQuartilePtr(StorageQuartile* storageQuartile){
    m_storageQuartile = storageQuartile;
}
int ReactionQuartile::getLightPlotPtr(LightPlotItem* lightPlotPointer){
    QVector<FilterData*> chartPtrs;
    switch(m_reactionPressurePlots.count()){
        // i don't have quartile filter data
        case 0:
        {
            chartPtrs.append(m_reactionPressureHigh);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        case 1:
        {
            chartPtrs.append(m_reactionPressureLow);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        default:
            qDebug() << "default\n"; // no error
            break;
    }
    lightPlotPointer->initCustomPlot();
    lightPlotPointer->placeGraph();
    m_reactionPressurePlots << lightPlotPointer;
    return m_reactionPressurePlots.count() - 1;
}

void ReactionQuartile::setReactionAdjustParameters(QVariantMap parameters){
    m_currentGasLeakage = parameters["leakageValve"].toInt();
    const auto& turn = parameters["turn"].toDouble();
    const double& initial_storage_pressure = m_storageQuartilePressure->getCurValue();
    const double& initial_reaction_pressure = pressureReactionQuartile->getCurValue();
    m_gasLeakage[m_currentGasLeakage].setInitialParametersLeakage(
        m_currentGasLeakage, turn, initial_storage_pressure, initial_reaction_pressure
    );
    qDebug() << "Begin leakage with parameters:" << QString("%1 %2 %3 bar %4 bar").arg(m_currentGasLeakage).arg(turn)
    .arg(initial_storage_pressure).arg(initial_reaction_pressure);
    preCalculateLeakageTime(m_currentGasLeakage, turn);
}

void ReactionQuartile::preCalculateLeakageTime(int portId, double turn){
    // initial_flow = quartile_storage->moles to std cm3
    GasLeakage model_leakage;
    const double& initial_storage_pressure = m_storageQuartilePressure->getCurValue();
    const double& initial_reaction_pressure = pressureReactionQuartile->getCurValue();
    model_leakage.setInitialParametersLeakage(portId, turn, initial_storage_pressure, initial_reaction_pressure);
    // what volumes are currently used?

    // model from B to E+F
    // copying wanted objects
    // at this moment
    QStringList allVolumeNames;
    QList<VolumeObject> reactionVolumes;
    for(const QString& name : getUsedVolumes()){
        reactionVolumes << getVolumeByName(name);
        allVolumeNames << name;
    }
    // here you can remove or add unwanted Volumes for model
    
    QList<VolumeObject> storageVolumes;
    for(const QString& name : m_storageQuartile->getUsedVolumes()){
        storageVolumes << m_storageQuartile->getVolumeByName(name);
        allVolumeNames << name;
    }
    model_leakage.setVolumeNames(allVolumeNames); // potentially changing, moles per use

    model_leakage.initResultFile(true);
    const auto& initial_reaction_moles = CalcMoles::getMolesSum(reactionVolumes); // moles //getQuartileMoleVolume();
    const auto& initial_storarge_moles = CalcMoles::getMolesSum(storageVolumes); // moles
    // w/o gradient
    const auto& initial_temp = temperatureReactionQuartile->getCurValue()+Constants::temperature_std_K;

    model_leakage.startCalc(initial_storage_pressure, initial_reaction_pressure, initial_temp, initial_reaction_moles);
    model_leakage.setLeakageOpen(true);
    double model_reaction_pressure = pressureReactionQuartile->getCurValue();
    double model_storage_pressure = m_storageQuartilePressure->getCurValue();
    float seconds_max = 120;
    double model_time;
    for(model_time = 0; model_time < seconds_max; model_time += 0.01){ // more than 10 seconds? -> 120
        if(model_leakage.addModelMeasure(model_storage_pressure, model_reaction_pressure, initial_temp, model_time))
            break;
        // reactionVolumes changing if something (add, remove)
        const double& moles_pass = model_leakage.getLastFlowPass();
        const double& moles_taken = initial_storarge_moles - (moles_pass - initial_reaction_moles);
        model_reaction_pressure = CalcMoles::getPressureFromMoles(moles_pass, reactionVolumes);
        model_storage_pressure = CalcMoles::getPressureFromMoles(moles_taken, storageVolumes);
        // qDebug() << "Model leak pass " << flow_pass << "; taken " << flow_taken;
        // target check
        // total time
    }
    model_leakage.setLeakageOpen(false);
    qDebug() << "Total model_time = " << model_time;
    // this total time can be used to stop supply
    model_leakage.saveResultsToFile();
}

void ReactionQuartile::startLeakageMeasure(bool measure){
    QList<VolumeObject> reactionVolumes;
    for(const QString& name : getUsedVolumes()){
        reactionVolumes << getVolumeByName(name);
    }
    if(measure){
        qDebug() << "Current leakage state: " << m_valves[m_currentGasLeakage]->getState();
        m_gasLeakage[m_currentGasLeakage].initResultFile();

        const double& initial_storage_pressure = m_storageQuartilePressure->getCurValue();
        const double& initial_reaction_pressure = pressureReactionQuartile->getCurValue();
        // starting reactionVolumes has the same values
        const auto& initial_flow = CalcMoles::getMolesSum(reactionVolumes);
        const auto& initial_temp = temperatureReactionQuartile->getCurValue()+Constants::temperature_std_K;
        m_gasLeakage[m_currentGasLeakage].startCalc(initial_storage_pressure, initial_reaction_pressure, initial_temp, initial_flow);
        
        m_reactionPressurePlots[0]->initPlotData("leakageData", m_gasLeakage[m_currentGasLeakage].getResultFileSuffix()); // only high pressure
        
        m_expUpdate->start();
    }
    else{
        m_expUpdate->stop();
        //saves
        m_gasLeakage[m_currentGasLeakage].saveResultsToFile();
        //clear
        m_gasLeakage[m_currentGasLeakage].endCalc();
        m_reactionPressurePlots[0]->savePlotData();  // only high pressure
        m_reactionPressurePlots[0]->clearPlotData();  // only high pressure
        m_currentGasLeakage = -1;
    }
}

void ReactionQuartile::updateLeakageState(){
    // update all ports
    // start from 0?
    for(int i = 0; i < 3; ++i){ // if drain ptr! exceed count to 4
        m_gasLeakage[i].setLeakageOpen(m_valves[i]->getState());
    }
}

void ReactionQuartile::expEvent(){
    fillGasLeakageData();
}

void ReactionQuartile::fillGasLeakageData(){
    // but graph update values from m_reactionPressureLow
    // m_reactionPressurePlots[1]->dataUpdated();
    // but this graph update values from m_reactionPressureHigh
    
    // ASSERT ERROR
    // qDebug() << "Before Assersion";
    m_reactionPressurePlots[0]->dataUpdated();
    // qDebug() << "After Assersion";
    const bool& currentLeakageValve = m_valves[m_currentGasLeakage]->getState();
    const double& storage_pressure = m_storageQuartilePressure->getCurValue();
    const double& reaction_pressure = pressureReactionQuartile->getCurValue();
    const auto& reaction_temp = temperatureReactionQuartile->getCurValue()+Constants::temperature_std_K;
    m_gasLeakage[m_currentGasLeakage].setLeakageOpen(currentLeakageValve);
    m_gasLeakage[m_currentGasLeakage].addMeasure(storage_pressure, reaction_pressure, reaction_temp);
}

changeToTarget ReactionQuartile::getChangeToTarget(const double& targetPressure){
    changeToTarget a;
    a.targetPressure = targetPressure;
    a.currentPressure = pressureReactionQuartile->getCurValue();
    const double& pressureChange = a.targetPressure - a.currentPressure; 
    QList<VolumeObject> reactionVolumes;
    // not currently used, but will be used
    for(const QString& name : getUsedVolumes()){
        reactionVolumes << getVolumeByName(name);
    }
    a.molesChange = CalcMoles::getMolesFromPressureChange(pressureChange, reactionVolumes);
    return a;
}