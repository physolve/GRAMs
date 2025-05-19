#include "SerialCtrl.h"

SerialCtrl::SerialCtrl(QObject *parent) : 
    QObject(parent), m_serial(new QSerialPort(this)) // unique pointer
{
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialCtrl::handleError);
    // connect(m_serial, &QSerialPort::readyRead, this, &SerialCtrl::readData);
    // setLogText("Click connect");
}
SerialCtrl::~SerialCtrl(){
    this->closeSerialPort();
    delete m_serial;
}

void SerialCtrl::setSerialPortInfo(const SerialPortInfo &serialInfo){
    m_serialInfo = serialInfo;
}

void SerialCtrl::openSerialPort()
{
    m_serial->setPortName(m_serialInfo.portName);
    m_serial->setBaudRate(m_serialInfo.baudRate);
    m_serial->setDataBits(m_serialInfo.dataBits);
    m_serial->setParity(m_serialInfo.parity);
    m_serial->setStopBits(m_serialInfo.stopBits);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to " << m_serialInfo.description;
        // setLogText(tr("Connected to %1 : %2, %3, %4, %5")
        //                   .arg(m_serialInfo.portName).arg(m_serialInfo.stringBaudRate).arg(m_serialInfo.stringDataBits)
        //                   .arg(m_serialInfo.stringParity).arg(m_serialInfo.stringStopBits));
    } else {
        // setLogText(tr("Open error"));
        qDebug() << "Serial open error";
    }
}

void SerialCtrl::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    // setLogText(tr("Disconnected"));
    qDebug() << "Serial disconnected";
}

void SerialCtrl::requestData(){
    const QString query = "#01\r"; 
    m_serial->write(query.toLocal8Bit());
}

// void SerialCtrl::writeData() // virtual?
// {
//     const QString query = "#01\r"; 
//     m_serial->write(query.toLocal8Bit());
// }

void SerialCtrl::readData() // virtual?
{
    const QByteArray data = m_serial->readAll();
    qDebug() << "BASE CLASS" << QString::fromLocal8Bit(data);
}

void SerialCtrl::shuttingOff(){
    qDebug() << "Wrong data in sensor!";
    // stopReading();
    //emit to qml status about error   
}

void SerialCtrl::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        //QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void SerialCtrl::setLogText(const QString &text)
{
    if (text != logText)
    {
        logText = text;
        emit logChanged(text);
    }
}

VacuumController::VacuumController(QObject *parent) : SerialCtrl(parent)
{
    // default request
    requestArray.resize(6);
    requestArray[0] = 0x50; // P
    requestArray[1] = 0x52; // R
    requestArray[2] = 0x31; // 1
    requestArray[3] = 0x0D; // CR
    requestArray[4] = 0x0A; // LF
    requestArray[5] = 0x00; // NUL
    askData.resize(6); 
    const char a[6] = {'P', 'R', '1', '\r', '\n', '\0'};
    askData = QByteArray::fromHex(a);
    // other commands
    connect(m_serial, &QSerialPort::readyRead, this, &VacuumController::readData);

}

void VacuumController::requestData(){
    m_serial->write(requestArray);
}

void VacuumController::readData(){
    const QByteArray data = m_serial->readAll();
    QString responce = QString::fromLocal8Bit(data);
    qDebug() << responce;
    // if(!responce.endsWith('\r')){
    //     m_bufferData = responce;
    //     return;
    // }
    // responce = m_bufferData + responce;
    // responce.remove(0,1);
    // responce.chop(1);
    // QStringList channelsVoltage = responce.split('+', Qt::SkipEmptyParts);
    // //qDebug() << channelsVoltage;
    // bool ok = true;
    // auto voltageVacuum = channelsVoltage.at(1).toDouble(&ok);

    // ?
    // char requestENQ[1];
    // requestENQ[0] = 0x05; // ENQ

    // auto point_vac = 0.0;
    // if(voltageVacuum != 0 && ok){
    //     point_vac = filterData_vac(voltageVacuum);
    //     threshold = 0;
    // }
    // else{
    //     point_vac = 0.0;
    //     if(++threshold>3){ //?
    //         shuttingOff();
    //     }
    // }
    
}