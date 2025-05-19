#pragma once

#include "SerialInfo.h"
// #include "datacollection.h"
#include <QSerialPort>
// #include <QElapsedTimer>
#include <QTimer>

class SerialCtrl : public QObject
{
    Q_OBJECT
    
public:
    explicit SerialCtrl(QObject *parent = nullptr);
    virtual ~SerialCtrl();
    void setSerialPortInfo(const SerialPortInfo &serialInfo);
    void shuttingOff(); //?
    void openSerialPort();
    void closeSerialPort();
    virtual void requestData();
private slots:
    virtual void readData();
    void handleError(QSerialPort::SerialPortError error);
signals:
    //void mySettingsChanged();
    void logChanged(QString);
protected:
    void setLogText(const QString &text);
    QSerialPort *m_serial = nullptr;
    // QTimer* m_timer;
    uint8_t threshold;
    SerialPortInfo m_serialInfo;
private:
    QString logText;
};

class VacuumController : public SerialCtrl
{
    Q_OBJECT
public:
    VacuumController(QObject *parent = nullptr);
    void requestData() override;
signals:
    // void pressureChanged();
    // void pressureValChanged(); // temporally
    // void vacuumValChanged(); // temporally
private slots:
    void readData() override;
private:
    QString m_bufferData;
    QByteArray requestArray;
    // QElapsedTimer m_programmTime;
    // QSharedPointer<ControllerData> timeData;
    // QSharedPointer<ControllerData> pressure;
    // QSharedPointer<ControllerData> vacuum;
    QByteArray askData;
    QByteArray enablePump;
    QByteArray startPump;
    QByteArray stopPump;
    QByteArray getPumpSpeed;
    QByteArray getPumpError;
    // QByteArray getPumpSpd; //?
};