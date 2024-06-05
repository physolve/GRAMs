#pragma once

#include <QtCore/QObject>
#include "../../lib/bdaqctrl.h"
#include "ControllerInfo.h"

#include "../VoltageFilter.h"

using namespace Automation::BDaq;


class AdvantechCtrl : public QObject
{
    Q_OBJECT
public:
    AdvantechCtrl(const QString &name = "unknown", QObject *parent = nullptr);
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
    void ConfigureDeviceTest(); // rename TEST
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

class AdvantechBuff : public AdvantechCtrl
{
    Q_OBJECT
public:
    AdvantechBuff(const AdvAIType &info, QObject *parent = nullptr); 
    virtual ~AdvantechBuff();
    void Initialization() ; //override
    void initialInfo();
    void ConfigureDeviceTest(); // rename TEST
	void CheckError(ErrorCode errorCode);
    const AdvAIType& getInfo(); // move to base class
    
    void readData() override;
    const QVector<double> getData();
    const QVector<double> getBufferedData(uint8_t channelN, bool debug);
    const QVector<qreal> getTimeBuffer();
    static void BDAQCALL OnStoppedEvent(void *sender, BfdAiEventArgs *args, void *userParam);
//Q_SIGNALS:
public slots:
    //void settingAccepted();
    //void generateData(int type, int rowCount, int colCount);
    //void setFilterMatrix();

private:
    void resizeDataVector(uint8_t size);
    void resizeVoltageFilterList(uint8_t size);
    void setVoltageToFilter(const QVector<double> &voltageBuffer);
    void doFilter();
    
    AdvAIType m_info;
    const int m_sectionLength = 128;
    ValueRange m_valueRange;
    WaveformAiCtrl* m_waveformAiCtrl; // change to smart pointer or initialize inside class
    QVector<double> m_vector; // should be list of values 
	QList<VoltageFilter> m_voltageFilters; // to filterView?
    
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
    AdvDOType getInfo(); // move to base class
    QVector<bool> getData();
    void setData(const QVector<bool> &changedState);
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