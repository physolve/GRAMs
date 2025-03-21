#include "LightPlotItem.h"
#include "../lib/qcustomplot.h"
#include <QDebug>

LightPlotItem::LightPlotItem(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_CustomPlot(nullptr), rescalingON(true), lastPointKey(0), fastResultCount(0) {
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::AllButtons);

    connect(this, &QQuickPaintedItem::widthChanged, this,
            &LightPlotItem::updateCustomPlotSize);
    connect(this, &QQuickPaintedItem::heightChanged, this,
            &LightPlotItem::updateCustomPlotSize);
    qDebug() << "CustomPlotItem Created";
}

LightPlotItem::~LightPlotItem() {
    delete m_CustomPlot;
    m_CustomPlot = nullptr;
    qDebug() << "CustomPlotItem Destroyed";
}

LightPlotItem* LightPlotItem::getLightPlot()
{
    return this;
}

void LightPlotItem::initCustomPlot() {
    if(!m_CustomPlot){
        m_CustomPlot = new QCustomPlot();
        m_CustomPlot->setOpenGl(true); // it's not working without some fckn include
        updateCustomPlotSize();
        QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
        timeTicker->setTimeFormat("%h:%m:%s");
        m_CustomPlot->xAxis->setTicker(timeTicker);
        m_CustomPlot->axisRect()->setupFullAxesBox();
        m_CustomPlot->yAxis->setRange(-1.2, 1.2);
        // setup a timer that repeatedly calls MainWindow::realtimeDataSlot:
        setupPlot(m_CustomPlot); // time
    }
}

void LightPlotItem::setupPlot(QCustomPlot* customPlot){ // knows how many should be // realize only for one

    //make left and bottom axes transfer their ranges to right and top axes:
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange))); //?
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange))); //?
    customPlot->xAxis->setLabel("Время, с");
    customPlot->xAxis->setLabelColor(Qt::white);
    customPlot->yAxis->setLabel("Давление, бар"); // changeble label
    customPlot->yAxis->setLabelColor(Qt::white);
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    connect(customPlot, &QCustomPlot::afterReplot, this,
            &LightPlotItem::onCustomReplot);
    qDebug() << QString("QCustomPlot Initialized");
}

void LightPlotItem::setDataPointers(FilterData** ptr, int ptrCnt){
    // not good but it works
    // m_sensors.resize(ptrCnt - 1);
    for(auto i = 0; i < ptrCnt; ++i){
        m_sensors.append(ptr[i]);
    }
}

void LightPlotItem::placeGraph(){
    if(m_sensors.isEmpty()){
        return;
    }
    QStringList lineColors = {"#cb8175", "#e2a97e", "#f0cf8e", "#f6edcd", "#a8c8a6", "#6d8d8a", "#655057" };
    for(unsigned short i = 0; auto sensor : m_sensors){
        m_CustomPlot->addGraph();
        auto pen = QPen(QColor(lineColors[i]), 1.5);
        m_CustomPlot->graph()->setPen(QPen(QColor(120, 120, 120), 2));
        m_CustomPlot->graph()->setAdaptiveSampling(true); //?
        m_CustomPlot->graph()->setName(sensor->m_name);
        ++i;
    }
    m_CustomPlot->legend->setVisible(true);
    QFont legendFont = m_CustomPlot->legend->font();  // start out with MainWindow's font..
    legendFont.setPointSize(9); // and make a bit smaller for legend
    m_CustomPlot->legend->setFont(legendFont);
    m_CustomPlot->legend->setBrush(QBrush(QColor(255,255,255,230)));
}

void LightPlotItem::paint(QPainter *painter) {
    if (m_CustomPlot) {
        QPixmap picture(boundingRect().size().toSize());
        QCPPainter qcpPainter(&picture);

        m_CustomPlot->toPainter(&qcpPainter);

        painter->drawPixmap(QPoint(), picture);
    }
}

void LightPlotItem::mousePressEvent(QMouseEvent *event) {
    //qDebug() << Q_FUNC_INFO;
    routeMouseEvents(event);
}

void LightPlotItem::mouseReleaseEvent(QMouseEvent *event) {
    //qDebug() << Q_FUNC_INFO;
    routeMouseEvents(event);
    //QQuickPaintedItem::mouseReleaseEvent(event);
}

void LightPlotItem::mouseMoveEvent(QMouseEvent *event) {
    rescalingON = false;
    routeMouseEvents(event);
}

void LightPlotItem::mouseDoubleClickEvent(QMouseEvent *event) {
    qDebug() << Q_FUNC_INFO;
    rescalingON = true;
    routeMouseEvents(event);
}

void LightPlotItem::wheelEvent(QWheelEvent *event) { 
    rescalingON = false;
    routeWheelEvents(event); 
}

void LightPlotItem::dataUpdated(){
    const auto& time = pseudo_time.getCurValue();
    const auto& bufferCount = m_sensors[0]->getCumulativeCount();
    QVector<double> indexTime;
    for(int i = 1; i <= 512*bufferCount; i++){
        indexTime << time + i; 
    }
    pseudo_time.setData(indexTime);
    const auto &timePoint = pseudo_time.getCurValue(); 
    for(unsigned short i = 0; auto* ptr : m_sensors){
        m_CustomPlot->graph(i)->addData(pseudo_time.getValue(), ptr->getCumulativeData());
        ++i;
        ptr->clearCumulative();
    }  
    if(lastPointKey < timePoint)
        lastPointKey = timePoint;
    if(rescalingON){
        m_CustomPlot->rescaleAxes();
        m_CustomPlot->yAxis->setRangeUpper(m_CustomPlot->yAxis->range().upper*1.1);
    }
    m_CustomPlot->replot(QCustomPlot::rpQueuedReplot);
}

void LightPlotItem::graphClicked(QCPAbstractPlottable *plottable) {
    qDebug() << Q_FUNC_INFO
            << QString("Clicked on graph '%1 ").arg(plottable->name());
}

void LightPlotItem::resetPos(){
    rescalingON = true;
}

void LightPlotItem::routeMouseEvents(QMouseEvent *event) {
    if (m_CustomPlot) {
        QMouseEvent *newEvent =
            new QMouseEvent(event->type(), event->localPos(), event->button(),
                            event->buttons(), event->modifiers());
        QCoreApplication::postEvent(m_CustomPlot, newEvent);
    }
}

void LightPlotItem::routeWheelEvents(QWheelEvent *event) {
    if (m_CustomPlot) {
        QWheelEvent *newEvent = new QWheelEvent(
            event->position(), event->globalPosition(), event->pixelDelta(),
            event->angleDelta(), event->buttons(), event->modifiers(),
            event->phase(), event->inverted());
        QCoreApplication::postEvent(m_CustomPlot, newEvent);
        m_CustomPlot->yAxis->rescale(); //?
        m_CustomPlot->yAxis->setRangeUpper(m_CustomPlot->yAxis->range().upper*1.1);
    }
}

void LightPlotItem::updateCustomPlotSize() {
    if (m_CustomPlot) {
        m_CustomPlot->setGeometry(0, 0, (int)width(), (int)height());
        m_CustomPlot->setViewport(QRect(0, 0, (int)width(), (int)height()));
    }
}

void LightPlotItem::onCustomReplot() {
    update();
}

void LightPlotItem::initPlotData(){
    fastFile.setFileName(QString("data/fastResult_%1.csv").arg(fastResultCount));
    if (!fastFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug() << "File don't exist";
        return;
    }
    fastResultCount++;
}

void LightPlotItem::savePlotData(){
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

void LightPlotItem::clearPlotData(){
    for(auto i{0}; i<m_CustomPlot->graphCount(); ++i){
        m_CustomPlot->graph(i)->data()->clear();
    }
    pseudo_time.clearPoints();
}
