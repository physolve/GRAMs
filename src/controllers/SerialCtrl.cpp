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

bool SerialCtrl::openSerialPort()
{
    m_serial->setPortName(m_serialInfo.portName);
    m_serial->setBaudRate(m_serialInfo.baudRate);
    m_serial->setDataBits(m_serialInfo.dataBits);
    m_serial->setParity(m_serialInfo.parity);
    m_serial->setStopBits(m_serialInfo.stopBits);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to " << m_serialInfo.description
                 << "on" << m_serialInfo.portName;
        // setLogText(tr("Connected to %1 : %2, %3, %4, %5")
        //                   .arg(m_serialInfo.portName).arg(m_serialInfo.stringBaudRate).arg(m_serialInfo.stringDataBits)
        //                   .arg(m_serialInfo.stringParity).arg(m_serialInfo.stringStopBits));
        return true;
    }
    // setLogText(tr("Open error"));
    qDebug() << "Serial open error:" << m_serialInfo.portName
             << m_serial->errorString();
    return false;
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
        m_quality  = Quality::Valid;
        threshold  = 0;          // счётчик мусора сбрасывается удачным кадром,
                                 // иначе 4 плохих кадра за всю сессию глушат
                                 // датчик навсегда
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

Quality VacuumController::quality() const{
    return m_quality;
}

void VacuumController::startReading(){
    threshold = 0;
    m_quality = Quality::NoResponse;   // валидным станет после первого кадра
    m_timer->start(1000);
}

void VacuumController::stopReading(){
    m_timer->stop();
    // Показание НЕ обнуляется: 0 Па меньше любого порога вакуума, поэтому
    // мёртвый датчик выглядел бы как достигнутая цель откачки. Значение
    // остаётся последним известным, а качество говорит, что верить ему нельзя.
    m_quality = Quality::NoResponse;
}

double VacuumController::getData() const{
    return m_lastData;
}

TurboVacuumController::TurboVacuumController(QObject *parent) :
    SerialCtrl(parent), m_timer(new QTimer(this)), isEnquiry(false), lastData(0)
{
    connect(m_timer, &QTimer::timeout, this, &TurboVacuumController::processEvents);
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

TurboVacuumController::~TurboVacuumController(){
    m_timer->stop();
}

void TurboVacuumController::requestData(){
    if(!m_serial->isOpen())
        return;
    // default request
    isEnquiry = false;          // новый цикл: ждём ACK, потом строку данных
    m_serial->write(requestArray);
}

// Один цикл опроса: запрос → ACK → enquiry → строка с показанием.
//
// Здесь же сторож молчания. Кадр приходит по readyRead, поэтому «датчик замолк»
// не выражается ничем, кроме отсутствия события: без счётчика последнее
// показание навсегда осталось бы Valid, и рецепт держал бы К179 открытым по
// давно протухшему числу.
void TurboVacuumController::processEvents(){
    if(m_quality != Quality::NoResponse && ++m_silentPolls >= kStalePolls){
        if(m_silentPolls == kDeadPolls)
            qWarning() << "ДВ302: нет кадров" << kDeadPolls << "опросов подряд";
        m_quality = (m_silentPolls >= kDeadPolls) ? Quality::NoResponse
                                                  : Quality::Stale;
    }
    if(isEnquiry)
        requestRepetitive();    // ACK уже получен — забираем данные
    else
        requestData();
}

void TurboVacuumController::startReading(){
    threshold     = 0;
    m_silentPolls = 0;
    m_quality = Quality::NoResponse;
    isEnquiry = false;
    m_timer->start(1000);
}

void TurboVacuumController::stopReading(){
    m_timer->stop();
    // Как и у ДВ301: значение не обнуляем, недостоверность выражаем качеством.
    m_quality = Quality::NoResponse;
}

void TurboVacuumController::shuttingOff(){
    qDebug() << "ДВ302: некорректные данные датчика";
    stopReading();
}

Quality TurboVacuumController::quality() const{
    return m_quality;
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
    if(data.isEmpty()){
        rejectFrame(QStringLiteral("пустой кадр"), true);
        return;
    }
    // Кадр: "<статус>,<знак><мантисса>E<знак><порядок>", напр. "0,+1.5000E+00".
    // Статус — готовый признак диапазона, ради него и разбирается префикс.
    switch(data.at(0)){
    case '2':                          // over range — штатная фаза откачки
        lastData  = 761;               // torr
        m_quality = Quality::OverRange;
        break;
    case '1':                          // under range
        lastData  = 1e-8;              // torr ?
        m_quality = Quality::UnderRange;
        break;
    case '0': {                        // measurement OK
        // Число берём по запятой, а не по фиксированному смещению: в кадре без
        // знака мантиссы срез mid(3,10) съедал первую цифру и давал 0.5 вместо
        // 1.5 — ошибку в 3 раза, причём молча.
        const int comma = data.indexOf(',');
        bool ok = false;
        const double value = comma < 0 ? 0.0
                                       : data.mid(comma + 1).trimmed().toDouble(&ok);
        if(!ok){
            rejectFrame(QStringLiteral("статус 0, мантисса не разобрана"), true);
            return;
        }
        lastData  = value;
        m_quality = Quality::Valid;
        break;
    }
    case '3': case '4': case '5': case '6':
        // 3 — отказ датчика, 4 — датчик выключен, 5 — датчика нет,
        // 6 — ошибка идентификации. Прибор ОТВЕТИЛ, неисправен сам датчик,
        // поэтому опрос не глушим: оператор включит датчик — качество вернётся
        // само. Иначе выключенный на старте ДВ302 умирал бы на всю сессию.
        rejectFrame(QStringLiteral("датчик сообщает статус %1")
                        .arg(QChar::fromLatin1(data.at(0))), false);
        return;
    default:
        rejectFrame(QStringLiteral("нераспознанный ответ %1")
                        .arg(QString::fromLatin1(data.left(16))), true);
        return;
    }
    // Кадр разобран: сбрасываем счётчик мусора и сторожа молчания, иначе
    // редкий брак за всю сессию заглушил бы живой датчик.
    threshold     = 0;
    m_silentPolls = 0;
}

// Кадр не годится. Показание НЕ трогаем: обнулять нельзя — 0 Па меньше любого
// порога вакуума, и мёртвый датчик читался бы как достигнутая цель откачки.
// Недостоверность выражаем качеством.
//
// garbage = кадр не по протоколу (порт не тот, скорость не та): 4 подряд —
// и опрос глушим, чинить нужно снаружи. Осмысленный кадр об отказе датчика
// сюда приходит с garbage = false и опрос не останавливает.
void TurboVacuumController::rejectFrame(const QString &reason, bool garbage){
    qWarning() << "ДВ302: кадр отвергнут —" << reason;
    m_quality     = Quality::NoResponse;
    m_silentPolls = 0;                  // прибор ответил — молчания нет
    if(garbage && ++threshold > 3)
        shuttingOff();
}

double TurboVacuumController::getData() const{
    return lastData;
}
