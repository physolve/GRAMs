#pragma once

#include <QtCore/QObject>
#include <QtCharts/QAbstractSeries>
#include "Controller.h"

class DataAcquisition : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap connectedDevices MEMBER m_connectedDevices NOTIFY profileNamesChanged)
    Q_PROPERTY(QVariantMap profileJson MEMBER m_profileJson CONSTANT)
    Q_PROPERTY(QStringList profileNames MEMBER m_profileNames CONSTANT)
public:
    explicit DataAcquisition(QObject *parent = 0);
    QVariantMap profileJson () const;
Q_SIGNALS:

public slots:
    void generateData(int type, int rowCount, int colCount);
    void update(QAbstractSeries *series);
    //void choosenGRAMInitialization();
signals:
    void profileNamesChanged();
    void profileJsonChanged();
private:
    QList<QList<QPointF>> m_data;
    int m_index;
    QVariantMap  m_connectedDevices;

    QVariantMap  m_profileJson;

    QStringList m_profileNames;

    bool advantechDeviceCheck(QVariantMap& advantechDeviceMap) const; 
};