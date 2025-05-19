#pragma once
#include <QSerialPort>
#include <QVariant>

struct SerialPortInfo {
    QString portName;
    QString description;
    QSerialPort::BaudRate baudRate;
    // QString stringBaudRate;
    QSerialPort::DataBits dataBits;
    // QString stringDataBits;
    QSerialPort::Parity parity;
    // QString stringParity;
    QSerialPort::StopBits stopBits;
    int timeout;
};

class SerialInfo : public QObject
{
    Q_OBJECT
public:

    explicit SerialInfo(QObject *parent = nullptr);
    ~SerialInfo();

    static bool serialPortsInfo(QStringList& serialNames);
    SerialPortInfo settings(const QString &c_name) const;

    // QVariantMap serialPortListRead() const;
    // bool isPressureConnected();

//private slots:

private:
    void fillPortsInfo();
    void updateSettings();
    // QStringList m_serial
    // QMap<QString,SerialPortInfo> m_SerialInfoMap;
    // QVariantMap m_serialPortList;

    // bool pressureConnected;
};