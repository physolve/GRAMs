#include "SupplyPort.h"

#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";
    supplyResultCount = 0;
    m_portOpen = false;
}

SupplyPort::~SupplyPort()
{
    qDebug() << "SupplyPort class is destroyed";
}

void SupplyPort::setInitialParametersSupply(int portId, double turn, double portPressure){
    m_portId = portId;
    m_turn = turn;
    m_portPressure = portPressure;
}

void SupplyPort::initResultFile(){
    supplyResultFile.setFileName(QString("data/SupplyPort_%1_%2.txt").arg(m_portId).arg(supplyResultCount));
    if (!supplyResultFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "File don't exist";
        return;
    }
    QTextStream out(&supplyResultFile);
    out << "SupplyPort " << m_portId << "\tCurrent turn " << m_turn << "\tPort pressure " << m_portPressure << "\n";
    out << "Elapsed\t" << "Pressure\t" << "Flow rate\t" << "Modelled cm3 H2\t"<< "\n";
    supplyResultCount++;
}

void SupplyPort::startCalc(double pressure_quartile){
    const auto& flow_factor = getFlowCoefficient(m_turn);
    const auto& diff_pres = m_portPressure - pressure_quartile; // initial
    calculateRate(flow_factor, diff_pres);
    m_flowPass = 0;
    last_time_pass = 0;
}

void SupplyPort::setPortOpen(bool state){
    if(m_portOpen&&!progressTime.isValid()){
        progressTime.start();
    }
    m_portOpen = state;
}

void SupplyPort::addMeasure(double pressure_quartile){
    const auto& flow_factor = getFlowCoefficient(m_turn);
    calculateRate(flow_factor, pressure_quartile);
    if(m_portOpen){
        auto time_pass = 0.0;
        if(progressTime.isValid())
            time_pass = progressTime.nsecsElapsed()/1000000000.0;
        m_timePoints << time_pass;
        m_pressurePoints << pressure_quartile;
        m_ratePoints << m_currentRate;
        m_flowPass += calcualteModelPass(time_pass-last_time_pass);
        m_modelPassPoints << m_flowPass;
        last_time_pass = time_pass;
    }
}

double SupplyPort::getFlowCoefficient(double turn){
    return turn < 1 ? 0.0037*turn-0.0024 : 0.001225; // for s series from 2 to 8 turns
}

void SupplyPort::calculateRate(double flow_factor, double pressure){
    // 
    // if p_B < 1/2*p_inlet
    // Q = 0.471*N2*Cv*p_inlet*sqrt(1/(G_g*T_B))
    //
    // if p_B > 1/2*p_inlet
    // Q = N2*Cv*p_inlet*(1-2*dp/(3*p_inlet))*sqrt(dp/(p_inlet*G_g*T_B))
    //
    // N2 = 6950 std L/min (bar, K)
    // G_g = 0.07 (H2)
    const auto& diff_pres = (pressure<m_portPressure)?m_portPressure-pressure:0;
    if(pressure < 0.5*m_portPressure){
        m_currentRate = 0.471*6950*flow_factor*m_portPressure*sqrt(1/(specific_gravity*300))*16.6667; // 300 K is a room temperature (27 C), L/min -> 16.6667*cm3/s
    }
    else{
        m_currentRate = 6950*flow_factor*m_portPressure*(1-2*diff_pres/(3*m_portPressure))*sqrt(diff_pres/(m_portPressure*specific_gravity*300))*16.6667; // 300 K is a room temperature (27 C), L/min -> 16.6667*cm3/s
    }
}

double SupplyPort::calcualteModelPass(double time_pass){
    return m_currentRate * time_pass; // cm3/s*s -> cm3
}

void SupplyPort::saveResultsToFile(){
    QTextStream out(&supplyResultFile);
    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_pressurePoints[i] << "\t" 
        << m_ratePoints[i] << "\t" << m_modelPassPoints[i] << "\n";
    }
    supplyResultFile.close();
}

void SupplyPort::endCalc(){
    m_timePoints.clear();
    m_pressurePoints.clear();
    m_modelPassPoints.clear();
    progressTime.invalidate();
}