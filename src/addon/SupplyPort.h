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
    void setInitialParametersSupply(int portId, double portPressure, double temperature, double startPressure, double actionTime);
    void initResultFile();
    double supply(double pressure, double actionTime, double v_S);
    double calcRealRate(double dP, double dt, double v_S);
    
    void modelSupply(double pressureLimit, double v_S); // Storage strategy for model
    
    double getLastRate();
    int getTodayRuns();
    void saveResultsToFile();
private:
    int m_todayRuns;
    int todayRunCount();
    double calcRate(double pressure);
    double getPressureIncome(double rate, double dt, double v_S);
    void saveModelFile(const QList<double>& timePoints, const QList<double>& pressurePoints, const QList<double>& ratePoints, const QList<double>& volumePoints);
    // double calcualteModelPass(double time_differ);
    int m_portId;
    double Cv;
    double m_T;

    double m_portPressure;

    double last_pressure_pass;
    double last_time_pass;

    double last_rate;
    // to save
    // QElapsedTimer progressTime;
    // Quartile pressure
    QList<double> m_timePoints;
    QList<double> m_pressurePoints;
    QList<double> m_ratePoints;
    QList<double> m_volumePoints;
};