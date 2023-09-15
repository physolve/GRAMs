#pragma once

#include <QApplication>
#include <QQmlApplicationEngine>

#include "DataAcquisition.h"
#include "DataModel.h"

class Grams : public QApplication
{
    Q_OBJECT

public:
    Grams(int &argc, char **argv);
    ~Grams();

private:

    void initGUI();

    QQmlApplicationEngine m_engine;
    DataAcquisition dataSource;
    MyModel dataModel;
};