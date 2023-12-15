#pragma once

#include <QtCore/QObject>
#include "../lib/bdaqctrl.h"
#include "ControllerInfo.h"
using namespace Automation::BDaq;


class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(const ControllerInfo &info, QObject *parent = nullptr);
    virtual ~Controller();
    void Initialization();
//Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    ControllerInfo m_info;
    //QList<QList<QPointF>> m_data;
    //int m_index;

};

class AdvantechTest : public QObject // can it be universal? maybe create advantechBase
{
    Q_OBJECT
public:
    AdvantechTest(const ControllerInfo &info, QObject *parent = nullptr);
    virtual ~AdvantechTest();
    void Initialization() ; //override
    void ConfigureDeviceTest();
	void CheckError(ErrorCode errorCode);
    QString m_deviceName;
    const ControllerInfo& getInfo();
    void resizeDataVector(uint8_t size);
    void readData();
    QVector<double> getData();
//Q_SIGNALS:

public slots:
    //void settingAccepted();
    //void generateData(int type, int rowCount, int colCount);

private:
    ControllerInfo m_info;
    
    ValueRange m_valueRange;
    InstantAiCtrl* m_instantAiCtrl; // change to smart pointer or initialize inside class 
    QVector<double> m_vector;
	//double scaledData[16];
};

class AdvantechDO : public QObject
{
    Q_OBJECT
public:
    AdvantechDO(const ControllerInfo &info, QObject *parent = nullptr);
    ~AdvantechDO();
    void Initialization() ; //override
    void ConfigureDeviceDO();
	void CheckError(ErrorCode errorCode);
//Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    ControllerInfo m_info;
    QString profilePath;
    
};

class AdvantechAI : public QObject
{
    Q_OBJECT
public:
    AdvantechAI(const ControllerInfo &info, QObject *parent = nullptr);
    ~AdvantechAI();
    void Initialization() ; //override
    void ConfigureDeviceAI();
	void CheckError(ErrorCode errorCode);
//Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    ControllerInfo m_info;
    int channelCount;
	int channelStart;
	ValueRange valueRange;
    QString profilePath;
};