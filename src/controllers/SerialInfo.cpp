#include "SerialInfo.h"

#include <QSerialPortInfo>

static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A"); // ?

bool SerialInfo::serialPortsInfo(QStringList& serialNames){
    QString description;
    QString manufacturer;
    QString serialNumber;
    const auto infos = QSerialPortInfo::availablePorts();
    if (infos.isEmpty())
    {
        qDebug() << "No serial ports connected";
        return false;
    }
    for (const QSerialPortInfo &info : infos) {
        QStringList list;
        description = info.description();
        manufacturer = info.manufacturer();
        serialNumber = info.serialNumber();
        list << info.portName()
             << (!description.isEmpty() ? description : blankString)
             << (!manufacturer.isEmpty() ? manufacturer : blankString)
             << (!serialNumber.isEmpty() ? serialNumber : blankString)
             << info.systemLocation()
             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
             << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);
        QString c_name = "unknown";

        serialNames << info.description();
    }
    return true;
}

SerialInfo::SerialInfo(QObject *parent) :
    QObject(parent)
{
    // pressureConnected = false;

    // for(QString c_name : m_serialPortList.keys()){
    //     QVariantMap map; 
    //     // default?
    //     map["serialPortInfo"] = m_serialPortList[c_name].toStringList().first();
    //     map["baudRate"] = QSerialPort::Baud9600;
    //     map["dataBitsBox"] = QSerialPort::Data8;
    //     map["parityBox"] = QSerialPort::NoParity;
    //     map["stopBitsBox"] = QSerialPort::OneStop;
    //     // apply(map,c_name);
    // }
}

SerialInfo::~SerialInfo()
{
}

SerialPortInfo SerialInfo::settings(const QString &c_name) const
{
    // return m_SerialInfoMap[c_name];
}

// QVariantMap SerialInfo::serialPortListRead() const{
//     return m_serialPortList;
// }

// void SerialInfo::apply(const QVariantMap& map, const QString &c_name)
// {
//     m_SerialInfoMap[c_name].portName = map["serialPortInfo"].toString();
//     qDebug() << map["baudRate"].toInt();
//     m_SerialInfoMap[c_name].baudRate = static_cast<QSerialPort::BaudRate>(map["baudRate"].toInt());
//     m_SerialInfoMap[c_name].stringBaudRate = map["baudRate"].toString();
//     qDebug() << map["dataBitsBox"].toInt();
//     m_SerialInfoMap[c_name].dataBits = static_cast<QSerialPort::DataBits>(map["dataBitsBox"].toInt());
//     m_SerialInfoMap[c_name].stringDataBits = map["dataBitsBox"].toString();

//     m_SerialInfoMap[c_name].parity = static_cast<QSerialPort::Parity>(map["parityBox"].toInt());
//     //qDebug() << "Parity box: " << m_currentSettings.parity;
//     m_SerialInfoMap[c_name].stringParity = map["parityBox"].toString();
//     m_SerialInfoMap[c_name].stopBits = static_cast<QSerialPort::StopBits>(map["stopBitsBox"].toInt());
//     //qDebug() << "stop bits: " <<  m_currentSettings.stopBits;
//     m_SerialInfoMap[c_name].stringStopBits = map["stopBitsBox"].toString();

//     // m_currentSettings.flowControl = static_cast<QSerialPort::FlowControl>(map["flowControlBox"].toInt());
//     // m_currentSettings.stringFlowControl = map["flowControlBox"].toString();
//     // m_currentSettings.localEchoEnabled = map["localEchoCheckBox"].toBool();
// }

// bool SerialInfo::isPressureConnected(){
//     return pressureConnected;
// }