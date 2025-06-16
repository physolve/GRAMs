#pragma once

#include <QtQuick>
#include "../DataCollection.h"

class QCustomPlot;
class QCPGraph;
// class QCPAbstractPlottable;

// might be types: a vaulue, pressure, temperature, two-axis, weight%
class BasePlot : public QQuickPaintedItem{
    Q_OBJECT

public:
    explicit BasePlot(QQuickItem *parent = nullptr);
    virtual ~BasePlot();
    QString m_chartName;
    
    void setDataPointers(DataCollection* x, DataCollection* ptr);
    void setDataPointers(DataCollection* x, const QVector<DataCollection*>& ptr);
    
    virtual void setPlotColor();
    virtual void initPlot();
    void placeLegend();
    void setLogValueAxis();
    // QVariantMap graphs() const;

    Q_INVOKABLE void rescaleAxes(bool onlyVisiblePlottables=false);
    void paint(QPainter *painter) override;
    virtual void dataUpdated();
public slots:
    virtual void customBeforeReplot();
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

    QCustomPlot *m_CustomPlot;
    DataCollection* m_time;
    QVector<DataCollection*> m_sensors;

    bool rescalingON;
    double lastPointKey;
    QVector<QCPGraph*> m_leftGraphs;
    QVector<QCPGraph*> m_rightGraphs;
};
