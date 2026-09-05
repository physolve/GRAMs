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
    virtual void shuttingOff();
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

// Default vacuum gauge: USB-SERIAL CH340 adapter, ASCII frame protocol
// (see PhyGROM ControllerSerial). Polls the gauge on its own 1 s timer.
class VacuumController : public SerialCtrl
{
    Q_OBJECT
public:
    VacuumController(QObject *parent = nullptr);
    ~VacuumController() override;
    void requestData() override;
    void shuttingOff() override;
    void startReading();
    void stopReading();
    double getData() const;
private slots:
    void readData() override;
    void processEvents();
private:
    QTimer* m_timer;
    QByteArray m_data;
    double m_lastData;
};

// Backup option: Pfeiffer-style turbo pump gauge (enquiry/acknowledgement
// protocol over "USB Serial Port"). Former VacuumController implementation.
class TurboVacuumController : public SerialCtrl
{
    Q_OBJECT
public:
    TurboVacuumController(QObject *parent = nullptr);
    void requestData() override;
    void requestRepetitive();
    double getData() const;
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
    bool isEnquiry;
    double lastData;
};
