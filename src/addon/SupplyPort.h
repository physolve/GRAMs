#pragma once

#include <QObject>
#include <QFile>
#include <QElapsedTimer>
// let's make it as experiment object with file saves
static float constexpr specific_gravity{0.07};
class SupplyPort : public QObject
{
    Q_OBJECT // ?
public:
    SupplyPort(QObject *parent = nullptr);
    ~SupplyPort();
    void setInitialParametersSupply(int portId, double turn, double portPressure);
    // info about port
    // data to file
    void initResultFile();
    void startCalc(double pressure_quartile, double initial_flow);
    void setPortOpen(bool state);
    void endCalc();
    void addMeasure(double pressure_quartile);
    double getFlowCoefficient(double turn);
    void calculateRate(double flow_factor, double pressure);
    void saveResultsToFile();
private:
    double calcualteModelPass(double time_differ);
    int m_portId;
    double m_turn;
    double m_currentRate;
    double m_portPressure;
    double m_flowPass;
    double last_time_pass;
    int supplyResultCount;
    bool m_portOpen;
    // assume volume pre valve
    void addPreValveFlow();
    QFile supplyResultFile;
    // to save
    QElapsedTimer progressTime;
    // Quartile pressure
    QList<double> m_timePoints;
    QList<double> m_pressurePoints;
    QList<double> m_ratePoints;
    QList<double> m_modelPassPoints;
};