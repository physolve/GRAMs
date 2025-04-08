#pragma once

#include <QtQuick>
#include <QFile>
#include "../DataCollection.h"

class QCustomPlot;
class QCPAbstractPlottable;

class LightPlotItem : public QQuickPaintedItem {
  Q_OBJECT

public:
  LightPlotItem(QQuickItem *parent = 0);
  virtual ~LightPlotItem();
  Q_INVOKABLE LightPlotItem* getLightPlot();
  Q_INVOKABLE void resetPos();

  void paint(QPainter *painter);
  void initCustomPlot();
  void setupPlot(QCustomPlot* customPlot);
  
  void setDataPointers(const QVector<FilterData*>& ptr);
  void placeGraph();

  void initPlotData();
  void initPlotData(const QString& dirName, const QString& suffix);
  void savePlotData();
  void clearPlotData();

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

private:
  QCustomPlot *m_CustomPlot;
  FilterData pseudo_time;
  QVector<FilterData*> m_sensors;
  bool rescalingON;
  double lastPointKey;

  QFile fastFile;
  int fastResultCount;
private slots:
  void graphClicked(QCPAbstractPlottable *plottable);
  void onCustomReplot();
  void updateCustomPlotSize();
};
