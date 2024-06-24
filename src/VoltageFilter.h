#pragma once
#include <QDebug>
#include <Eigen/Dense>
#include "../lib/kalman.hpp"

struct FilterMatrix{
    QList<double> mA;
    QList<double> mC;
    QList<double> mQ;
    double mR;
    QList<double> mP;
};


class VoltageFilter{
public:
    VoltageFilter();
    VoltageFilter(const FilterMatrix &parameters);
    void appendToBuffer(const double &value);
    QVector<double> getFilteredVoltage(bool debug); // const?
    QVector<double> lastFiltered();
    QVector<double> lastXhatS();
    QVector<double> lastXhatT();
    void changeMatrixParameters(double n_dt = 1.0/500);
private:
    QVector<double> m_voltageBuffer;
    QVector<double> m_filteredVoltage;
    QVector<double> m_XhatS;
    QVector<double> m_XhatT;
    int n, m;
    double dt;
    KalmanFilter kf;
};