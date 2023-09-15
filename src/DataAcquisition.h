#pragma once

#include <QtCore/QObject>
#include <QtCharts/QAbstractSeries>


class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
Q_SIGNALS:

public slots:
    void generateData(int type, int rowCount, int colCount);
    void update(QAbstractSeries *series);

private:
    QList<QList<QPointF>> m_data;
    int m_index;
};