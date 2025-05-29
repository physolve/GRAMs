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
    update();
}

BasePlot::~BasePlot()
{
    m_CustomPlot = nullptr;
}


void BasePlot::setDataPointers(DataCollection* x, DataCollection* ptr){
    m_time = x;
    m_sensors << ptr;
}

void BasePlot::setDataPointers(DataCollection* x, const QVector<DataCollection*>& ptr){
    m_time = x;
    m_sensors = ptr;
}

void BasePlot::setPlotColor(){
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

void BasePlot::initPlot(){
    if(m_sensors.isEmpty()){
        qDebug() << "BasePlot empty sensors";
        return;
    }
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

    // create graphs:
    QStringList pressureColors = {"#a8c8a6", "#6d8d8a", "#655057"}; // add more
    QStringList temperatureColors = {"#cb8175", "#e2a97e", "#f0cf8e"}; // add more
    DataType leftAxis = m_sensors[0]->m_type;
    QCPAxis::AxisType axisSide = QCPAxis::atLeft;
    for(auto sensor : m_sensors){
        QColor lineColor;
        if(sensor->m_type == DataType::Pressure){
            lineColor = QColor(pressureColors.takeFirst());
        }
        else if(sensor->m_type == DataType::Temperature){
            lineColor = QColor(temperatureColors.takeFirst());
        }
        else{
            qDebug() << "unknown DataType at " << sensor->m_name;
        }
        axisSide = sensor->m_type == leftAxis ? QCPAxis::atLeft : QCPAxis::atRight; // 2, 3
        auto curGraph = m_CustomPlot->addGraph(m_CustomPlot->xAxis, m_CustomPlot->axisRect()->axis(axisSide, 0));
        auto pen = QPen(lineColor, 1.5);
        m_CustomPlot->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, pen, QBrush(Qt::white), 9));
        m_CustomPlot->graph()->setPen(QPen(QColor(120, 120, 120), 2));
        m_CustomPlot->graph()->setAdaptiveSampling(true);
        m_CustomPlot->graph()->setName(sensor->m_name);
    }
}

void BasePlot::placeLegend(){
    m_CustomPlot->legend->setVisible(true);
    auto font = m_CustomPlot->legend->font();
    font.setPointSize(9);
    m_CustomPlot->legend->setFont(font);
    m_CustomPlot->legend->setTextColor(QColor("white"));
    m_CustomPlot->legend->setBorderPen(QPen(QColor("transparent")));
    
    m_CustomPlot->legend->setBrush(QBrush(QColor(0,0,0,63)));
    // by default, the legend is in the inset layout of the main axis rect. So this is how we access it to change legend placement:
    m_CustomPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
    
    // QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    // m_CustomPlot->plotLayout()->addElement(1, 0, subLayout);
    // subLayout->setMargins(QMargins(5, 0, 5, 5));
    // subLayout->addElement(0, 0, m_CustomPlot->legend);
    // // change the fill order of the legend, so it's filled left to right in columns:
    // m_CustomPlot->legend->setFillOrder(QCPLegend::foColumnsFirst);
    // // set legend's row stretch factor very small so it ends up with minimum height:
    // m_CustomPlot->plotLayout()->setRowStretchFactor(1, 0.001);
}

void BasePlot::setLogValueAxis(){
    m_CustomPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    m_CustomPlot->yAxis2->setScaleType(QCPAxis::stLogarithmic);
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    m_CustomPlot->yAxis->setTicker(logTicker);
    m_CustomPlot->yAxis2->setTicker(logTicker);
    m_CustomPlot->yAxis->setNumberFormat("eb"); // e = exponential, b = beautiful decimal powers
    m_CustomPlot->yAxis->setNumberPrecision(0); // makes sure "1*10^4" is displayed only as "10^4"
    m_CustomPlot->xAxis->setRange(0, 10.0);
    m_CustomPlot->yAxis->setRange(1e-6, 1);
}

void BasePlot::dataUpdated(){
    const auto &timePoint = m_time->getCurValue(); 
    for(unsigned short i = 0; auto* ptr : m_sensors){
        m_CustomPlot->graph(i)->addData(timePoint, ptr->getCurValue());
        ++i;
    }
    if(lastPointKey < m_time->getCurValue())
        lastPointKey = m_time->getCurValue();
    
    if(rescalingON){
        m_CustomPlot->xAxis->setRange(lastPointKey, 10, Qt::AlignRight); // means there a 10 sec
        m_CustomPlot->yAxis->rescale(true);
        m_CustomPlot->yAxis->setRangeUpper(m_CustomPlot->yAxis->range().upper*1.1);
    }
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

// QVariantMap BasePlot::graphs(){
//     QVariantMap map;
//     for(auto it = m_graphs.begin(); it != m_graphs.end(); ++it) {
//         map.insert(it.key(), QVariant::fromValue(it.value()));
//     }
// }

