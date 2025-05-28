#include "BasePlot.h"
#include <stdexcept>
#include "../lib/qcustomplot.h"

BasePlot::BasePlot(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_CustomPlot(new QCustomPlot()),
    rescalingON(true), lastPointKey(0), rangeLow(0)
{
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);  
    connect(this, &QQuickPaintedItem::widthChanged, this, &BasePlot::onChartViewSizeChanged);
    connect(this, &QQuickPaintedItem::heightChanged, this, &BasePlot::onChartViewSizeChanged);   
    connect(m_CustomPlot, &QCustomPlot::afterReplot, this, &BasePlot::onChartViewReplot, Qt::UniqueConnection); // custom replot
    // try {
    //     QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    //     timeTicker->setTimeFormat("%h:%m:%s");
    //     m_CustomPlot->xAxis->setTicker(timeTicker);
    //     m_CustomPlot->xAxis->setLabel("Время, с");
    //     m_CustomPlot->xAxis->setLabelColor(Qt::white); //?
    //     connect(m_CustomPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->xAxis2, SLOT(setRange(QCPRange)));
    //     connect(m_CustomPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->yAxis2, SLOT(setRange(QCPRange)));
    // }
    // catch(const std::exception &e) {
    //     qCritical() << e.what();
    // }

    // setBaseColor();
    // rescaleAxes(true);
    update();
}

BasePlot::~BasePlot()
{
    m_CustomPlot = nullptr;
}

void BasePlot::setBaseColor()
{
    m_CustomPlot->setNoAntialiasingOnDrag(true);
    // set some pens, brushes and backgrounds:
    m_CustomPlot->xAxis->setBasePen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis->setBasePen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setTickPen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis->setTickPen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    m_CustomPlot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    m_CustomPlot->xAxis->setTickLabelColor(Qt::white);
    m_CustomPlot->yAxis->setTickLabelColor(Qt::white);
    m_CustomPlot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    m_CustomPlot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    m_CustomPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    m_CustomPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    m_CustomPlot->xAxis->grid()->setSubGridVisible(true);
    m_CustomPlot->yAxis->grid()->setSubGridVisible(true);
    m_CustomPlot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    m_CustomPlot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    m_CustomPlot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    m_CustomPlot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
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

void BasePlot::setDataPointers(DataCollection* x, const QVector<DataCollection*>& ptr){
    m_time = x;
    m_sensors = ptr;
}

void BasePlot::initBasePlot(){
    try {
        QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
        timeTicker->setTimeFormat("%h:%m:%s");
        m_CustomPlot->xAxis->setTicker(timeTicker);
        m_CustomPlot->xAxis->setLabel("Время, с");
        m_CustomPlot->xAxis->setLabelColor(Qt::white); //?
        connect(m_CustomPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->xAxis2, SLOT(setRange(QCPRange)));
        connect(m_CustomPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->yAxis2, SLOT(setRange(QCPRange)));
    }
    catch(const std::exception &e) {
        qCritical() << e.what();
    }

    // split to two defined pressure and temperature 
    m_CustomPlot->yAxis->setLabel("Давление, бар"); // changeble label
    m_CustomPlot->yAxis->setLabelColor(Qt::white);
    m_CustomPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_CustomPlot->setAttribute(Qt::WA_OpaquePaintEvent, true);

    // to function
    auto g = m_CustomPlot->addGraph();
    auto pen = QPen(QColor("#cb8175"), 1.5);
    m_CustomPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, pen, QBrush(Qt::white), 9));
    m_CustomPlot->graph()->setPen(QPen(QColor(120, 120, 120), 2));
    m_CustomPlot->graph()->setAdaptiveSampling(true);
    
    m_CustomPlot->graph(0)->addData({0, 10}, {1, 10});
}


void BasePlot::setTwoAxisPlotColor(){
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

void BasePlot::initTwoAxisPlot(){
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


void BasePlot::dataUpdated(){
    const auto &timePoint = m_time->getCurValue(); 
    for(unsigned short i = 0; auto* ptr : m_sensors){
        const double& curValue = ptr->getCurValue();
        m_CustomPlot->graph(i)->addData(timePoint, curValue); //ptr->getCurValue()
        m_tags[i]->updatePosition(curValue);
        m_tags[i]->setText(QString::number(curValue,'g',2));
        ++i;
    }
    if(lastPointKey < m_time->getCurValue())
        lastPointKey = m_time->getCurValue();
    
    if(rescalingON){
        m_CustomPlot->xAxis->setRange(lastPointKey, 10, Qt::AlignRight); // means there a 10 sec
        m_CustomPlot->yAxis->rescale(true);
        m_CustomPlot->yAxis->setRangeUpper(m_CustomPlot->yAxis->range().upper*1.1);
    }
    // case of two axis tags custom replot

    // if m_CustomPlot points more than x delete first y points
    m_CustomPlot->replot();

}

Q_INVOKABLE void BasePlot::rescaleAxes(bool onlyVisiblePlottables)
{
    m_CustomPlot->rescaleAxes(onlyVisiblePlottables);
}

void BasePlot::paint(QPainter *painter)
{
    if (!painter->isActive())
        return;
    QPixmap picture( boundingRect().size().toSize() );
    QCPPainter qcpPainter( &picture );
    m_CustomPlot->toPainter(&qcpPainter);
    painter->drawPixmap(QPoint(), picture);
}

void BasePlot::onChartViewSizeChanged()
{
    m_CustomPlot->setGeometry(0, 0, (int)width(), (int)height());
    m_CustomPlot->setViewport(QRect(0, 0, (int)width(), (int)height()));
    m_CustomPlot->axisRect()->setOuterRect(QRect(0, 0, (int)width(), (int)height()));
    m_CustomPlot->axisRect()->setMinimumMargins (QMargins(0, 0, 0, 0));
    m_CustomPlot->axisRect()->setMargins(QMargins(0, 0, 0, 0));
}

void BasePlot::routeMouseEvents(QMouseEvent *event)
{
    QMouseEvent* newEvent = new QMouseEvent(event->type(), event->position(), event->globalPosition(), 
    event->button(), event->buttons(), event->modifiers());
    QCoreApplication::postEvent(m_CustomPlot, newEvent);
}

void BasePlot::routeWheelEvents(QWheelEvent *event)
{
    QWheelEvent* newEvent = new QWheelEvent(event->position(), event->globalPosition(),
                                            event->pixelDelta(), event->angleDelta(), 
                                            event->buttons(), event->modifiers(), 
                                            event->phase(), event->inverted());
    QCoreApplication::postEvent(m_CustomPlot, newEvent);
}
