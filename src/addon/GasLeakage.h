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
    // GasLeakage(const GasLeakage &) = default;
    ~GasLeakage();
    void setInitialParametersLeakage(int portId, double turn, double sPressure, double rPressure);
    void setVolumeNames(const QStringList& volumeNames);
    void initResultFile(bool debug = false);
    void startCalc(double sPressure, double rPressure, double rTempAbs, double initial_r_flow);
    void endCalc();
    void setLeakageOpen(bool state);
    void addMeasure(double sPressure, double rPressure, double rTempAbs);
    bool addModelMeasure(double model_sPressure, double model_rPressure, double rTempAbs, double time_model);
    void saveResultsToFile();
    double getLastFlowPass() const;
    QString getResultFileSuffix() const;
private:
    int m_portId;
    double m_turn;
    double m_storagePressure;
    double m_reactionPressure;
    double last_time_pass;

    double m_flow_coef; // make conts
    double m_choked_curr;
    double m_subsonic_curr;

    bool m_leakageOpen;
    QStringList m_volumeNames;
    // double m_currentRate; 
    int todayRuns;
    int todayRunCount();
    double getFlowCoefficient(double turn); // replace to const
    double calculateRate(double sPressure, double rPressure, double rTempAbs) const;
    QFile leakageResultFile;
    QString resultFileSuffix;
    QElapsedTimer progressTime;
    QList<double> m_timePoints;
    QList<double> m_sPPoints;
    QList<double> m_rPPoints;
    QList<double> m_ratePoints;
    QList<double> m_modelPassPoints;
};