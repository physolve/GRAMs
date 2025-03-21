#include "SupplyPort.h"

#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";
    supplyResultCount = 0;
}

SupplyPort::~SupplyPort()
{
    m_supplyPressure = nullptr;
    pseudo_time = nullptr;
    qDebug() << "SupplyPort class is destroyed";
}

void SupplyPort::setInitialParametersSupply(int portId, double turn, double portPressure){
    m_portId = portId;
    m_turn = turn;
    m_portPressure = portPressure;
}

void SupplyPort::initResultFile(){
    supplyResultFile.setFileName(QString("data/SupplyPort%1_%2.txt").arg(m_portId).arg(supplyResultCount));
    if (!supplyResultFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "File don't exist";
        return;
    }
    supplyResultCount++;
    QTextStream out(&supplyResultFile);
    out << "SupplyPort " << m_portId << "\tCurrent turn" << m_turn << "\tPort pressure " << m_portPressure << "\tSystem pressure " << m_supplyPressure->getCurValue() << "\n";
    out << "Elapsed\t" << "Pressure\t" << "Modelled cm3 H2\t"<< "\n";
}

void SupplyPort::startCalc(){
    const auto& flow_factor = getFlowCoefficient(m_turn)/1.156;
    const auto& diff_pres = m_portPressure - m_supplyPressure->getCurValue(); // initial
    calculateRate(flow_factor, diff_pres);
    m_flowPass = 0;
    last_time_pass = 0;
}

void SupplyPort::addMeasure(double time_pass){
    m_timePoints << time_pass;
    m_pressurePoints << m_supplyPressure->getCurValue();
    const auto& flow_factor = getFlowCoefficient(m_turn);
    const auto& diff_pres = m_portPressure - m_supplyPressure->getCurValue(); 
    calculateRate(flow_factor, diff_pres);
    m_flowPass += calcualteModelPass(time_pass-last_time_pass);
    m_modelPassPoints << m_flowPass;
}

double SupplyPort::getFlowCoefficient(double turn){
    return 0.004*turn-0.003; // for s series from 2 to 8 turns
}

void SupplyPort::calculateRate(double flow_factor, double diff_pres){
    // 
    // if p_B < 1/2*p_inlet
    // Q = Cv/sqrt(SG/dP) = Cv*sqrt(dP/SG)
    //
    m_currentRate = flow_factor * sqrt(diff_pres/specific_gravity); // m3/h
}

double SupplyPort::calcualteModelPass(double time_pass){
    return m_currentRate * 16.6667 * time_pass; // std L/min * s -> 16.6667*cm3/s*s -> cm3
}

void SupplyPort::saveResultsToFile(){
    QTextStream out(&supplyResultFile);
    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_pressurePoints[i] << "\t" << m_modelPassPoints[i] << "\n";
    }
    supplyResultFile.close();
}

void SupplyPort::endCalc(){
    m_timePoints.clear();
    m_pressurePoints.clear();
    m_modelPassPoints.clear();
}

