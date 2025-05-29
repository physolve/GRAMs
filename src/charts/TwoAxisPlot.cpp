#include "TwoAxisPlot.h"

TwoAxisPlot::TwoAxisPlot(QQuickItem *parent): BasePlot(parent){

};

TwoAxisPlot::~TwoAxisPlot(){
    qDeleteAll(m_tags);
    m_tags.clear();
}

void TwoAxisPlot::setPlotColor(){
    m_CustomPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));

    
    m_CustomPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis2->setBasePen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis2->setTickPen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis2->setSubTickPen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setTickLabelColor(Qt::white);
    m_CustomPlot->yAxis2->setTickLabelColor(Qt::white);
    m_CustomPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    m_CustomPlot->yAxis2->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    m_CustomPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    m_CustomPlot->yAxis2->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    m_CustomPlot->xAxis->grid()->setSubGridVisible(true);
    m_CustomPlot->yAxis2->grid()->setSubGridVisible(true);
    m_CustomPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    m_CustomPlot->yAxis2->grid()->setZeroLinePen(Qt::NoPen);
    m_CustomPlot->xAxis->setUpperEnding(QCPLineEnding::esBar);
    m_CustomPlot->yAxis2->setUpperEnding(QCPLineEnding::esBar);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));
    m_CustomPlot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    m_CustomPlot->axisRect()->setBackground(axisRectGradient);
}

void TwoAxisPlot::initPlot(){
    if(m_sensors.isEmpty()){
        return;
    }
    try {
        QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
        timeTicker->setTimeFormat("%h:%m:%s");
        m_CustomPlot->xAxis->setTicker(timeTicker);
        m_CustomPlot->xAxis->setLabel("Время, с");
        m_CustomPlot->xAxis->setLabelColor(Qt::white); //?
        // connect(m_CustomPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->xAxis2, SLOT(setRange(QCPRange)));
        connect(m_CustomPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->yAxis2, SLOT(setRange(QCPRange)));
    }
    catch(const std::exception &e) {
        qCritical() << e.what();
    }
    m_CustomPlot->yAxis->setTickLabels(false); //?

    connect(m_CustomPlot->yAxis2, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->yAxis, SLOT(setRange(QCPRange))); // left axis only mirrors inner right axis
    m_CustomPlot->yAxis2->setVisible(true);
    
    auto secondAxis = m_CustomPlot->axisRect()->addAxis(QCPAxis::atRight);
    m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 0)->setPadding(30); // add some padding to have space for tags
    m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 1)->setPadding(30); // add some padding to have space for tags
    m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 0)->setLabel("Давление, бар");
    m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 1)->setLabel("Температура, °C");
    
    m_CustomPlot->yAxis2->setLabelColor(Qt::white);
    secondAxis->setLabelColor(Qt::white);
    
    secondAxis->setBasePen(QPen(Qt::white, 1));
    secondAxis->setTickPen(QPen(Qt::white, 1));
    secondAxis->setSubTickPen(QPen(Qt::white, 1));
    secondAxis->setTickLabelColor(Qt::white);
    secondAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    secondAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    secondAxis->grid()->setSubGridVisible(true);
    secondAxis->grid()->setZeroLinePen(Qt::NoPen);
    secondAxis->setUpperEnding(QCPLineEnding::esBar);
    
    // create graphs:
    QStringList pressureColors = {"#a8c8a6", "#6d8d8a", "#655057"}; // add more
    QStringList temperatureColors = {"#cb8175", "#e2a97e", "#f0cf8e"}; // add more
    int axisIndex = 0;
    for(auto sensor : m_sensors){
        QColor lineColor;
        if(sensor->m_type == DataType::Pressure){
            lineColor = QColor(pressureColors.takeFirst());
            axisIndex = 0;
        }
        else if(sensor->m_type == DataType::Temperature){
            lineColor = QColor(temperatureColors.takeFirst());
            axisIndex = 1;
        }
        else{
            qDebug() << "unknown DataType at " << sensor->m_name;
        }
        auto curGraph = m_CustomPlot->addGraph(m_CustomPlot->xAxis, m_CustomPlot->axisRect()->axis(QCPAxis::atRight, axisIndex));
        curGraph->setPen(QPen(QColor(120, 120, 120), 2));
        const auto& pen = QPen(lineColor, 1.5);
        curGraph->setLineStyle(QCPGraph::LineStyle::lsLine);
        curGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, pen, QBrush(Qt::white), 5));
        curGraph->setAdaptiveSampling(true);
        auto mTag = new AxisTag(curGraph->valueAxis());
        mTag->setPen(pen);
        m_tags << mTag;
        curGraph->setName(sensor->m_name);
    }
}

void TwoAxisPlot::dataUpdated(){
    const auto &timePoint = m_time->getCurValue(); 
    for(unsigned short i = 0; auto* ptr : m_sensors){
        const double& curValue = ptr->getCurValue();
        m_CustomPlot->graph(i)->addData(timePoint, curValue); //ptr->getCurValue()
        m_tags[i]->updatePosition(curValue);
        m_tags[i]->setText(QString::number(curValue,'g',2));
        // m_CustomPlot->graph(i)->rescaleValueAxis(false, true);
        ++i;
    }
    auto yAxis2a = m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 0);
    yAxis2a->rescale(true);
    yAxis2a->setRangeUpper(yAxis2a->range().upper*1.1);

    auto yAxis2b = m_CustomPlot->axisRect()->axis(QCPAxis::atRight, 1);
    yAxis2b->rescale(true);
    yAxis2b->setRangeUpper(yAxis2b->range().upper*1.1);
    yAxis2b->setRangeLower(yAxis2b->range().lower*0.9);

    // m_CustomPlot->yAxis2->rescale(true);
    // m_CustomPlot->yAxis2->setRangeUpper(m_CustomPlot->yAxis2->range().upper*1.1);
    
    m_CustomPlot->xAxis->setRange(m_CustomPlot->xAxis->range().upper, 10, Qt::AlignRight); // 10 and larger by memory scaling?
    m_CustomPlot->xAxis->rescale();
    m_CustomPlot->xAxis->setRange(m_CustomPlot->xAxis->range().upper, 10, Qt::AlignRight);
    // if m_CustomPlot points more than x delete first y points
    m_CustomPlot->replot();
}