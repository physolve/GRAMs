#pragma once

#include <QObject>
#include <QFile>
#include "../DataCollection.h"
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
    void startCalc();
    void endCalc();
    void addMeasure(double time_pass);
    double getFlowCoefficient(double turn);
    void calculateRate(double flow_factor, double diff_pres);
    void saveResultsToFile();
private:
    double calcualteModelPass(double time_pass);
    int m_portId;
    double m_turn;
    double m_currentRate;
    double m_portPressure;
    double m_flowPass;
    double last_time_pass;
    int supplyResultCount;
    QFile supplyResultFile;
    // to save
    FilterData* m_supplyPressure;
    FilterData* pseudo_time;
    QList<double> m_timePoints;
    QList<double> m_pressurePoints;
    QList<double> m_modelPassPoints;
};