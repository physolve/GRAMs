#include "LightPlot.h"
#include "../lib/qcustomplot.h"
LightPlot::LightPlot(QQuickItem *parent): BasePlot(parent){

};

LightPlot::~LightPlot(){

};

void LightPlot::setDataPointers(DataCollection* ptr){
    m_sensors << ptr;
}

void LightPlot::setDataPointers(const QVector<DataCollection*>& ptr){
    m_sensors = ptr;
}

void LightPlot::dataUpdated(){
    // check length using getCumulativeData() maybe
    int timePoint = 0;
    // USE ITERATORS! 
    for(unsigned short i = 0; auto* ptr : m_sensors){

        auto filter_ptr = static_cast<FilterData*>(ptr);
        // FREQUENLY CHECK - REMOVE
        if(!filter_ptr->isCumulativeDataReady()){
            ++i;
            qDebug() << "Cumulative data not READY";
            continue;
        }
        const auto& data_value = filter_ptr->getCumulativeData();
        QList<double> time_value;
        for(int j = 0; j < data_value.count(); ++j){
            time_value << lastPointKey + j;
        }
        m_CustomPlot->graph(i)->addData(time_value, data_value);
        ++i;
        filter_ptr->clearCumulative();
        timePoint = data_value.count();
    }  
    if(lastPointKey < timePoint)
        lastPointKey = timePoint;
    if(rescalingON){
        m_CustomPlot->rescaleAxes();
        m_CustomPlot->yAxis->setRangeUpper(m_CustomPlot->yAxis->range().upper*1.1);
    }
    m_CustomPlot->replot(QCustomPlot::rpQueuedReplot);
}

void LightPlot::initPlotData(){
    fastFile.setFileName(QString("data/fastResult_%1.csv").arg(fastResultCount));
    if (!fastFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "File don't exist";
        return;
    }
    fastResultCount++;
}

void LightPlot::initPlotData(const QString& dirName, const QString& suffix){
    QDir dir("data");
    dir.cd(dirName);
    if (!dir.exists())
        dir.mkdir(dirName);
    const auto& baseFileName = QDate::currentDate().toString("yyyy-MM-dd")+"_fastResult"+suffix+".csv";
    fastFile.setFileName(dir.filePath(baseFileName));
    if (!fastFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "File doesn't exist";
        return;
    }
    fastResultCount++;
}

void LightPlot::savePlotData(){
    QTextStream out(&fastFile);
    out << "time"<< ',' << "value" << '\n';
    const auto& data = m_CustomPlot->graph(0)->data();
    // auto it = data->constBegin();
    QCPGraphDataContainer::const_iterator it;
    for (it = data->constBegin(); it != data->constEnd(); ++it) {
        out << (*it).key << ',' << (*it).value << '\n';
    }
    fastFile.close();
}

void LightPlot::clearPlotData(){
    for(auto i{0}; i<m_CustomPlot->graphCount(); ++i){
        m_CustomPlot->graph(i)->data()->clear();
    }
    // pseudo_time.clearPoints();
    for(auto* ptr : m_sensors){
        auto filter_ptr = static_cast<FilterData*>(ptr);
        filter_ptr->clearCumulative();
    }
}
