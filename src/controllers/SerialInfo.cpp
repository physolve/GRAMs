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
        // make single description: description, port
        serialNames << QString("%1, %2").arg(info.description(), info.portName());
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
    return SerialPortInfo();
}

// QVariantMap SerialInfo::serialPortListRead() const{
//     return m_serialPortList;
// }


// bool SerialInfo::isPressureConnected(){
//     return pressureConnected;
// }