#include "FilterView.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


FilterView::FilterView(QObject *parent) : QObject(parent), safeCheck(false), appendCheck(false)
{
    auto dt = (1.0/512);
    this->ui_mA = {1, dt, 0, 0, 1, dt, 0, 0, 1};
    this->ui_mC = {1, 0, 0};

    // Reasonable covariance matrices
    this->ui_mQ = {.001, .001, .0, .001, .001, .0, .0, .0, .0};
    this->ui_mR = 5;
    this->ui_mP = {10000, .1, .1, .1, 10000, 10, .1, 10, 100};
    readKalman();
    parseKalman();
}

FilterView::~FilterView(){
}

void FilterView::readKalman(){
    QDir dir("profile");
    if(!dir.exists()) return;
    QFile file;
    file.setFileName(dir.filePath("kalman.json"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    kalmanProfile = file.readAll();
    file.close();
}

void FilterView::parseKalman(){
    QJsonArray profileJson;
    QJsonDocument document = { QJsonDocument::fromJson(kalmanProfile.toUtf8()) };
    profileJson = document.array();//.object();
    
    const auto &profile = profileJson[0].toObject(); // first filter from array
    const auto& dt = profile["dt"].toVariant().toDouble();
    QList<double> mA;
    for(const auto& var: profile["A"].toArray().toVariantList()){
        mA << var.toDouble();
    }
    QList<double> mC;
    for(const auto& var: profile["C"].toArray().toVariantList()){
        mC << var.toDouble();
    }
    QList<double> mQ;
    for(const auto& var: profile["Q"].toArray().toVariantList()){
        mQ << var.toDouble();
    }
    const auto& mR = profile["R"].toVariant().toDouble();
    QList<double> mP;
    for(const auto& var: profile["P"].toArray().toVariantList()){
        mP << var.toDouble();
    }
    jsonMatrix = {mA, mC, mQ, mR, mP};
}

FilterMatrix FilterView::getJsonMatrix() const{
    return jsonMatrix;
}

void FilterView::safeCheckOn(){
    this->safeCheck = true;
}

bool FilterView::getSafeCheck(){
    return this->safeCheck;
}

void FilterView::saveToFile(const QVector<double> &data){
    QDir dir("output");
    int total_files = dir.count();
    QFile file(QString("output\\output_%1.txt").arg(total_files));
    if(!file.open(QIODevice::Append|QIODevice::Text))
        return;
    QTextStream out(&file);
    
    auto outLambda = [&out](const QList<double> &matrix){
        for(const double &m : matrix){
            out << QString::number(m) <<' ';            
        }
        out << '\n';
    };

    out << "A\t";
    outLambda(ui_mA);
    out << "C\t";
    outLambda(ui_mC);
    out << "Q\t";
    outLambda(ui_mQ);
    out << "P\t";
    outLambda(ui_mP);

    // const auto& timeBuffer = m_channelsData[0]->getTime();
    // if(timeBuffer.isEmpty())
    //     return;
    out << "N" << "\t" << "Data" << "\t" << "Filtered data" << "\t" << "XhatS" << "\t" << "XhatT" << "\n";
    // const auto& bufferFiltered = m_channelsData[0]->getValue();
    // const auto& bufferSecond = m_channelsXhatS[0]->getValue();
    // const auto& bufferThird = m_channelsXhatT[0]->getValue();
    
    // for(int i = 0; i < timeBuffer.count();++i){
        // out << i << "\t" << data[i] << "\t" << bufferFiltered[i] << "\t" << bufferSecond[i] << "\t" << bufferThird[i] << "\n";
    // }
    file.close();
    this->safeCheck = false;
}

void FilterView::appendToFile(const QVector<double> &data){\
    QDir dir("output");
    int total_files = dir.count();
    QFile file(QString("output\\output_%1.txt").arg(total_files-1));
    if(!file.open(QIODevice::Append|QIODevice::Text))
        return;
    QTextStream out(&file);
    // const auto& timeBuffer = m_channelsData[0]->getTime();
    // if(timeBuffer.isEmpty())
    //     return;
    // const auto& bufferFiltered = m_channelsData[0]->getValue();
    // const auto& bufferSecond = m_channelsXhatS[0]->getValue();
    // const auto& bufferThird = m_channelsXhatT[0]->getValue();
    
    // for(int i = 0; i < timeBuffer.count();++i){
    //     out << i << "\t" << data[i] << "\t" << bufferFiltered[i] << "\t" << bufferSecond[i] << "\t" << bufferThird[i] << "\n";
    // }
    file.close();
}

void FilterView::startAppend(bool state){
    this->appendCheck = state;
}

bool FilterView::getAppendCheck(){
    return this->appendCheck;
}

// QSharedPointer<Sensor> FilterView::getChannelSensor(int channel, QString a){
//     if(m_channelsData.isEmpty()){
//         return QSharedPointer<Sensor>::create(QString("empty"));
//     }
//     if(a == "view"){
//         return m_channelsData[channel];
//     }
//     else if( a == "xhats"){
//         return m_channelsXhatS[channel];
//     } 
//     else if(a == "xhatt") return m_channelsXhatT[channel];
// }

void FilterView::setUiA(const QList<double> &ui_A)
{
    if (ui_mA == ui_A)
        return;
    ui_mA = ui_A;
    emit uiAChanged(ui_A);
}
QList<double> FilterView::uiA() const{ 
    return ui_mA;
}
void FilterView::setUiC(const QList<double> &ui_C)
{
    if (ui_mA == ui_C)
        return;

    ui_mC = ui_C;
    emit uiCChanged(ui_C);
}
QList<double> FilterView::uiC() const{ 
    return ui_mC;
}
void FilterView::setUiQ(QList<double> ui_Q)
{
    if (ui_mQ == ui_Q)
        return;

    ui_mQ = ui_Q;
    emit uiQChanged(ui_Q);
}
QList<double> FilterView::uiQ() const{ 
    return ui_mQ;
}
void FilterView::setUiR(double ui_R)
{
    if (ui_mR == ui_R)
        return;

    ui_mR = ui_R;
    emit uiRChanged(ui_R);
}
double FilterView::uiR() const{ 
    return ui_mR;
}
void FilterView::setUiP(QList<double> ui_P)
{
    if (ui_mP == ui_P)
        return;

    ui_mP = ui_P;
    emit uiQChanged(ui_P);
}
QList<double> FilterView::uiP() const{ 
    return ui_mP;
}

FilterMatrix FilterView::getNewFilterParameters() const{
    return {ui_mA, ui_mC, ui_mQ, ui_mR, ui_mP}; 
}