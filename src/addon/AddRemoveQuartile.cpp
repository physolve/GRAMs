#include "AddRemoveQuartile.h"

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

void AddRemoveQuartile::setStorageQuartilePtr(StorageQuartile* storageQuartile){
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
        
        // case of supply to all volumes including reaction?
        QList<VolumeObject> storageVolumes;
        for(const QString& name : m_storageQuartile->getUsedVolumes()){
            storageVolumes << m_storageQuartile->getVolumeByName(name);
            // allVolumeNames << name;
        }
        // initial_flow = quartile_storage->moles to std cm3
        const auto& initial_flow = CalcMoles::getMolesSum(storageVolumes);  // maybe change to 
        
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