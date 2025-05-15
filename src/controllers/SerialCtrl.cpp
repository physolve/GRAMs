#include "SerialCtrl.h"

SerialCtrl::SerialCtrl(QObject *parent) : 
    QObject(parent), m_timer(new QTimer), m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialCtrl::handleError);

    connect(m_serial, &QSerialPort::readyRead, this, &SerialCtrl::readData);

    connect(m_timer, &QTimer::timeout, this, &SerialCtrl::processEvents);

    setLogText("Click connect");
}
SerialCtrl::~SerialCtrl(){
    m_timer->stop();
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
        setLogText(tr("Connected to %1 : %2, %3, %4, %5")
                          .arg(m_serialInfo.portName).arg(m_serialInfo.stringBaudRate).arg(m_serialInfo.stringDataBits)
                          .arg(m_serialInfo.stringParity).arg(m_serialInfo.stringStopBits));
    } else {
        setLogText(tr("Open error"));
    }
}

void SerialCtrl::closeSerialPort()
{
    if (m_serial->isOpen())
        m_serial->close();
    setLogText(tr("Disconnected"));
}

void SerialCtrl::writeData() // virtual?
{
    const QString query = "#01\r"; 
    m_serial->write(query.toLocal8Bit());
}

void SerialCtrl::readData() // virtual?
{
    const QByteArray data = m_serial->readAll();
    qDebug() << "BASE CLASS" << QString::fromLocal8Bit(data);
}

void SerialCtrl::shuttingOff(){
    qDebug() << "Wrong data in sensor!";
    stopReading();
    //emit to qml status about error   
}

void SerialCtrl::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        //QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void SerialCtrl::startReading()
{
    threshold = 0;
    m_timer->start(1000);
}
void SerialCtrl::stopReading(){
    m_timer->stop();
}

void SerialCtrl::processEvents(){
    //setLogText("");
    writeData();
}

void SerialCtrl::setLogText(const QString &text)
{
    if (text != logText)
    {
        logText = text;
        emit logChanged(text);
    }
}

VacuumController::VacuumController(const SerialPortInfo &serialInfo, QObject *parent) : SerialCtrl(parent)
{
    this->setSerialPortInfo(serialInfo);
    //query("#01\r")
    //, currentPressure(0), currentVacuum(0)
}
// #010\r to read only first channel, #000\r to read all channels, but what is syntax? 
void VacuumController::writeData(){
    m_serial->write(query.toLocal8Bit());
}

void VacuumController::readData(){
    const QByteArray data = m_serial->readAll();
    //data format "+00.000\r" For ONE channel
    //data format: +000.00+000.00+000.00...+000.00\r

    // QString responce = QString::fromLocal8Bit(data);
    // responce.remove(0,1);
    // responce.chop(1);

    QString responce = QString::fromLocal8Bit(data);
    if(!responce.endsWith('\r')){
        m_bufferData = responce;
        return;
    }
    responce = m_bufferData + responce;
    responce.remove(0,1);
    responce.chop(1);
    QStringList channelsVoltage = responce.split('+', Qt::SkipEmptyParts);
    //qDebug() << channelsVoltage;
    bool ok = true;
    auto voltageVacuum = channelsVoltage.at(1).toDouble(&ok);

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
    
    // currentVacuum = point_vac;
    
    // const auto &c_time = m_programmTime.elapsed()/1000;
    // timeData->addPoint(c_time);
    // pressure->addPoint(point_pr);
    // emit pressureValChanged();
    // vacuum->addPoint(point_vac);
    // emit vacuumValChanged();
    // emit pressureChanged();
}

// QMap<QString, double> PressureController::getLastChanged(){
//     QMap<QString, double> points;
//     points["pressure"] = currentPressure;
//     points["vacuum"] = currentVacuum;
//     return points;
// }

void VacuumController::stopReading(){
    m_timer->stop();
    // currentPressure = 0;
    // currentVacuum = 0;
}