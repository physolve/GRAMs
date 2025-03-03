#pragma once

#include <QtQuick>
#include "DataCollection.h"

class QCustomPlot;
class QCPAbstractPlottable;

class CustomPlotItem : public QQuickPaintedItem {
  Q_OBJECT

public:
  CustomPlotItem(QQuickItem *parent = 0);
  virtual ~CustomPlotItem();

  void paint(QPainter *painter);

  Q_INVOKABLE CustomPlotItem* getCustomPlot();

  // Q_INVOKABLE void updatePlot();
  Q_INVOKABLE void resetPos();

  void initCustomPlot();
  void backgroundCustomPlot();
  void setupPlot(QCustomPlot* customPlot);

  void setDataPointers(DataCollection **ptr , int ptrCnt);
  void placeGraph();
protected:
  void routeMouseEvents(QMouseEvent *event);
  void routeWheelEvents(QWheelEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void wheelEvent(QWheelEvent *event);

public slots:
  void dataUpdated();
  void dataSetUpdated();

private:
  QCustomPlot *m_CustomPlot;
  DataCollection* m_time;
  QList<DataCollection*> m_sensors;
  bool rescalingON;
  double lastPointKey;
  
private slots:
  void graphClicked(QCPAbstractPlottable *plottable);
  void onCustomReplot();
  void updateCustomPlotSize();
};
