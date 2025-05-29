#pragma once

#include <QtQuick>
#include "BasePlot.h"
#include "AxisTag.h"
#include "../DataCollection.h"
class TwoAxisPlot : public BasePlot{
    Q_OBJECT
public:
    TwoAxisPlot(QQuickItem *parent = nullptr);
    ~TwoAxisPlot();
    void setPlotColor() override;
    void initPlot() override;
    void dataUpdated() override;
private:
    QVector<AxisTag*> m_tags;

};