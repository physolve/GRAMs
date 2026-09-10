#pragma once

#include "SerialInfo.h"
#include "../SensorQuality.h"
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
    bool openSerialPort();   // false = порт не открылся
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
    // Показание с признаком достоверности. getData() оставлен для графиков и
    // GUI, где качество не нужно; рецепт режима обязан читать quality().
    Quality quality() const;
private slots:
    void readData() override;
    void processEvents();
private:
    QTimer* m_timer;
    QByteArray m_data;
    double m_lastData;
    Quality m_quality = Quality::NoResponse;   // до первого кадра ответа нет
};

// ДВ302 — вакуумметр второго тракта (турбо-область, раздел 12.2).
//
// Это тот же прибор, который раньше стоял как ДВ301: обмен
// enquiry/acknowledgement, ответ с префиксом состояния. Отсюда готовый признак
// over range (REQ-081) — префикс '2', и under range — префикс '1'.
//
// Топология: магистраль → AR6/К176 (форвакуум) и SL2/К179 (турбо) → ДВ302 →
// SL1/К192 (выход второго тракта).
//
// В отличие от прежней схемы опрашивает себя сам, как VacuumController: период
// опроса не должен зависеть от такта softTimer приложения.
class TurboVacuumController : public SerialCtrl
{
    Q_OBJECT
public:
    TurboVacuumController(QObject *parent = nullptr);
    ~TurboVacuumController() override;
    void requestData() override;
    void requestRepetitive();
    void shuttingOff() override;
    void startReading();
    void stopReading();
    double getData() const;
    Quality quality() const;
signals:
    // void pressureChanged();
    // void pressureValChanged(); // temporally
    // void vacuumValChanged(); // temporally
private slots:
    void readData() override;
    void processEvents();
private:
    void rejectFrame(const QString &reason, bool garbage);
    QTimer* m_timer;
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
    Quality m_quality = Quality::NoResponse;   // до первого кадра ответа нет
    // Опросов подряд без разобранного кадра. Растёт в processEvents (такт 1 с),
    // обнуляется каждым удачным кадром.
    int m_silentPolls = 0;
    static constexpr int kStalePolls = 3;      // 3 с молчания → Stale
    static constexpr int kDeadPolls  = 6;      // 6 с молчания → NoResponse
};
