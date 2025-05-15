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
    void startReading();
    void shuttingOff(); //?
    void stopReading();
    void openSerialPort();
    void closeSerialPort();

private slots:
    virtual void readData();
    void handleError(QSerialPort::SerialPortError error);
    void processEvents();

signals:
    //void mySettingsChanged();
    void logChanged(QString);
protected:
    void setLogText(const QString &text);
    QSerialPort *m_serial = nullptr;
    QTimer* m_timer;
    uint8_t threshold;
private:
    virtual void writeData() ;
    SerialPortInfo m_serialInfo;
    QString logText;
};

class VacuumController : public SerialCtrl
{
    Q_OBJECT
public:
    VacuumController(const SerialPortInfo &serialInfo, QObject *parent = nullptr);
    void stopReading();
signals:
    // void pressureChanged();
    // void pressureValChanged(); // temporally
    // void vacuumValChanged(); // temporally
private slots:
    void readData() override;
private:
    void writeData();
    const QString query;
    QString m_bufferData;

    // QElapsedTimer m_programmTime;
    // QSharedPointer<ControllerData> timeData;
    // QSharedPointer<ControllerData> pressure;
    // QSharedPointer<ControllerData> vacuum;
    // elapsedTimer?
};