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
    out << "SupplyPort " << m_portId << "\tCurrent turn" << m_turn << "\tPort pressure " << m_portPressure << "\n";
    out << "Elapsed\t" << "Pressure\t" << "Flow rate\t" << "Modelled cm3 H2\t"<< "\n";
    supplyResultCount++;
}

void SupplyPort::startCalc(double pressure_quartile){
    const auto& flow_factor = getFlowCoefficient(m_turn)/1.156;
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
    const auto& flow_factor = getFlowCoefficient(m_turn)/1.156; // Cv = 1.156*Kv
    const auto& diff_pres = m_portPressure - pressure_quartile; 
    calculateRate(flow_factor, diff_pres);
    if(m_portOpen){
        auto time_pass = progressTime.elapsed()/1000.;
        if(time_pass < 0) {
            qDebug() << "Cyka";
            time_pass = 0; 
        }
        m_timePoints << time_pass;
        m_pressurePoints << pressure_quartile;
        m_ratePoints << m_currentRate;
        m_flowPass += calcualteModelPass(time_pass-last_time_pass);
        m_modelPassPoints << m_flowPass;
        last_time_pass = time_pass;
    }
}

double SupplyPort::getFlowCoefficient(double turn){
    return 0.004*turn-0.003;
}

void SupplyPort::calculateRate(double flow_factor, double diff_pres){
    // 
    // Cv = Q*sqrt(SG/dP)
    // => Q = Cv/sqrt(SG/dP) = Cv*sqrt(dP/SG)
    //
    if(diff_pres < 0){
        qDebug() << "Wrong diff_press";
        return;
    }
    m_currentRate = flow_factor * sqrt(diff_pres/specific_gravity); // m3/h
}

double SupplyPort::calcualteModelPass(double time_differ){
    return m_currentRate * 227.778 * time_differ; // m3/h * s -> 227.778*cm3/s*s -> cm3
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