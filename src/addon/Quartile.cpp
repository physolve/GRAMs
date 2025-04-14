#include "Quartile.h"

#include <QDebug>

Quartile::Quartile(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Quartile class is created";
}

Quartile::~Quartile(){
    qDebug() << "Quartile class is destroyed";
}

void Quartile::setVolume(double volume){
    m_volume = volume;
}

void Quartile::setMainVolume(const QString& name){
    m_mainVolume = name;
}

void Quartile::addVolume(const QString& name, double volume){
    VolumeObject volumeObject{name, volume, 1, 300};
    // volumeObject.name = name;
    // volumeObject.volume = volume;
    m_volumeObjects[name] = volumeObject;
}

void Quartile::addValvePtrs(const QVector<Valve*>& ptr){
    m_valves = ptr;
}

void Quartile::addPressurePtrs(const QVector<ControllerData*>& ptr){
    m_pressureList = ptr;
}

void Quartile::addTemperaturePtrs(const QVector<ControllerData*>& ptr){
    m_temperatureList = ptr;
}

void Quartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){

}

AddRemoveQuartile::AddRemoveQuartile(QObject *parent) : Quartile(parent), m_expUpdate(new QTimer){
    for(int i{2}; i >= 0; --i){
        m_supplyPort[i].setInitialParametersSupply(i,0,1);
    }
    connect(m_expUpdate, &QTimer::timeout, this, &AddRemoveQuartile::expEvent);
    m_expUpdate->setInterval(500);
}
AddRemoveQuartile::~AddRemoveQuartile(){
    m_supplyPressureHigh = nullptr;
    m_supplyPressureLow = nullptr;
}

void AddRemoveQuartile::setSupplyPressurePtr(FilterData* high, FilterData* low){
    m_supplyPressureHigh = high;
    m_supplyPressureLow = low;
}

void AddRemoveQuartile::setStorageQuartilePtr(Quartile* storageQuartile){
    m_storageQuartile = storageQuartile;
}

void AddRemoveQuartile::setStorageQuartilePressure(QuartileData* storageQuartilePressure){
    m_storageQuartilePressure = storageQuartilePressure;
}

int AddRemoveQuartile::getLightPlotPtr(LightPlotItem* lightPlotPointer){
    QVector<FilterData*> chartPtrs;
    switch(m_supplyPressurePlots.count()){
        // i don't have quartile filter data
        case 0:
        {
            chartPtrs.append(m_supplyPressureHigh);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        case 1:
        {
            chartPtrs.append(m_supplyPressureLow);
            lightPlotPointer->setDataPointers(chartPtrs);
            break;
        }
        default:
            qDebug() << "default\n"; // no error
            break;
    }
    lightPlotPointer->initCustomPlot();
    lightPlotPointer->placeGraph();
    m_supplyPressurePlots << lightPlotPointer;
    return m_supplyPressurePlots.count() - 1;
}

void AddRemoveQuartile::setSupplyAdjustParameters(QVariantMap parameters){
    m_currentSupplyPort = parameters["supplyPort"].toInt();
    const auto& turn = parameters["turn"].toDouble();
    const auto& portPressure = parameters["portPressure"].toDouble();
    m_supplyPort[m_currentSupplyPort].setInitialParametersSupply(m_currentSupplyPort, turn, portPressure);
    qDebug() << "Begin supply with parameters:" << QString("%1 %2 %3").arg(m_currentSupplyPort).arg(turn).arg(portPressure);

    preCalculateSupplyTime(m_currentSupplyPort, turn, portPressure);
}
void AddRemoveQuartile::startSupplyMeasure(bool measure){
    if(measure){
        qDebug() << "Current supply port state: " << m_valves[m_currentSupplyPort]->getState();
        m_supplyPort[m_currentSupplyPort].initResultFile();
        // initial_flow = quartile_storage->moles to std cm3
        const auto& initial_flow = static_cast<StorageQuartile*>(m_storageQuartile)->getQuartileMoleVolume(); // maybe change to 
        m_supplyPort[m_currentSupplyPort].startCalc(m_storageQuartilePressure->getCurValue(), initial_flow);
        m_supplyPressurePlots[0]->initPlotData(); // only high pressure
        m_expUpdate->start();
    }
    else{
        m_expUpdate->stop();
        //saves
        m_supplyPort[m_currentSupplyPort].saveResultsToFile();
        //clear
        m_supplyPort[m_currentSupplyPort].endCalc();
        m_supplyPressurePlots[0]->savePlotData();  // only high pressure
        m_supplyPressurePlots[0]->clearPlotData();  // only high pressure
        m_currentSupplyPort = -1;
    }
}
void AddRemoveQuartile::expEvent(){
    fillSupplyPortData();
}
void AddRemoveQuartile::updatePortState(){
    // update all ports
    for(int i = 0; i < m_valves.count(); ++i){ // if drain ptr! exceed count to 4
        m_supplyPort[i].setPortOpen(m_valves[i]->getState());
    }
}
void AddRemoveQuartile::fillSupplyPortData(){
    // but graph update values from m_supplyPressureLow
    // m_supplyPressurePlots[1]->dataUpdated();
    // but this graph update values from m_supplyPressureHigh
    m_supplyPressurePlots[0]->dataUpdated();

    m_supplyPort[m_currentSupplyPort].setPortOpen(m_valves[m_currentSupplyPort]->getState());
    m_supplyPort[m_currentSupplyPort].addMeasure(m_storageQuartilePressure->getCurValue());
}

void AddRemoveQuartile::preCalculateSupplyTime(int portId, double turn, double portPressure){
    // initial_flow = quartile_storage->moles to std cm3
    SupplyPort model_port;
    model_port.setInitialParametersSupply(portId, turn, portPressure);
    model_port.initResultFile(true);
    StorageQuartile* storageQuartile = static_cast<StorageQuartile*>(m_storageQuartile);
    const auto& initial_flow = storageQuartile->getQuartileMoleVolume();
    model_port.startCalc(m_storageQuartilePressure->getCurValue(), initial_flow);
    model_port.setPortOpen(true);
    double model_pressure = m_storageQuartilePressure->getCurValue();
    double model_time;
    for(model_time = 0; model_time < 10; model_time += 0.01){
        if(model_port.addModelMeasure(model_pressure, model_time)) break;
        const double& flow_pass = model_port.getFlowPass();
        model_pressure = storageQuartile->getQuartileModelPressure(flow_pass);
        // target check
        // total time
    }
    model_port.setPortOpen(false);
    qDebug() << "Total model_time = " << model_time;
    // this total time can be used to stop supply
    model_port.saveResultsToFile();
}

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

void StorageQuartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){
    // gas store valves   
    m_pressureNodes.insert(nodeName, NodePressure());
    m_pressureNodes[nodeName].setVolumeA(&m_volumeObjects[volA]);
    m_pressureNodes[nodeName].setVolumeB(&m_volumeObjects[volB]);
    // recalculate node? and moles?
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

void StorageQuartile::fillVolumePairs(const QMap<QString,QString>& volumeToValve){
    for(const auto& [volume, valve] : volumeToValve.asKeyValueRange()){
        int index;
        for(index = 0; index < m_valves.count(); ++index){
            if(m_valves[index]->m_name == valve)
                break;
        }
        m_valveToVolumeList.append({index, volume});
    }
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
    for(auto moleSensor : m_molesDataList){
        moleSensor->addPoint(m_volumeObjects[moleSensor->m_name].getMoles());
    }
}

double StorageQuartile::getQuartileMole() const{
    double moles = 0;
    moles+=m_volumeObjects[m_mainVolume].getMoles();
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            moles+=m_volumeObjects[valveToVolume.second].getMoles();
        }
    }
    return moles;
}

double StorageQuartile::getQuartileMoleVolume() const{
    const auto& moles = getQuartileMole();
    return moles*Constants::temperature_std_K*Constants::gas_constant*10/Constants::pressure_std_bar; // moles * K *Jl/(mol*K) / (10^5*bar) -> m3 -> * 10^6 -> cm3
}

double StorageQuartile::getQuartileModelPressure(double model_flow){
    const auto& moles = model_flow*Constants::pressure_std_bar/(Constants::temperature_std_K*Constants::gas_constant*10);
    double volume = m_volumeObjects[m_mainVolume].volume;
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            volume+=m_volumeObjects[valveToVolume.second].volume;
        }
    }
    return moles*Constants::gas_constant*Constants::temperature_std_K*10/volume;
}

double StorageQuartile::getQuartileModelPressureFromMoles(double moles) const{
    double volume = m_volumeObjects[m_mainVolume].volume;
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            volume+=m_volumeObjects[valveToVolume.second].volume;
        }
    }
    const double& temp = m_volumeObjects[m_mainVolume].temperature + Constants::temperature_std_K; 
    return moles*Constants::gas_constant*temp*10/volume; // bar
}

QStringList StorageQuartile::getUsedVolumes() const{
    QStringList usedVolumes;
    usedVolumes << m_volumeObjects[m_mainVolume].name;
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            usedVolumes << m_volumeObjects[valveToVolume.second].name;
        }
    }
    return usedVolumes;
}

ReactionQuartile::ReactionQuartile(QObject *parent) : Quartile(parent), m_expUpdate(new QTimer){
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
    profileChamber = chamber;
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
void ReactionQuartile::addPressureNode(const QString& nodeName, const QString& volA, const QString& volB){
    // same as in storage quartile, make as quartile class method
    // gas store valves   
    m_pressureNodes.insert(nodeName, NodePressure());
    m_pressureNodes[nodeName].setVolumeA(&m_volumeObjects[volA]);
    m_pressureNodes[nodeName].setVolumeB(&m_volumeObjects[volB]);
    // recalculate node? and moles?
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

void ReactionQuartile::fillVolumePairs(const QMap<QString,QString>& volumeToValve){
    // same as in storage quartile, make as quartile class method
    for(const auto& [volume, valve] : volumeToValve.asKeyValueRange()){
        int index;
        for(index = 0; index < m_valves.count(); ++index){
            if(m_valves[index]->m_name == valve)
                break;
        }
        m_valveToVolumeList.append({index, volume});
    }
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
}
void ReactionQuartile::updateMoles(){
    // same as in storage quartile, make as quartile class method
        // chamer object difference
    for(auto moleSensor : m_molesDataList){
        moleSensor->addPoint(m_volumeObjects[moleSensor->m_name].getMoles());
    }
}
double ReactionQuartile::getQuartileMole() const{
    // same as in storage quartile, make as quartile class method
        // chamer object difference
    double moles = 0;
    moles+=m_volumeObjects[m_mainVolume].getMoles();
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            moles+=m_volumeObjects[valveToVolume.second].getMoles();
        }
    }
    return moles;
}
double ReactionQuartile::getQuartileMoleVolume() const{
    // same as in storage quartile, make as quartile class method
        // chamber object difference
    const auto& moles = getQuartileMole();
    return moles*Constants::temperature_std_K*Constants::gas_constant*10/Constants::pressure_std_bar; // moles * K *Jl/(mol*K) / (10^5*bar) -> m3 -> * 10^6 -> cm3
}
double ReactionQuartile::getQuartileModelPressure(double model_flow){
    // chamber object difference
    const auto& moles = model_flow*Constants::pressure_std_bar/(Constants::temperature_std_K*Constants::gas_constant*10);
    double volume = m_volumeObjects[m_mainVolume].volume;
    for(const auto& valveToVolume:m_valveToVolumeList){ // chamber
        if(m_valves[valveToVolume.first]->getState()){
            volume+=m_volumeObjects[valveToVolume.second].volume;
        }
    }
    return moles*Constants::gas_constant*Constants::temperature_std_K*10/volume;
}

double ReactionQuartile::getQuartileModelPressureFromMoles(double moles) const{
    double volume = m_volumeObjects[m_mainVolume].volume;
    for(const auto& valveToVolume:m_valveToVolumeList){ // chamber
        if(m_valves[valveToVolume.first]->getState()){
            volume+=m_volumeObjects[valveToVolume.second].volume;
        }
    }
    const double& temp = m_volumeObjects[m_mainVolume].temperature + Constants::temperature_std_K; 
    return moles*Constants::gas_constant*temp*10/volume; // bar
}

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
    // what volumes are currently used?
    QStringList usedVolumes;
    usedVolumes.append(this->getUsedVolumes());
    usedVolumes.append(m_storageQuartile->getUsedVolumes());
    m_gasLeakage[m_currentGasLeakage].setUsedVolumes(usedVolumes);
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
    QStringList usedVolumes;
    usedVolumes.append(this->getUsedVolumes());
    usedVolumes.append(m_storageQuartile->getUsedVolumes());
    model_leakage.setUsedVolumes(usedVolumes);
    model_leakage.initResultFile(true);
    const auto& initial_reaction_moles = getQuartileMole(); // moles //getQuartileMoleVolume();
    const auto& initial_storarge_moles = m_storageQuartile->getQuartileMole(); // moles
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
        const double& moles_pass = model_leakage.getLastFlowPass();
        const double& moles_taken = initial_storarge_moles - (moles_pass - initial_reaction_moles);
        model_reaction_pressure = getQuartileModelPressureFromMoles(moles_pass);
        model_storage_pressure = m_storageQuartile->getQuartileModelPressureFromMoles(moles_taken);
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
    if(measure){
        qDebug() << "Current leakage state: " << m_valves[m_currentGasLeakage]->getState();
        m_gasLeakage[m_currentGasLeakage].initResultFile();
        // initial_flow = quartile_storage->moles to std cm3
        const double& initial_storage_pressure = m_storageQuartilePressure->getCurValue();
        const double& initial_reaction_pressure = pressureReactionQuartile->getCurValue();
        const auto& initial_flow = getQuartileMoleVolume();
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

QStringList ReactionQuartile::getUsedVolumes() const{
    QStringList usedVolumes;
    usedVolumes << m_volumeObjects[m_mainVolume].name;
    for(const auto& valveToVolume:m_valveToVolumeList){
        if(m_valves[valveToVolume.first]->getState()){
            usedVolumes << m_volumeObjects[valveToVolume.second].name;
        }
    }
    return usedVolumes;
}

SecondLineQuartile::SecondLineQuartile(QObject *parent) :
    Quartile(parent){
        
}
SecondLineQuartile::~SecondLineQuartile(){

}


