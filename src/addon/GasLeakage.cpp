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

void GasLeakage::setVolumeNames(const QStringList& volumesNames){
    m_volumeNames = volumesNames;
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
    out << "LeakagePort " << m_portId << "\tCurrent turn " << m_turn << "\tPressure storage " << m_storagePressure << "\tPressure reaction " << m_reactionPressure
    << "\tUsed volumes:";
    int i = m_volumeNames.count()-1;
    for(const auto& str : m_volumeNames){
        out << str << (i == 0 ? "\n" : ", ");
        --i;
    }
    out << "Elapsed\t" << "Storage\t" << "Reaction\t" << "Flow rate\t" << "Modelled cm3 H2\t"<< "\n";
    todayRuns++;
}

QString GasLeakage::getResultFileSuffix() const{
    return resultFileSuffix;
}

double GasLeakage::calculateRate(double sPressure, double rPressure, double rTempAbs) const{
    const double& gamma = Constants::gamma_H;
    const double& M = Constants::M_H;
    const double& R = Constants::gas_constant;
    const double& Pup = sPressure*1e5;
    const double& Pdown = rPressure*1e5;
    const double& critical_p = pow((2 / (gamma + 1)),(gamma / (gamma - 1))); 
    const double& Cv = m_flow_coef * 1.7e-5; // (m³/s·Pa^0.5)
    const double& T = rTempAbs;
    if(Pdown / Pup < critical_p){
        const double& m_dot = Cv * m_choked_curr * Pup * sqrt((gamma * M) / (R * T) * pow((2 / (gamma + 1)),((gamma + 1) / (gamma - 1))));  
        // CHECK equation      
        // const double& m_dot = Cv * m_choked_curr * Pup * sqrt((gamma * M) / (R * T)) * pow((2 / (gamma + 1)),((gamma + 1) /(2*(gamma - 1))));        
        return m_dot / M; // кг/c / кг/моль -> моль/c
    }
    else{
        const double& term = pow((Pdown / Pup),(2 / gamma)) - pow((Pdown / Pup),((gamma + 1) / gamma));
        const double& m_dot = Cv  * m_subsonic_curr * Pup * sqrt((2 * gamma * M) / ((gamma - 1) * R * T) * term);
        return m_dot / M; // кг/c / кг/моль -> моль/c
    }
}

void GasLeakage::setLeakageOpen(bool state){
    if(m_leakageOpen&&!progressTime.isValid()){
        progressTime.start();
    }
    m_leakageOpen = state;
}

void GasLeakage::addMeasure(double sPressure, double rPressure, double rTempAbs){
    const auto& flow_rate = calculateRate(sPressure, rPressure, rTempAbs); // positive to reaction quartile 
    // FREQUENLY CHECK - REMOVE
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
        m_ratePoints << flow_rate*Constants::M_H*1e3; // моль/с * кг/моль * 1e3 -> г/с
        m_modelPassPoints << m_modelPassPoints.last() + flow_rate * (time_pass-last_time_pass);
        last_time_pass = time_pass;
    }
}

bool GasLeakage::addModelMeasure(double model_sPressure, double model_rPressure, double rTempAbs, double time_model){
    const auto& flow_rate = calculateRate(model_sPressure, model_rPressure, rTempAbs); // positive to reaction quartile 
    const auto& time_pass = time_model;
    // without port open check
    // qDebug() << "Model leak time " << time_pass << "; flow " << flow_rate;
    if(qIsNaN(flow_rate))
        return true;
    m_timePoints << time_pass;
    m_sPPoints << model_sPressure;
    m_rPPoints << model_rPressure;
    m_ratePoints << flow_rate*Constants::M_H*1e3; // моль/с * кг/моль * 1e3 -> г/с
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
        case 0: return turn >= 1 ? 0.00379*turn-0.00129 : 0.00085 * 0.8; break; // for m series from 1 to 8 turns
        case 1: return turn >= 5 ? 0.00055*turn-0.00145 : 0.00017; break; // for s series from 5 to 8 turns 
        case 2: return 0.05; break;
        default: return 0; break;
    }
}

void GasLeakage::startCalc(double sPressure, double rPressure, double rTempAbs, double initial_r_flow){
    m_flow_coef = getFlowCoefficient(m_turn);
    m_choked_curr = 1.35*log10(sPressure-rPressure);
    m_subsonic_curr = 0.75*log10(sPressure-rPressure); 
    const auto& flow_rate = calculateRate(sPressure, rPressure, rTempAbs);
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