#pragma once
#include "../Constants.h"
#include <QObject>
#include <QFile>
#include <QElapsedTimer>
// let's make it as experiment object with file saves

class SupplyPort : public QObject
{
    Q_OBJECT // ?
public:
    SupplyPort(QObject *parent = nullptr);
    ~SupplyPort();
    void setInitialParametersSupply(int portId, double portPressure, double temperature, double actionTime);
    void initResultFile(bool debug = false);
    double getPressureIncome(double pressure, double v_S, double actionTime);
    QString getResultFileSuffix() const;
    void saveResultsToFile();
private:
    int todayRuns;
    int todayRunCount();
    double calcRate(double pressure);
    // double calcualteModelPass(double time_differ);
    int m_portId;
    double Cv;
    double m_T;

    double m_currentRate;
    double m_portPressure;
    double last_time_pass;

    QFile supplyResultFile;
    QString resultFileSuffix;
    // to save
    // QElapsedTimer progressTime;
    // Quartile pressure
    QList<double> m_timePoints;
    QList<double> m_pressurePoints;
    QList<double> m_ratePoints;
};