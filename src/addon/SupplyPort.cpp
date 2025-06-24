#include "SupplyPort.h"
#include <QDir>
#include <QDebug>

SupplyPort::SupplyPort(QObject *parent) :
    QObject(parent)
{
    qDebug() << "SupplyPort class is created";
    m_todayRuns = 0;
    m_portId = 0;
    m_portPressure = 50;
    m_T = 25;
    last_time_pass = 1;
    last_pressure_pass = 0;
    last_rate = 0;
}

SupplyPort::~SupplyPort()
{
    qDebug() << "SupplyPort class is destroyed";
}

int SupplyPort::getTodayRuns(){
    return m_todayRuns;
}

void SupplyPort::setInitialParametersSupply(int portId, double portPressure, double temperature, double startPressure, double actionTime){
    m_portId = portId;
    switch(portId){
        case 0: Cv = 0.0005; break;
        case 1: Cv = 0.0005 * 2 * 1.7e-5; break; // # Convert Cv to SI units (m³/s·Pa^0.5)
        case 2: Cv = 0.0005; break;
    }
    m_portPressure = portPressure;
    m_T = temperature;
    last_time_pass = actionTime;
    last_pressure_pass = startPressure;
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

double SupplyPort::supply(double pressure, double actionTime, double v_S){
    const double& dP = pressure - last_pressure_pass;
    const double& dt = actionTime - last_time_pass;
    const double& rate = dt > 0 ? calcRealRate(dP, dt, v_S) : 0;
    m_timePoints << actionTime;
    m_pressurePoints << pressure;
    m_ratePoints << rate;
    m_volumePoints << v_S;
    last_pressure_pass = pressure;
    last_time_pass = actionTime;
    return dP;
}

double SupplyPort::calcRealRate(double dP, double dt, double v_S){
    return dP*v_S/(Constants::gas_constant*m_T*10)/dt; // моль/с
}


// QString SupplyPort::getResultFileSuffix() const{
//     return m_resultFileSuffix;
// }

void SupplyPort::saveResultsToFile(){
    QDir dir("data");
    dir.cd("supplyData");
    if (!dir.exists())
        dir.mkpath("supplyData"); // doesnt add folder for some reason
    const auto& baseFileName = QDate::currentDate().toString("yyyy-MM-dd")+QString("_AR%1_").arg(m_portId)+QString::number(m_todayRuns=todayRunCount())+".txt";
    // m_resultFileSuffix = QString("_AR%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount());
    QFile supplyResultFile;
    supplyResultFile.setFileName(dir.filePath(baseFileName));
    if (!supplyResultFile.open(QIODevice::ReadWrite)){
        qDebug() << "File don't exist";
        return;
    }
    QTextStream out(&supplyResultFile);
    out << "SupplyPort " << m_portId << "\tPort pressure " << m_portPressure << "\n";
    out << "Elapsed" << "\tPressure" << "\tRate" << "\tVolume"<< "\n";
    m_todayRuns++;

    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_pressurePoints[i] << "\t" 
        << m_ratePoints[i] << "\t" << m_volumePoints[i] << "\n";
    }
    supplyResultFile.close();

    m_ratePoints.clear();
    m_pressurePoints.clear();
    m_timePoints.clear();
    m_volumePoints.clear();
    last_rate = 0;
}

void SupplyPort::modelSupply(double pressureLimit, double v_S){
    double model_pressure = last_pressure_pass;
    double model_time = last_time_pass;
    QList<double> timePoints;
    QList<double> pressurePoints;
    QList<double> ratePoints;
    QList<double> currentVolume;
    const double& dt = 0.01;
    // v_S -> strategy Storage
    for(; model_time < 30; model_time += dt){ // is 30 seconds enough always?
        const double& rate = calcRate(model_pressure);
        const double& pressure_income = getPressureIncome(rate, dt, v_S);
        model_pressure += pressure_income;
        timePoints << model_time;
        pressurePoints << model_pressure;
        ratePoints << rate;
        last_rate = rate;
        currentVolume << v_S;
        // target check
        if(model_pressure > pressureLimit || abs(m_portPressure - model_pressure) < 0.01){
            qDebug() << "Model stopped by pressure limit ";
            break;
        }
        // if(model_time > m_inletStrategy.m_openTime/1000){ //ms
        //     qDebug() << "Model stopped by time limit ";
        //     break;
        // }
        // total time
    }
    qDebug() << "Supply model pressure " << model_pressure << " at time " << model_time;
    saveModelFile(timePoints, pressurePoints, ratePoints, currentVolume);
}

double SupplyPort::getPressureIncome(double rate, double dt, double v_S){
    return rate*dt*Constants::gas_constant*m_T/v_S *10;
}

double SupplyPort::calcRate(double pressure){
    double rate = 0;
    if(pressure > 0.528*m_portPressure) { // hydrogen only
        const double& presPart = Cv*m_portPressure*1.01*1e5;
        const double& gammaPart = sqrt(2*Constants::gamma_H/(Constants::gas_constant/Constants::M_H*(Constants::gamma_H-1)*m_T));
        const double& underRoot = pow(pressure/m_portPressure,2/Constants::gamma_H)-pow(pressure/m_portPressure,(Constants::gamma_H+1)/Constants::gamma_H);
        const double& relPresPart = underRoot > 0 ? sqrt(underRoot) : 0;
        const double& convergePart = pressure < m_portPressure ? 0.3*sqrt(m_portPressure - pressure) : 0;
        rate = convergePart*presPart*gammaPart*relPresPart;
    }
    else{
        const double& presPart = Cv*m_portPressure*1.01*1e5;
        const double& rootPart = sqrt(Constants::gamma_H/(Constants::gas_constant/Constants::M_H*m_T));
        const double& powerPart = pow(2/(Constants::gamma_H+1),(Constants::gamma_H+1)/(2*(Constants::gamma_H-1)));
        rate = presPart*rootPart*powerPart;
    }
    return rate/ Constants::M_H; // кг/c / кг/моль -> моль/c
}

void SupplyPort::saveModelFile(const QList<double>& timePoints, const QList<double>& pressurePoints, const QList<double>& ratePoints, const QList<double>& volumePoints){
    QDir dir("data");
    dir.cd("supplyData");
    if (!dir.exists())
        dir.mkpath("supplyData"); // doesnt add folder for some reason
    QString model_str = "_model";
    const auto& baseFileName = QDate::currentDate().toString("yyyy-MM-dd")+QString("_AR%1_").arg(m_portId)+QString::number(m_todayRuns=todayRunCount())+model_str+".txt";
    // m_resultFileSuffix = QString("_AR%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount());
    QFile modelResultFile;
    modelResultFile.setFileName(dir.filePath(baseFileName));
    if (!modelResultFile.open(QIODevice::ReadWrite)){
        qDebug() << "File don't exist";
        return;
    }
    QTextStream out(&modelResultFile);
    out << "SupplyPort " << m_portId << "\tPort pressure " << m_portPressure << "\n";
    out << "Elapsed" << "\tPressure" << "\tRate"<< "\n";
    for (int i = 0; i < timePoints.size(); i++){
        out << timePoints[i] << "\t" << pressurePoints[i] << "\t" 
        << ratePoints[i] << "\t" << volumePoints[i] << "\n";
    }
    modelResultFile.close();
}

double SupplyPort::getLastRate(){
    return last_rate;
}