#include "VoltageFilter.h"

    // int n = 3; // Number of states
    // int m = 1; // Number of measurements

    // double dt = 1.0/30; // Time step

    //let's try 500 Hz time step (1.0/500)

VoltageFilter::VoltageFilter(): n(3), m(1), dt(1.0/500) {

    Eigen::MatrixXd A(n, n); // System dynamics matrix
    Eigen::MatrixXd C(m, n); // Output matrix
    Eigen::MatrixXd Q(n, n); // Process noise covariance
    Eigen::MatrixXd R(m, m); // Measurement noise covariance
    Eigen::MatrixXd P(n, n); // Estimate error covariance

    // Discrete LTI projectile motion, measuring position only
    A << 1, dt, 0, 0, 1, dt, 0, 0, 1;
    C << 1, 0, 0;

    // Reasonable covariance matrices
    Q << .05, .05, .0, .05, .05, .0, .0, .0, .0;
    R << 5;
    P << .1, .1, .1, .1, 10000, 10, .1, 10, 100;

    // Construct the filter
    kf = KalmanFilter (dt, A, C, Q, R, P);
    qDebug() << "Kalman created";
}


VoltageFilter::VoltageFilter(const FilterMatrix &parameters):n(3), m(1), dt(1.0/500) {
    Eigen::MatrixXd R(m, m); // Measurement noise covariance

    std::vector<double> a(parameters.mA.constBegin(), parameters.mA.constEnd());
    Eigen::MatrixXd A = Eigen::Map<Eigen::MatrixXd>(a.data(), n, n);
    std::vector<double> c(parameters.mC.constBegin(), parameters.mC.constEnd());
    Eigen::MatrixXd C = Eigen::Map<Eigen::MatrixXd>(c.data(), m, n);
    std::vector<double> q(parameters.mQ.constBegin(), parameters.mQ.constEnd());
    Eigen::MatrixXd Q = Eigen::Map<Eigen::MatrixXd>(q.data(), n, n);
    R << parameters.mR;
    std::vector<double> p(parameters.mP.constBegin(), parameters.mP.constEnd());
    Eigen::MatrixXd P = Eigen::Map<Eigen::MatrixXd>(p.data(), n, n);
    kf = KalmanFilter (dt, A, C, Q, R, P);
    qDebug() << "Custom Kalman created";
}

void VoltageFilter::appendToBuffer(const double &value){ // change to replace Vector
    m_voltageBuffer << value;
}

QVector<double> VoltageFilter::getFilteredVoltage(bool debug) {
    // Best guess of initial states
    Eigen::VectorXd x0(n);
    double t = 0;
    x0 << m_voltageBuffer[0], 0, -9.81;
    kf.init(t, x0);
    // Feed measurements into filter, output estimated states
    QVector<double> filteredVoltage;

    Eigen::VectorXd y(m);
    if(debug) qDebug() << m_voltageBuffer;
    for(int i = 0; i < m_voltageBuffer.size(); i++) {
        //QVector<double> buffVector;
        t += dt;
        y << m_voltageBuffer[i];
        kf.update(y);
        // qDebug() << "t = " << t << ", x_hat[" << i << "] = " << kf.state().transpose();

        // buffVector.resize(y.transpose().size());
        // Eigen::Map<Eigen::VectorXd>(buffVector.data(), buffVector.size()) = y.transpose();
        filteredVoltage << kf.state().transpose()[0];//buffVector;
    }
    m_filteredVoltage = filteredVoltage;
    m_voltageBuffer.clear();
    return filteredVoltage;
}

QVector<double> VoltageFilter::lastFiltered(){
    return m_filteredVoltage;
}