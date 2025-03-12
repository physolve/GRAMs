#include "SupplyPort.h"

#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";

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
    supplyResultFile.setFileName(QString("data/SupplyPort%1.txt").arg(m_portId));
    if (!supplyResultFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&supplyResultFile);
    out << "SupplyPort " << m_portId << "\tCurrent turn" << m_turn << "\tPort pressure " << m_portPressure << "\tSystem pressure " << m_supplyPressure->getCurValue() << "\n";
    out << "Elapsed\t" << "Pressure\t" << "Modelled cm3 H2\t"<< "\n";
}

void SupplyPort::addMeasure(double time_pass){
    m_timePoints << time_pass;
    m_pressurePoints << m_portPressure;
    const auto& flow_factor = getFlowCoefficient(m_turn)/1.156;
    const auto& diff_pres = m_portPressure - m_supplyPressure->getCurValue(); 
    calculateRate(flow_factor, diff_pres);
    m_flowPass += calcualteModelPass(time_pass-last_time_pass);
    m_modelPassPoints << m_flowPass;
}

void SupplyPort::saveResultsToFile(){
    QTextStream out(&supplyResultFile);
    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_pressurePoints[i] << "\t" << m_modelPassPoints[i] << "\n";
    }
    supplyResultFile.close();
}

void SupplyPort::startCalc(){
    const auto& flow_factor = getFlowCoefficient(m_turn)/1.156;
    const auto& diff_pres = m_portPressure - m_supplyPressure->getCurValue(); // initial
    calculateRate(flow_factor, diff_pres);
    m_flowPass = 0;
    last_time_pass = 0;
}

double SupplyPort::getFlowCoefficient(double turn){
    return 0.004*turn-0.003;
}

void SupplyPort::calculateRate(double flow_factor, double diff_pres){
    // 
    // Cv = Q*sqrt(SG/dP)
    // => Q = Cv/sqrt(SG/dP) = Cv*sqrt(dP/SG)
    //
    m_currentRate = flow_factor * sqrt(diff_pres/specific_gravity); // m3/h
}

double SupplyPort::calcualteModelPass(double time_pass){
    return m_currentRate * 227.778 * time_pass; // m3/h * s -> 227.778*cm3/s*s -> cm3
}

