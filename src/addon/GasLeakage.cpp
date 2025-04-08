#include "GasLeakage.h"
#include <QDir>
#include <QDebug>

GasLeakage::GasLeakage(QObject *parent) :
    QObject(parent)
{
    qDebug() << "GasLeakage class is created";
    todayRuns = 0;
    m_leakageOpen = false;
}

GasLeakage::~GasLeakage()
{
    qDebug() << "GasLeakage class is destroyed";
}

void GasLeakage::setInitialParametersLeakage(int portId, double turn, double sPressure, double rPressure){
    m_portId = portId;
    m_turn = turn;
    m_storagePressure = sPressure;
    m_reactionPressure = rPressure;
}

int GasLeakage::todayRunCount(){
    QDir dir("data/leakageData");
    // let's find out today run count
    const auto& directoryRunNames = dir.entryList(QStringList() << "*.txt",QDir::Files);
    QString compareToDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString compareToPort = QString("R%1").arg(m_portId);
    auto runs = 0;
    for(const auto& str : directoryRunNames){
        const auto& prepend = str.split('_');
        if(prepend.isEmpty()) continue;
        if(prepend.at(0) == compareToDate && prepend.at(1) == compareToPort && prepend.last() != "model.txt")
            runs++;
    }
    return runs;
}

void GasLeakage::initResultFile(bool debug){
    QDir dir("data");
    dir.cd("leakageData");
    if (!dir.exists())
        dir.mkpath("leakageData"); // doesnt add folder for some reason
    QString model_str = debug ? "_model" : "";
    const auto& baseFileName = QDate::currentDate().toString("yyyy-MM-dd")+QString("_R%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount())+model_str+".txt";
    resultFileSuffix = QString("_R%1_").arg(m_portId)+QString::number(todayRuns=todayRunCount());
    leakageResultFile.setFileName(dir.filePath(baseFileName));
    if (!leakageResultFile.open(QIODevice::ReadWrite)){
        qDebug() << "File don't exist";
        return;
    }
    QTextStream out(&leakageResultFile);
    out << "LeakagePort " << m_portId << "\tCurrent turn " << m_turn << "\tPressure storage " << m_storagePressure << "\tPressure reaction " << m_reactionPressure << "\n";
    out << "Elapsed\t" << "Storage\t" << "Reaction\t" << "Flow rate\t" << "Modelled cm3 H2\t"<< "\n";
    todayRuns++;
}

QString GasLeakage::getResultFileSuffix() const{
    return resultFileSuffix;
}

double GasLeakage::calculateRate(double flow_factor, double sPressure, double rPressure, double rTempAbs) const{
    if(rPressure < 0.528*sPressure){
        return 0.019*1360*flow_factor*sPressure*sqrt((0.53*sPressure)/(0.0696*rTempAbs))*16.6667;
        // return 152.98*flow_factor*sPressure*sqrt(1/(0.07*rTempAbs))*16.6667; // L/min -> 16.6667*cm3/s
        // return 0.471*6950*flow_factor*sPressure*sqrt(1/(Constants::specific_gravity*300))*16.6667; 
    }
    else{
        const auto& diff_pres = (sPressure-rPressure>0)?sPressure-rPressure:0; // sPressure-rPressure:0; // inward
        return 0.013*1360*flow_factor*sPressure*sqrt(diff_pres/(0.0696*rTempAbs))*16.6667;
        // return 306.91*flow_factor*sqrt(diff_pres_sq)/(0.07*rTempAbs)*16.6667;
        // return 6950*flow_factor*sPressure*(1-2*diff_pres/(3*sPressure))*sqrt(diff_pres/(sPressure*Constants::specific_gravity*300))*16.6667; // 300 K is a room temperature (27 C), L/min -> 16.6667*cm3/s
    }
}

void GasLeakage::setLeakageOpen(bool state){
    if(m_leakageOpen&&!progressTime.isValid()){
        progressTime.start();
    }
    m_leakageOpen = state;
}

void GasLeakage::addMeasure(double sPressure, double rPressure, double rTempAbs){
    const auto& flow_factor = getFlowCoefficient(m_turn);
    const auto& flow_rate = calculateRate(flow_factor, sPressure, rPressure, rTempAbs); // positive to reaction quartile 
    if(qIsNaN(flow_rate)){
        qDebug() << "ERROR FLOW";
        return;
    }
    if(m_leakageOpen){
        auto time_pass = 0.0;
        if(progressTime.isValid())
            time_pass = progressTime.nsecsElapsed()/1000000000.0;
        m_timePoints << time_pass;
        m_sPPoints << sPressure;
        m_rPPoints << rPressure;
        m_ratePoints << flow_rate;
        m_modelPassPoints << m_modelPassPoints.last() + flow_rate * (time_pass-last_time_pass);
        last_time_pass = time_pass;
    }
}

bool GasLeakage::addModelMeasure(double model_sPressure, double model_rPressure, double rTempAbs, double time_model){
    const auto& flow_factor = getFlowCoefficient(m_turn);
    const auto& flow_rate = calculateRate(flow_factor, model_sPressure, model_rPressure, rTempAbs); // positive to reaction quartile 
    const auto& time_pass = time_model;
    // without port open check
    // qDebug() << "Model leak time " << time_pass << "; flow " << flow_rate;
    if(qIsNaN(flow_rate))
        return true;
    m_timePoints << time_pass;
    m_sPPoints << model_sPressure;
    m_rPPoints << model_rPressure;
    m_ratePoints << flow_rate;
    m_modelPassPoints << m_modelPassPoints.last() + flow_rate * (time_pass-last_time_pass);
    last_time_pass = time_pass;
    return (model_sPressure - model_rPressure < 0.01);
}

double GasLeakage::getLastFlowPass() const{
    return m_modelPassPoints.last();
}

double GasLeakage::getFlowCoefficient(double turn){
    // case 2 - instant
    switch(m_portId){
        case 0: return turn >= 5 ? 0.00054*turn-0.0014 : 0.0007; break;// for s series from 2 to 8 turns break; //
        case 1: return turn >= 1 ? 0.0035*turn-0.001 : 0.002; break;
        case 2: return 0.0037; break;
        default: return 0; break;
    }
}

void GasLeakage::startCalc(double sPressure, double rPressure, double rTempAbs, double initial_r_flow){
    const auto& flow_factor = getFlowCoefficient(m_turn);
    const auto& diff_pres = sPressure - rPressure; // initial
    const auto& flow_rate = calculateRate(flow_factor, sPressure, rPressure, rTempAbs);
    m_modelPassPoints << initial_r_flow;
    // addPreValveFlow(); // prepare preValveFlow for different supply ports
    last_time_pass = 0;
}

void GasLeakage::saveResultsToFile(){
    QTextStream out(&leakageResultFile);
    for (int i = 0; i < m_timePoints.size(); i++){
        out << m_timePoints[i] << "\t" << m_sPPoints[i] << "\t" << m_rPPoints[i] << "\t" 
        << m_ratePoints[i] << "\t" << m_modelPassPoints[i] << "\n";
    }
    leakageResultFile.close();
}

void GasLeakage::endCalc(){
    m_timePoints.clear();
    m_sPPoints.clear();
    m_rPPoints.clear();
    m_modelPassPoints.clear();
    progressTime.invalidate(); // non model calc
}