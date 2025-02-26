#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>

#include "DataAcquisition.h"
#include "DataModel.h"
#include "ValveModel.h"
#include "Initialize.h"
#include "Security.h"

class Grams : public QApplication
{
    Q_OBJECT

public:
    Grams(int &argc, char **argvm, const QString &curInitProfile);
    ~Grams();
    // Q_INVOKABLE void initializeReading();
    //Q_INVOKABLE void testRead();
    // add JSON and profile here?
    Q_INVOKABLE void setValveState(bool state, int valveId);
    // Q_INVOKABLE void setValveState(const QString &name, const bool &state);
    Q_PROPERTY (bool vAR1State READ getVAR1State NOTIFY valveChanged)
    Q_PROPERTY (bool vAR2State READ getVAR2State NOTIFY valveChanged)
    Q_PROPERTY (bool vAR3State READ getVAR3State NOTIFY valveChanged)
    Q_PROPERTY (bool vAR4State READ getVAR4State NOTIFY valveChanged)
    Q_PROPERTY (bool vAR5State READ getVAR5State NOTIFY valveChanged)
    Q_PROPERTY (bool vAR6State READ getVAR6State NOTIFY valveChanged)
    Q_PROPERTY (bool vS1State READ getVS1State NOTIFY valveChanged)
    Q_PROPERTY (bool vS2State READ getVS2State NOTIFY valveChanged)
    Q_PROPERTY (bool vS3State READ getVS3State NOTIFY valveChanged)
    Q_PROPERTY (bool vS4State READ getVS4State NOTIFY valveChanged)
    Q_PROPERTY (bool vR1State READ getVR1State NOTIFY valveChanged)
    Q_PROPERTY (bool vR2State READ getVR2State NOTIFY valveChanged)
    Q_PROPERTY (bool vR3State READ getVR3State NOTIFY valveChanged)
    Q_PROPERTY (bool vR4State READ getVR4State NOTIFY valveChanged)
    Q_PROPERTY (bool vR5State READ getVR5State NOTIFY valveChanged)
    Q_PROPERTY (bool vSL1State READ getVSL1State NOTIFY valveChanged)
    Q_PROPERTY (bool vSL2State READ getVSL2State NOTIFY valveChanged)

signals:
    void valveChanged();

private slots:
    void softEvent();

private:
    void advDoController();
    void initGUI();
    void initDigitalData();
    void readingEvent(bool valveCheck); // valve check connect by new data signal

    Initialize initSource;
    DataAcquisition dataSource; // pass from constructor
    ValveModel valveModel;
    MyModel dataModel;
    Security m_safeModule; // naming?
    QQmlApplicationEngine m_engine;

    // those are required
    Valve vAR1; // 0 k104
    Valve vAR2; // 1 k109
    Valve vAR3; // 2 k114
    Valve vAR4; // 3 k118
    Valve vAR5; // 4 k178
    Valve vSL2; // k192
    Valve vAR6; // k176
    Valve vSL1; // k179
    Valve vS4; // k171
    Valve vS1; // k131
    Valve vS2; // k133
    Valve vS3; // k135
    Valve vR1; // k155
    Valve vR2; // k153
    Valve vR3; // k151
    Valve vR4; // k173
    Valve vR5; // chamber manual (config)

    bool getVAR1State() const;
    bool getVAR2State() const;
    bool getVAR3State() const;
    bool getVAR4State() const;
    bool getVAR5State() const;
    bool getVAR6State() const;
    bool getVS1State() const;
    bool getVS2State() const;
    bool getVS3State() const;
    bool getVS4State() const;
    bool getVR1State() const;
    bool getVR2State() const;
    bool getVR3State() const;
    bool getVR4State() const;
    bool getVR5State() const;
    bool getVSL1State() const;
    bool getVSL2State() const;
};