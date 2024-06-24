#pragma once

#include <QtQuick>
#include "Sensor.h"

class QCustomPlot;
class QCPAbstractPlottable;

class CustomPlotItem : public QQuickPaintedItem {
  Q_OBJECT

public:
  CustomPlotItem(QQuickItem *parent = 0);
  virtual ~CustomPlotItem();

  void paint(QPainter *painter);


  Q_INVOKABLE void initCustomPlot(int index); // index as place in somewhere
  Q_INVOKABLE void placePointerGraph(const QString &name, QSharedPointer<Sensor> sensor_ptr); // additional paramters
  Q_INVOKABLE void setCustomLabel(const QString &label); // additional paramters
  Q_INVOKABLE void updatePlot();
  Q_INVOKABLE void resetPos();
protected:
  void routeMouseEvents(QMouseEvent *event);
  void routeWheelEvents(QWheelEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void wheelEvent(QWheelEvent *event);

private:
  QCustomPlot *m_CustomPlot;
  int m_index;

  bool rescalingON;
  QStringList m_plotNames;
  QList<QSharedPointer<Sensor>> m_sensors;

private slots:
  void graphClicked(QCPAbstractPlottable *plottable);
  void onCustomReplot();
  void updateCustomPlotSize();
};
