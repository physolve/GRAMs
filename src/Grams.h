#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>

#include "DataAcquisition.h"
#include "DataModel.h"
#include "Initialize.h"

class Grams : public QApplication
{
    Q_OBJECT

public:
    Grams(int &argc, char **argv);
    ~Grams();
    Q_INVOKABLE void initializeModel();
    // add JSON and profile here?

private slots:
    void softEvent();

private:

    void initGUI();

    QQmlApplicationEngine m_engine;
    Initialize initSource;
    DataAcquisition dataSource;
    MyModel dataModel;

    QTimer *softTimer;
};