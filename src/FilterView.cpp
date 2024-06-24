#include "FilterView.h"

FilterView::FilterView(QObject *parent) : QObject(parent)
{
    auto dt = (1.0/500);
    this->ui_mA = {1, dt, 0, 0, 1, dt, 0, 0, 1};
    this->ui_mC = {1, 0, 0};

    // Reasonable covariance matrices
    this->ui_mQ = {.05, .05, .0, .05, .05, .0, .0, .0, .0};
    this->ui_mR = 5;
    this->ui_mP = {.1, .1, .1, .1, 10000, 10, .1, 10, 100};
}

FilterView::~FilterView(){
}

void FilterView::setFilterSize(int channelCount){
    // argument put count to sensor container
    for(int i = 0; i < channelCount; ++i){
        m_channelsData << QSharedPointer<Sensor>::create(QString("voltage_ch%1").arg(i));
        m_channelsXhatS << QSharedPointer<Sensor>::create(QString("XhatS_ch%1").arg(i));
        m_channelsXhatT << QSharedPointer<Sensor>::create(QString("XhatT_ch%1").arg(i));
    }
}

void FilterView::appendDataToView(int viewN, const QVector<qreal> &time, const QVector<double> &data){
    m_channelsData[viewN]->setData(time, data);
    emit updateView();
}
void FilterView::appendDataToXhatS(int viewN, const QVector<qreal> &time, const QVector<double> &data){
    m_channelsXhatS[viewN]->setData(time, data);
    emit updateXhatS();
}
void FilterView::appendDataToXhatT(int viewN, const QVector<qreal> &time, const QVector<double> &data){
    m_channelsXhatT[viewN]->setData(time, data);
    emit updateXhatT();
}

QSharedPointer<Sensor> FilterView::getChannelSensor(int channel, QString a){
    if(a == "view"){
        return m_channelsData[channel];
    }
    else if( a == "xhats"){
        return m_channelsXhatS[channel];
    } 
    else if(a == "xhatt") return m_channelsXhatT[channel];
    return m_channelsData[channel];
}

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
    return FilterMatrix({ui_mA, ui_mC, ui_mQ, ui_mR, ui_mP}); 
}