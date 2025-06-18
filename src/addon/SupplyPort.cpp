#include "SupplyPort.h"
#include <QDir>
#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";
    todayRuns = 0;
}

SupplyPort::~SupplyPort()
{
    qDebug() << "SupplyPort class is destroyed";
}

void SupplyPort::setInitialParametersSupply(int portId, double portPressure, double temperature, double actionTime){
    m_portId = portId;
    switch(portId){
        case 0: Cv = 0.0005; break;
        case 1: Cv = 0.0005 * 1.7e-5; break; // # Convert Cv to SI units (m³/s·Pa^0.5)
        case 2: Cv = 0.0005; break;
    }
    m_portPressure = portPressure;
    m_T = temperature;
    last_time_pass = actionTime;
}

int SupplyPort::todayRunCount(){
    QDir dir("data/supplyData");
    // let's find out today run count
    const auto& directoryRunNames = dir.entryList(QStringList() << "*.txt",QDir::Files);
    QString compareToDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString compareToPort = QString("AR%1").arg(m_portId);
    auto runs = 0;
    for(const auto& str : directoryRunNames){
        const auto& prepend = str.split('_');
        if(prepend.isEmpty()) continue;
        if(prepend.at(0) == compareToDate && prepend.at(1) == compareToPort && prepend.last() != "model.txt")
            runs++;
    }
    return runs;
}

void SupplyPort::initResultFile(bool debug){
    QDir dir("data");
    dir.cd("supplyData");
    if (!dir.exists())
        dir.mkpath("supplyData"); // doesnt add folder for some reason
    QString model_str = debug ? "_model" : "";
    const auto& baseFileName = QDate::currentDate().toString("yyyy-MM-dd")+QString("_AR%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount())+model_str+".txt";
    resultFileSuffix = QString("_AR%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount());
    supplyResultFile.setFileName(dir.filePath(baseFileName));
    if (!supplyResultFile.open(QIODevice::ReadWrite)){
        qDebug() << "File don't exist";
        return;
    }
    QTextStream out(&supplyResultFile);
    out << "SupplyPort " << m_portId << "\tPort pressure " << m_portPressure << "\n";
    out << "Elapsed" << "\tPressure" << "\tRate"<< "\n";
    todayRuns++;
}

double SupplyPort::getPressureIncome(double pressure, double v_S, double time_pass){
    const double& rate = calcRate(pressure);
    const double& v_Q = v_S;
    const double& pressure_income = rate*(time_pass-last_time_pass)*Constants::gas_constant*m_T/v_Q *10;
    last_time_pass = time_pass;
    m_ratePoints << rate;
    m_pressurePoints << pressure;
    m_timePoints << time_pass;
    return pressure_income;
}

double SupplyPort::calcRate(double pressure){
    double rate = 0;
    if(pressure > 0.528*m_portPressure) { // hydrogen only
        const double& presPart = Cv*m_portPressure*1.01*10e5;
        const double& gammaPart = sqrt(2*Constants::gamma_H/(Constants::gas_constant/Constants::M_H*(Constants::gamma_H-1)*m_T));
        const double& relPresPart = sqrt(pow(pressure/m_portPressure,2/Constants::gamma_H)-pow(pressure/m_portPressure,(Constants::gamma_H+1)/Constants::gamma_H));
        rate = presPart*gammaPart*relPresPart;
    }
    else{
        rate = Cv*m_portPressure*1.01*10e5*sqrt(Constants::gamma_H/(Constants::gas_constant/Constants::M_H*m_T));
    }
    return rate/ Constants::M_H; // кг/c / кг/моль -> моль/c
}

QString SupplyPort::getResultFileSuffix() const{
    return resultFileSuffix;
}


void SupplyPort::saveResultsToFile(){
    QTextStream out(&supplyResultFile);
    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_pressurePoints[i] << "\t" 
        << m_ratePoints[i] << "\n";
    }
    supplyResultFile.close();
    m_ratePoints.clear();
    m_pressurePoints.clear();
    m_timePoints.clear();
}