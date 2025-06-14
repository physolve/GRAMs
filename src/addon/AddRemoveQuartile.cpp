#include "AddRemoveQuartile.h"

AddRemoveQuartile::AddRemoveQuartile(QObject *parent) : Quartile(parent),
m_inletStrategy{50, "AR2", 45, 10000} // middle default port
{ 
    for(int i{2}; i >= 0; --i){
        m_supplyPort[i].setInitialParametersSupply(i,0,1);
    }
}
AddRemoveQuartile::~AddRemoveQuartile(){
    m_supplyPressureHigh = nullptr;
    m_supplyPressureLow = nullptr;
}

void AddRemoveQuartile::setSupplyPressurePtr(FilterData* high, FilterData* low){
    m_supplyPressureHigh = high;
    m_supplyPressureLow = low;
}

void AddRemoveQuartile::setStorageQuartilePtr(StorageQuartile* storageQuartile){
    m_storageQuartile = storageQuartile;
}

void AddRemoveQuartile::setStorageQuartilePressure(QuartileData* storageQuartilePressure){
    m_storageQuartilePressure = storageQuartilePressure;
}

void AddRemoveQuartile::initAddRemoveCharts(){
    auto supplyChart = new LightPlot();
    supplyChart->setDataPointers(m_supplyPressureHigh);
    supplyChart->setPlotColor();
    supplyChart->initPlot();
    supplyChart->placeLegend();
    supplyChart->update();
    supplyChart->dataUpdated();
    supplyChart->m_chartName = "Подача газа";
    m_addRemoveGraphs << supplyChart;
    emit addRemoveGraphsChanged();
}

void AddRemoveQuartile::clearAddRemoveCharts(){
    for(auto* graph : m_addRemoveGraphs){
        delete graph;
    }
    m_addRemoveGraphs.clear();
    // emit addRemoveGraphsChanged();
}

QStringList AddRemoveQuartile::getAddRemoveChartNames() const{
    QStringList chartNames;
    for(auto graph : m_addRemoveGraphs){
        chartNames << graph->m_chartName;
    }
    return chartNames;
}

QList<LightPlot*> AddRemoveQuartile::getAddRemoveGraphs() const
{
    return m_addRemoveGraphs;
}


void AddRemoveQuartile::setInletStrategy(const InletStrategy& inletStrategy){ // from gui
    m_inletStrategy = inletStrategy;
}
InletStrategy AddRemoveQuartile::getInletStrategy() const{
    return m_inletStrategy;
}

void AddRemoveQuartile::updatePortState(){
    // update all ports
    for(int i = 0; i < m_valves.count(); ++i){ // if drain ptr! exceed count to 4
        m_supplyPort[i].setPortOpen(m_valves[i]->getState());
    }
}

bool AddRemoveQuartile::checkSupplyAction(){
    // speed on m_supplyPressureHigh
    // speed on m_supplyPressureLow

    // fast value check with m_supplyPressureLow (if opened)

    // fast value check with m_supplyPressureHigh
    if(m_supplyPressureHigh->getCurValue() > m_inletStrategy.m_pressureLimit){
        return false;
    }
    if(m_storageQuartilePressure->getCurValue() > m_inletStrategy.m_pressureLimit){
        return false;
    }
    return true;
}

void AddRemoveQuartile::fillSupplyActionData(){
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    
    m_supplyPort[indexPort].setInitialParametersSupply(indexPort, 1, m_inletStrategy.m_reducerLimit);
    preCalculateSupplyTime(indexPort, 1, m_inletStrategy.m_reducerLimit);
    
    qDebug() << "Current supply port state: " << m_valves[indexPort]->getState();
    m_supplyPort[indexPort].initResultFile();
    
    // case of supply to all volumes including reaction?
    QList<VolumeObject> storageVolumes;
    for(const QString& name : m_storageQuartile->getUsedVolumes()){
        storageVolumes << m_storageQuartile->getVolumeByName(name);
        // allVolumeNames << name;
    }
    // initial_flow = quartile_storage->moles to std cm3
    const auto& initial_flow = CalcMoles::getMolesSum(storageVolumes);  // maybe change to 
    
    m_supplyPort[indexPort].startCalc(m_storageQuartilePressure->getCurValue(), initial_flow); // begin
    m_addRemoveGraphs[0]->initPlotData(); // only high pressure
}

void AddRemoveQuartile::preCalculateSupplyTime(int portId, double turn, double portPressure){
    // initial_flow = quartile_storage->moles to std cm3
    SupplyPort model_port;
    model_port.setInitialParametersSupply(portId, turn, portPressure);
    model_port.initResultFile(true);

    QList<VolumeObject> storageVolumes;
    for(const QString& name : m_storageQuartile->getUsedVolumes()){
        storageVolumes << m_storageQuartile->getVolumeByName(name);
        // allVolumeNames << name;
    }

    const auto& initial_flow = CalcMoles::getMolesSum(storageVolumes);
    model_port.startCalc(m_storageQuartilePressure->getCurValue(), initial_flow);
    model_port.setPortOpen(true);
    double model_pressure = m_storageQuartilePressure->getCurValue();
    double model_time;
    for(model_time = 0; model_time < 10; model_time += 0.01){
        if(model_port.addModelMeasure(model_pressure, model_time)) break;
        const double& flow_pass = model_port.getFlowPass(); // moles pass
        const double& moles_pass = 10e-4;
        model_pressure = CalcMoles::getPressureFromMoles(moles_pass, storageVolumes);
        // target check
        // total time
    }
    model_port.setPortOpen(false);
    qDebug() << "Total model_time = " << model_time;
    // this total time can be used to stop supply
    model_port.saveResultsToFile();
}

void AddRemoveQuartile::fillSupplyPortData(){
    // but graph update values from m_supplyPressureLow
    // m_supplyPressurePlots[1]->dataUpdated();
    // but this graph update values from m_supplyPressureHigh
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    m_supplyPort[indexPort].setPortOpen(m_valves[indexPort]->getState());
    m_supplyPort[indexPort].addMeasure(m_storageQuartilePressure->getCurValue());
    m_addRemoveGraphs[0]->dataUpdated();
}

void AddRemoveQuartile::saveSupplyActionData(){
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    //saves
    m_supplyPort[indexPort].saveResultsToFile();
    //clear
    m_supplyPort[indexPort].endCalc();
    m_addRemoveGraphs[0]->savePlotData();  // only high pressure
    m_addRemoveGraphs[0]->clearPlotData();  // only high pressure
}