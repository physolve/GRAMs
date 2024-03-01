#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>

#include "DataAcquisition.h"
#include "DataModel.h"
#include "ValveModel.h"
#include "Initialize.h"

class Grams : public QApplication
{
    Q_OBJECT

public:
    Grams(int &argc, char **argv);
    ~Grams();
    Q_INVOKABLE void initializeModel();
    Q_INVOKABLE void testRead();
    Q_INVOKABLE void testSet();
    // add JSON and profile here?
    Q_INVOKABLE void setValveState(QString name, bool state);
private slots:
    void softEvent();

private:

    void initGUI();

    QQmlApplicationEngine m_engine;
    Initialize initSource;
    DataAcquisition dataSource;
    ValveModel valveModel;
    MyModel dataModel;

    QTimer *softTimer;
};