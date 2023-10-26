#include "DataAcquisition.h"
#include <QtCharts/QXYSeries>
#include <QtCharts/QAreaSeries>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtCore/QDebug>
#include <QtCore/QRandomGenerator>
#include <QtCore/QtMath>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>
#include <QTimer>

Q_DECLARE_METATYPE(QAbstractSeries *)
Q_DECLARE_METATYPE(QAbstractAxis *)


DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent),
    m_index(-1), m_timer(new QTimer(this))
{
    qRegisterMetaType<QAbstractSeries*>();
    qRegisterMetaType<QAbstractAxis*>();

    generateData(0, 5, 1024);

    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    
    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    connect(m_timer, &QTimer::timeout, this, &DataAcquisition::processEvents);
    QFile file;
    QDir dir(".");
    file.setFileName(":/MyApplication/profile/GRAMsPfp.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawData = file.readAll();
    file.close();
    QJsonDocument document   =   { QJsonDocument::fromJson(rawData.toUtf8()) };
    QJsonObject jsonObject = document.object();

    if(advantechDeviceCheck(m_connectedDevices)){
        // create rectangle in qml using map
        for(auto device : m_connectedDevices.keys()){
            auto description = m_connectedDevices.value(device).toString()+','+device;
            ControllerInfo a(description);
            m_deviceInfoList.append(a);
            // make qml combobox with ret.m_channelCount and ret.m_channelStart and profile and etc.
        }
        auto demoPressure = AdvantechTest(m_deviceInfoList.value(0));
        demoPressure.Initialization();
        m_deviceInfoList[0] = demoPressure.getInfo();
        m_deviceSettings[demoPressure.m_deviceName] = m_deviceInfoList[0].getSettings();
        auto demoValves = AdvantechTest(m_deviceInfoList.value(1));
        demoValves.Initialization();
        m_deviceInfoList[1] = demoValves.getInfo();
        m_deviceSettings[demoValves.m_deviceName] = m_deviceInfoList[1].getSettings();
    }

    // try to send to qml GRAM keys
    // TO CREATE PROFILE combo box
    QMap<int, QString> m;
    for(auto s : jsonObject.keys()) m[jsonObject[s].toObject()["profileId"].toInt()] = s;
    m_profileNames = QStringList(m.values());
    m_profileJson = jsonObject.toVariantMap();
}

bool DataAcquisition::advantechDeviceCheck(QVariantMap& advantechDeviceMap) const{
    /*ADVANTECH Controller Initialization*/
    //QVariantMap advantechDevices;
    // FARPROC fn = GetProcAddress(DNL_Instance(), "AdxDaqNaviLibInitialize");
    // if(fn == NULL) return false; // работает

    static HMODULE instance = LoadLibrary(TEXT("biodaq.dll"));;
    
    if(instance == NULL) return false; 

    // try DaqCtrlBase_Create(Scenario type)  - SceInstantAi works but badly
    auto startCheckInstance = InstantDoCtrl::Create(); // does it work with every Adv controller?
    auto allSupportedDevices = startCheckInstance->getSupportedDevices();
    if (allSupportedDevices->getCount() == 0)
    {
        qDebug() << "No advantech devices connected";
        return false;
    }
    for(int i = 0; i < allSupportedDevices->getCount(); i++){
        DeviceTreeNode const &node = allSupportedDevices->getItem(i);
        qDebug("%d, %ls", node.DeviceNumber, node.Description);
        auto advantechDescription = QString::fromWCharArray(node.Description).split(',');
        //auto advantechName = advantechDescription[0]; // I use this because we split BoardID, might use later
        advantechDeviceMap.insert(advantechDescription.value(1),advantechDescription.value(0)); // подумай насчет номера в Map, тут только advantech номера
    }
    startCheckInstance->Dispose();
    allSupportedDevices->Dispose();
    return true;
    
    /*ADVANTECH Controller Initialization*/
    // use advantechDeviceMap to additional features like in Instant_AI
}

void DataAcquisition::setDeviceParameters(QString name, QVariantMap obj){
    QVariantMap deviceMap = m_deviceSettings[name].toMap();
    deviceMap["chChannelCount"] = obj["indexChannelCount"];
    deviceMap["chCannelStart"] = obj["indexChannelStart"];
    deviceMap["chValueRange"] = obj["indexValueRange"];
    m_deviceSettings[name].setValue(deviceMap);
}

void DataAcquisition::saveStartDevice(){
    for(ControllerInfo info : m_deviceInfoList){
        info.setSettings(m_deviceSettings[info.deviceName()].toMap());
        auto demo = AdvantechTest(info);
        demo.ConfigureDeviceTest();
        appendToControllerList(demo);
    }
}

void DataAcquisition::appendToControllerList(AdvantechTest& device){
    controllerList.append(&device);
}

void DataAcquisition::processEvents(){

}

void DataAcquisition::readDataFromDevice(ControllerInfo info){
    auto itr = std::find_if(controllerList.begin(), controllerList.end(), [&](AdvantechTest* someclass) { return someclass->m_deviceName == info.deviceName(); });
    if(itr != controllerList.end()) {
        (*itr)->someData = "NEW";
    }
}


QVariantMap DataAcquisition::profileJson() const
{
  return m_profileJson;
}

void DataAcquisition::update(QAbstractSeries *series)
{
    if (series) {
        QXYSeries *xySeries = static_cast<QXYSeries *>(series);
        m_index++;
        if (m_index > m_data.count() - 1)
            m_index = 0;

        QList<QPointF> points = m_data.at(m_index);
        // Use replace instead of clear + append, it's optimized for performance
        xySeries->replace(points);
    }
}

void DataAcquisition::generateData(int type, int rowCount, int colCount)
{
    // Remove previous data
    m_data.clear();

    // Append the new data depending on the type
    for (int i(0); i < rowCount; i++) {
        QList<QPointF> points;
        points.reserve(colCount);
        for (int j(0); j < colCount; j++) {
            qreal x(0);
            qreal y(0);
            switch (type) {
            case 0:
                // data with sin + random component
                y = qSin(M_PI / 50 * j) + 0.5 + QRandomGenerator::global()->generateDouble();
                x = j;
                break;
            case 1:
                // linear data
                x = j;
                y = (qreal) i / 10;
                break;
            default:
                // unknown, do nothing
                break;
            }
            points.append(QPointF(x, y));
        }
        m_data.append(points);
    }
}
