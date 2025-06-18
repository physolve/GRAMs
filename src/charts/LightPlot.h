#pragma once

#include "BasePlot.h"
#include "../DataCollection.h"
#include <QFile>

class LightPlot : public BasePlot{
    Q_OBJECT
public:
    LightPlot(QQuickItem *parent = nullptr);
    ~LightPlot();
    void setDataPointers(DataCollection* ptr);
    void setDataPointers(const QVector<DataCollection*>& ptr);
    void dataUpdated() override;
    void dataWithTime(unsigned int nowTime);
    void initPlotData();
    void initPlotData(const QString& dirName, const QString& suffix);
    void savePlotData();
    void clearPlotData();
private:
    QFile fastFile;
    int fastResultCount;

};