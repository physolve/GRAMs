#include "AddRemoveQuartile.h"
#include "StorageQuartile.h"
AddRemoveQuartile::AddRemoveQuartile(QObject *parent) : Quartile(parent),
m_inletStrategy{50, "AR2", 45, 10000} // middle default port
{ 

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

void AddRemoveQuartile::setStorageQuartileTemperature(QuartileData* storageQuartileTemperature){
    m_storageQuartileTemperature = storageQuartileTemperature;
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

void AddRemoveQuartile::fillSupplyActionData(unsigned int nowTime){
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    const double& start_pressure = m_storageQuartilePressure->getCurValue();
    const double& quartile_temperature = m_storageQuartileTemperature->getCurValue() + Constants::temperature_std_K;
    const double& timeSeconds = nowTime/1000.0;

    m_supplyPort[indexPort].setInitialParametersSupply(indexPort, m_inletStrategy.m_reducerLimit, quartile_temperature, start_pressure, timeSeconds);
    preCalculateSupplyTime(indexPort, start_pressure);
    m_addRemoveGraphs[0]->initPlotData("supplyData", QString("_AR%1_%2").arg(indexPort).arg(m_supplyPort[indexPort].getTodayRuns()));
}

void AddRemoveQuartile::preCalculateSupplyTime(int portId, double start_pressure){
    // initial_flow = quartile_storage->moles to std cm3
    // QList<VolumeObject> storageVolumes;
    double storageVolume = 0;
    for(const QString& name : m_storageQuartile->getUsedVolumes()){
        storageVolume += m_storageQuartile->getVolumeByName(name).volume;
        // variations from strategy in storage quartile
    }
    // strategy storage
    m_supplyPort[portId].modelSupply(m_inletStrategy.m_pressureLimit, storageVolume);
}

bool AddRemoveQuartile::appendSupplyActionData(unsigned int nowTime){
    // but graph update values from m_supplyPressureLow
    m_addRemoveGraphs[0]->dataWithTime(nowTime);
    // but this graph update values from m_supplyPressureHigh
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    const double& s_pressure = m_supplyPressureHigh->getCurValue(); // or low
    double storageVolume = 0;
    for(const QString& name : m_storageQuartile->getUsedVolumes()){
        storageVolume += m_storageQuartile->getVolumeByName(name).volume;
        // variations from strategy in storage quartile
    }
    const double& timeSeconds = nowTime/1000.0;
    const double& pressure_income = m_supplyPort[indexPort].supply(s_pressure, timeSeconds, storageVolume);
    emit rateSupplyChanged();
    // check supply action future
    if(s_pressure + pressure_income > m_inletStrategy.m_pressureLimit){
        qDebug() << "Predict to stop";
        return false;
    }
    return true;
}

void AddRemoveQuartile::saveSupplyActionData(){
    QStringList supplyPortNames = {"AR1", "AR2", "AR3"}; // from profile
    const int& indexPort = supplyPortNames.indexOf(m_inletStrategy.m_usePort);
    //saves
    m_supplyPort[indexPort].saveResultsToFile();
    emit rateSupplyChanged();
    //clear
    m_addRemoveGraphs[0]->savePlotData();  // only high pressure
    m_addRemoveGraphs[0]->clearPlotData();  // only high pressure
}

QList<double> AddRemoveQuartile::getRateSupply(){
    QList<double> rateSupply; // mmol/s
    rateSupply << m_supplyPort[0].getLastRate()*1000 << m_supplyPort[1].getLastRate()*1000 << m_supplyPort[2].getLastRate()*1000;
    return rateSupply; 
}