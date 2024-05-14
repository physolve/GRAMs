#pragma once
#include <QDebug>
#include <Eigen/Dense>
#include "../lib/kalman.hpp"
class VoltageFilter{
public:
    VoltageFilter();
    void appendToBuffer(const double &value);
    QVector<double> getFilteredVoltage();  
private:
    QVector<double> m_voltageBuffer;
    int n, m;
    double dt;
    KalmanFilter kf;
};