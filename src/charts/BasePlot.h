#pragma once

#include <QtQuick>
#include "AxisTag.h"
#include "../DataCollection.h"

class QCustomPlot;
// class QCPAbstractPlottable;

// might be types: a vaulue, pressure, temperature, two-axis, weight%
class BasePlot : public QQuickPaintedItem{
    Q_OBJECT

public:
    BasePlot(QQuickItem *parent = nullptr);
    ~BasePlot();

    
    void setDataPointers(DataCollection* x, const QVector<DataCollection*>& ptr);

    void setBaseColor();
    void initBasePlot();

    void setTwoAxisPlotColor();
    void initTwoAxisPlot();
    
    QVariantMap graphs() const;
    Q_INVOKABLE void rescaleAxes(bool onlyVisiblePlottables=false);
    void paint(QPainter *painter);
    void dataUpdated();
protected:
    virtual void onChartViewReplot() { update(); }
    virtual void onChartViewSizeChanged();

    virtual void hoverMoveEvent(QHoverEvent *event) override { Q_UNUSED(event) }
    virtual void mousePressEvent(QMouseEvent *event) override { routeMouseEvents(event); }
    virtual void mouseReleaseEvent(QMouseEvent *event) override { routeMouseEvents(event); }
    virtual void mouseMoveEvent(QMouseEvent *event) override { routeMouseEvents(event); }
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override { routeMouseEvents(event); }
    virtual void wheelEvent(QWheelEvent *event) override { routeWheelEvents(event); }
    void routeMouseEvents(QMouseEvent *event);
    void routeWheelEvents(QWheelEvent *event);

private:
    QCustomPlot *m_CustomPlot;
    DataCollection* m_time;
    QVector<DataCollection*> m_sensors;
    QVector<AxisTag*> m_tags;

    bool rescalingON;
    double lastPointKey;
    double rangeLow;
};