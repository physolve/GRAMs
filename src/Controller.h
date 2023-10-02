#pragma once

#include <QtCore/QObject>
#include "../lib/bdaqctrl.h"
using namespace Automation::BDaq;

class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(QString deviceName, QObject *parent = nullptr);
    virtual ~Controller();
Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    virtual void Initialization();
    QString deviceName;
    
    //QList<QList<QPointF>> m_data;
    //int m_index;

};

class AdvantechDO : public Controller
{
    Q_OBJECT
public:
    AdvantechDO(QString deviceName, QObject *parent = nullptr);
    ~AdvantechDO() override;
    void Initialization() override;
	void CheckError(ErrorCode errorCode);
Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    QString profilePath;
    
};

class AdvantechAI : public Controller
{
    Q_OBJECT
public:
    AdvantechAI(QString deviceName, QObject *parent = nullptr);
    ~AdvantechAI() override;
    void Initialization() override;
	void CheckError(ErrorCode errorCode);
Q_SIGNALS:

public slots:
    //void generateData(int type, int rowCount, int colCount);

private:
    int channelCount;
	int channelStart;
	ValueRange valueRange;
    QString profilePath;
};