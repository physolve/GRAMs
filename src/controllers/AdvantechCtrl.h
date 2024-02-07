#pragma once

#include <QtCore/QObject>
#include "../../lib/bdaqctrl.h"
#include "ControllerInfo.h"
using namespace Automation::BDaq;


class AdvantechCtrl : public QObject
{
    Q_OBJECT
public:
    AdvantechCtrl(QString name, QObject *parent = nullptr);
    virtual ~AdvantechCtrl();
    virtual void Initialization();
    virtual void readData(); 
protected:
    QString m_name;

};

class AdvantechAI : public AdvantechCtrl
{
    Q_OBJECT
public:
    AdvantechAI(const AdvAIType &info, QObject *parent = nullptr); 
    virtual ~AdvantechAI();
    void Initialization() ; //override
    void initialInfo();
    void ConfigureDeviceTest();
	void CheckError(ErrorCode errorCode);
    const AdvAIType& getInfo(); // move to base class
    void resizeDataVector(uint8_t size);
    void readData() override;
    const QVector<double> getData();
//Q_SIGNALS:

public slots:
    //void settingAccepted();
    //void generateData(int type, int rowCount, int colCount);

private:
    AdvAIType m_info;
    
    ValueRange m_valueRange;
    InstantAiCtrl* m_instantAiCtrl; // change to smart pointer or initialize inside class 
    QVector<double> m_vector;
	//double scaledData[16];
};

class AdvantechDO : public AdvantechCtrl
{
    Q_OBJECT
public:
    AdvantechDO(const AdvDOType &info, QObject *parent = nullptr);
    ~AdvantechDO();
    void ConfigureDeviceDO();
	void CheckError(ErrorCode errorCode);
    void applyFeatures();
    void resizeDataVector(uint8_t size);
    void readData() override;
    void setData();
    const AdvDOType& getInfo(); // move to base class
    const QVector<bool> getData();
//Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    AdvDOType m_info;
    InstantDoCtrl* m_instantDoCtrl;  // change to smart pointer or initialize inside class
    int portCount;
    QVector<bool> m_vector;
    quint8 m_portMasks;
    quint8 m_portStates;
    int key;
	quint8 mask;
	quint8 state;
};