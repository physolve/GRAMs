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
    Grams(int &argc, char **argv);
    ~Grams();
    Q_INVOKABLE void initializeReading();
    //Q_INVOKABLE void testRead();
    // add JSON and profile here?
    Q_INVOKABLE void setValveState(const QString &name, const bool &state);
private slots:
    void softEvent();

private:

    void initGUI();

    void readingEvent();

    QQmlApplicationEngine m_engine;
    Initialize initSource;
    DataAcquisition dataSource;
    ValveModel valveModel;
    MyModel dataModel;
    Security m_safeModule; // naming?
    QTimer *softTimer;
};