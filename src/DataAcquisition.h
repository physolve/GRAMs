#pragma once

#include <QTimer>
#include "controllers/AdvantechCtrl.h"
#include "FilterView.h"

enum ControllerConnection{
    Offline,
    Online,
    Pending
};

class DataAcquisition : public QObject
{
    Q_OBJECT
public:
    explicit DataAcquisition(QObject *parent = 0);
    //QVariantMap profileJson ();
    void processEvents(); // wierdly written

    void processEvents(QString purpose);

    QMap<QString,QVector<double>> getMeasures();
    QVector<bool> getValves();
    void setValves(const QVector<bool> &states);
    bool getGRAMsIntegrity();
    Q_INVOKABLE void advantechDeviceSetting(const QString &description, const QString &type, const QVariantMap& deviceSettings);
    Q_INVOKABLE void turnOnFilterTimer();
    //Q_INVOKABLE void testRead();
    FilterView* getFilterView();
private slots:
    void filterEvent();

private:
    QMap<QString, QSharedPointer<AdvantechCtrl>> m_controllerList; // for read
    QMap<QString, ControllerConnection> GRAMsIntegrity;
    QTimer* fastFilter;
    FilterView filterView;
};