#pragma once
#include "../Constants.h"
#include <QObject>
#include <QFile>
#include <QElapsedTimer>
// let's make it as experiment object with file saves
class GasLeakage : public QObject
{
    Q_OBJECT // ?
public:
    GasLeakage(QObject *parent = nullptr);
    ~GasLeakage();
    void setInitialParametersLeakage(int portId, double turn, double sPressure, double rPressure);
    void initResultFile(bool debug = false);
    void startCalc(double sPressure, double rPressure, double initial_r_flow);
    void endCalc();
    void setLeakageOpen(bool state);
    void addMeasure(double sPressure, double rPressure);
    bool addModelMeasure(double model_sPressure, double model_rPressure, double time_model);
    void saveResultsToFile();
    double getLastFlowPass() const;
private:
    int m_portId;
    double m_turn;
    double m_storagePressure;
    double m_reactionPressure;
    double last_time_pass;
    bool m_leakageOpen;
    // double m_currentRate; 
    int todayRuns;
    int todayRunCount();
    double getFlowCoefficient(double turn);
    double calculateRate(double flow_factor, double sPressure, double rPressure) const;
    QFile leakageResultFile;
    QElapsedTimer progressTime;
    QList<double> m_timePoints;
    QList<double> m_sPPoints;
    QList<double> m_rPPoints;
    QList<double> m_ratePoints;
    QList<double> m_modelPassPoints;
};