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

Q_DECLARE_METATYPE(QAbstractSeries *)
Q_DECLARE_METATYPE(QAbstractAxis *)


DataAcquisition::DataAcquisition(QObject *parent) :
    QObject(parent),
    m_index(-1)
{
    qRegisterMetaType<QAbstractSeries*>();
    qRegisterMetaType<QAbstractAxis*>();

    generateData(0, 5, 1024);

    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    
    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    
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
    };
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
    
    if(instance == NULL) return false; // работает, но короче, проверь с драйвером

    auto startCheckInstance = InstantDoCtrl::Create();
    auto allSupportedDevices = startCheckInstance->getSupportedDevices();
    if (allSupportedDevices->getCount() == 0)
    {
        qDebug() << "No advantech devices connected";
        return false;
    }
    for(int i = 0; i < allSupportedDevices->getCount(); i++){
        DeviceTreeNode const &node = allSupportedDevices->getItem(i);
        qDebug("%d, %ls", node.DeviceNumber, node.Description);
        advantechDeviceMap.insert(QString::fromWCharArray(node.Description),(int)node.DeviceNumber); // подумай насчет номера в Map, тут только advantech номера
    }
    startCheckInstance->Dispose();
    allSupportedDevices->Dispose();
    return true;
    
    /*ADVANTECH Controller Initialization*/
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
