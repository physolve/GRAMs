#include "CustomPlotItem.h"
#include "../lib/qcustomplot.h"
#include <QDebug>

CustomPlotItem::CustomPlotItem(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_CustomPlot(nullptr), m_timerId(0), testTimer(0) {
  setFlag(QQuickItem::ItemHasContents, true);
  setAcceptedMouseButtons(Qt::AllButtons);

  connect(this, &QQuickPaintedItem::widthChanged, this,
          &CustomPlotItem::updateCustomPlotSize);
  connect(this, &QQuickPaintedItem::heightChanged, this,
          &CustomPlotItem::updateCustomPlotSize);
  qDebug() << "CustomPlotItem Created";
}

CustomPlotItem::~CustomPlotItem() {
  delete m_CustomPlot;
  m_CustomPlot = nullptr;

  if (m_timerId != 0) {
    killTimer(m_timerId);
  }

  qDebug() << "CustomPlotItem Destroyed";
  
}

void CustomPlotItem::initCustomPlot(int index) {
  if(!m_CustomPlot){
    m_CustomPlot = new QCustomPlot();

    connect( m_CustomPlot, &QCustomPlot::destroyed, this, [=](){ qDebug() << QString(" QCustomPlot (%1) pointer is destroyed ").arg(index); });

    updateCustomPlotSize();
    m_CustomPlot->addGraph();
    m_CustomPlot->graph(0)->setPen(QPen(Qt::red));
    //m_CustomPlot->addGraph();
    //m_CustomPlot->graph(1)->setPen(QPen(Qt::black));


    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    m_CustomPlot->xAxis->setTicker(timeTicker);
    m_CustomPlot->axisRect()->setupFullAxesBox(); //?
    m_CustomPlot->yAxis->setRange(-1.2, 1.2);

    //make left and bottom axes transfer their ranges to right and top axes:
    connect(m_CustomPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->xAxis2, SLOT(setRange(QCPRange))); //?
    connect(m_CustomPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_CustomPlot->yAxis2, SLOT(setRange(QCPRange))); //?


    m_CustomPlot->xAxis->setLabel("t");
    m_CustomPlot->yAxis->setLabel("S");
    m_CustomPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    //startTimer(500);

    connect(m_CustomPlot, &QCustomPlot::afterReplot, this,
            &CustomPlotItem::onCustomReplot);

    qDebug() << QString("QCustomplot (%1) Initialized").arg(index);
  }
  m_CustomPlot->replot();
}

void CustomPlotItem::paint(QPainter *painter) {
  if (m_CustomPlot) {
    QPixmap picture(boundingRect().size().toSize());
    QCPPainter qcpPainter(&picture);

    m_CustomPlot->toPainter(&qcpPainter);

    painter->drawPixmap(QPoint(), picture);
  }
}

void CustomPlotItem::mousePressEvent(QMouseEvent *event) {
  qDebug() << Q_FUNC_INFO;
  routeMouseEvents(event);
}

void CustomPlotItem::mouseReleaseEvent(QMouseEvent *event) {
  qDebug() << Q_FUNC_INFO;
  routeMouseEvents(event);
}

void CustomPlotItem::mouseMoveEvent(QMouseEvent *event) {
  routeMouseEvents(event);
}

void CustomPlotItem::mouseDoubleClickEvent(QMouseEvent *event) {
  qDebug() << Q_FUNC_INFO;
  routeMouseEvents(event);
}

void CustomPlotItem::wheelEvent(QWheelEvent *event) { routeWheelEvents(event); }

void CustomPlotItem::timerEvent(QTimerEvent * /*event*/) { // delete 
  static double t, U, V;
  U = ((double)rand() / RAND_MAX) * 5;
  m_CustomPlot->graph(0)->addData(t, U);
  V = ((double)rand() / RAND_MAX) * 5;
  m_CustomPlot->graph(1)->addData(t, V);
  //qDebug() << Q_FUNC_INFO << QString("Adding dot t = %1, S = %2").arg(t).arg(U);
  t++;
  m_CustomPlot->replot();
}

void CustomPlotItem::backendData(QList<double> x, QList<double> y){
  static double lastPointKey = 0;
  m_CustomPlot->graph(0)->setData(x, y);
  lastPointKey = x.last();
  m_CustomPlot->xAxis->setRange(lastPointKey, 8, Qt::AlignRight); // means there a 8 sec
  m_CustomPlot->yAxis->rescale();
  m_CustomPlot->replot();
}

void CustomPlotItem::graphClicked(QCPAbstractPlottable *plottable) {
  qDebug() << Q_FUNC_INFO
           << QString("Clicked on graph '%1 ").arg(plottable->name());
}

void CustomPlotItem::routeMouseEvents(QMouseEvent *event) {
  if (m_CustomPlot) {
    QMouseEvent *newEvent =
        new QMouseEvent(event->type(), event->localPos(), event->button(),
                        event->buttons(), event->modifiers());
    QCoreApplication::postEvent(m_CustomPlot, newEvent);
  }
}

void CustomPlotItem::routeWheelEvents(QWheelEvent *event) {
  if (m_CustomPlot) {
    QWheelEvent *newEvent = new QWheelEvent(
        event->position(), event->globalPosition(), event->pixelDelta(),
        event->angleDelta(), event->buttons(), event->modifiers(),
        event->phase(), event->inverted());
    QCoreApplication::postEvent(m_CustomPlot, newEvent);
  }
}

void CustomPlotItem::updateCustomPlotSize() {
  if (m_CustomPlot) {
    m_CustomPlot->setGeometry(0, 0, (int)width(), (int)height());
    m_CustomPlot->setViewport(QRect(0, 0, (int)width(), (int)height()));
  }
}

void CustomPlotItem::onCustomReplot() {
  update();
}