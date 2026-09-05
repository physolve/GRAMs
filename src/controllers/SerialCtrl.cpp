#include "SerialCtrl.h"

#include <cmath>

SerialCtrl::SerialCtrl(QObject *parent) : 
    QObject(parent), m_serial(new QSerialPort(this)), threshold(0) // unique pointer
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

VacuumController::VacuumController(QObject *parent) :
    SerialCtrl(parent), m_timer(new QTimer(this)), m_lastData(0)
{
    connect(m_serial, &QSerialPort::readyRead, this, &VacuumController::readData);
    connect(m_timer, &QTimer::timeout, this, &VacuumController::processEvents);
}

VacuumController::~VacuumController(){
    m_timer->stop();
}

void VacuumController::requestData(){
    if(!m_serial->isOpen())
        return;
    m_serial->write("001M^\r");
}

void VacuumController::processEvents(){
    requestData();
}

void VacuumController::readData(){
    m_data.append(m_serial->readAll());
    if (m_data.length() < 12)
        return;                     // serial reads arrive fragmented; wait for full frame
    //data format 001M100023D\r -> 1.000Ex (x = 23-20 = 3)
    const QByteArray data = m_data;
    m_data.clear();

    QString responce = QString::fromLocal8Bit(data);
    responce.remove(0, 4);          // strip "001M" header
    responce.chop(2);               // strip checksum char + CR

    bool ok = true;
    double result = responce.first(4).toDouble(&ok) / 1000.0;
    int mantissa = responce.last(2).toInt() - 20;

    if (result != 0 && ok) {
        m_lastData = result * std::pow(10, mantissa);
        qDebug() << "Vacuum:" << m_lastData;
    }
    else {
        if (++threshold > 3) {
            shuttingOff();
        }
    }
}

void VacuumController::shuttingOff(){
    qDebug() << "Wrong data in sensor!";
    stopReading();
    //emit to qml status about error
}

void VacuumController::startReading(){
    threshold = 0;
    m_timer->start(1000);
}

void VacuumController::stopReading(){
    m_timer->stop();
    m_lastData = 0;
}

double VacuumController::getData() const{
    return m_lastData;
}

TurboVacuumController::TurboVacuumController(QObject *parent) : SerialCtrl(parent), lastData(0), isEnquiry(false)
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
    connect(m_serial, &QSerialPort::readyRead, this, &TurboVacuumController::readData);
}

void TurboVacuumController::requestData(){
    // default request
    m_serial->write(requestArray);
}

void TurboVacuumController::requestRepetitive(){
    if(!m_serial->isWritable()){
        return;
    }
    QByteArray enquiry;
    enquiry.resize(1);
    enquiry[0] = 0x05;
    m_serial->write(enquiry);
}

void TurboVacuumController::readData(){
    if(!isEnquiry){
        const QByteArray data = m_serial->readAll();
        const QString responce = QString::fromLocal8Bit(data);
        QByteArray acknolegement;
        acknolegement.resize(3);
        acknolegement[0] = '\x06';
        acknolegement[1] = '\r';
        acknolegement[2] = '\n';
        if(data == acknolegement){
            isEnquiry = true;
        }
        return;
    }
    if(!m_serial->canReadLine()){
        return;
    }
    // requestArray value read from single gauge
    const QByteArray data = m_serial->readLine();
    // const QString responce = QString::fromLocal8Bit(data);
    double value = 0;
    if(data.startsWith('2')){
        // 2 -> overrange
        value = 761; // torr   
    }
    else if(data.startsWith('1')){
        // 1 -> underrange
        value = 1e-8; // torr ?
    }
    else if(data.startsWith('0')){
        // 0 -> measurement OK
        value = data.mid(3,10).toDouble(); // ok?
    }
    else{
        qDebug() << "Upredicted vacuum responce";
    }
    lastData = value;
}

double TurboVacuumController::getData() const{
    return lastData;
}
