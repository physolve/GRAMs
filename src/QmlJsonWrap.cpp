#include "QmlJsonWrap.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

MyData::MyData()
{
  //toDo...
}

bool MyData::fileExists(QString path) {
  QFileInfo check_file(path);
  // check if file exists and if yes, Is it really a file!
  if (check_file.exists() && check_file.isFile()) {
      return true;
    } else {
      return false;
    }
}

QVariantList MyData::data() const
{
  return m_data;
}

void MyData::setData(const QVariantList& data)
{
  if (m_data == data)
    return;
  m_data = data;
  emit dataChanged(m_data);
}

int MyData::length() const
{
  return m_length;
}

void MyData::setLength(int length)
{
  if (m_length == length)
    return;
  m_length = length;
  emit lengthChanged(m_length);
}

QVariantList MyData::cfg() const
{
  return m_startCfg;
}

void MyData::setStartCfg(const QVariantList& startCfg)
{
  if (m_startCfg == startCfg)
    return;
  m_startCfg = startCfg;
  qDebug() << "c++ moment " << m_startCfg;
  emit startCfgChanged(m_startCfg);
}

int MyData::lengthCfg() const
{
  return m_cfgLength;
}

void MyData::setCfgLength(int cfgLength)
{
  if (m_cfgLength == cfgLength)
    return;
  m_cfgLength = cfgLength;
  emit lengthCfgChanged(m_cfgLength);
}

bool MyData::result() const
{
  return m_result;
}

void MyData::setResult(bool result)
{
  if (m_result == result)
    return;

  m_result = result;
  emit resultChanged(m_result);
}

void MyData::parse(QString path) {

    QString rawData;
    QVariantMap modelData;
    QVariantList finalJson;

    QVariantMap modelCfg;
    QVariantList finalCfg;

    QFile file;
    QDir dir(".");

    if(fileExists(path)) {
        {
        file.setFileName(path);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        //Load data from json file!
        rawData = file.readAll();

        file.close();

        // Create json document.
        // Parses json as a UTF-8 encoded JSON document, and creates a QJsonDocument from it.

        QJsonDocument document   =   { QJsonDocument::fromJson(rawData.toUtf8()) };

        QJsonObject jsonObject = document.object();

        // Sets number of items in the list as integer.
        setLength(jsonObject["model"].toArray().count());

        foreach (const QJsonValue &value, jsonObject["model"].toArray()) {

            // Sets value from model as Json object
            QJsonObject modelObject = value.toObject();

            modelData.insert("id", modelObject["id"].toInt());
            modelData.insert("name", modelObject["name"].toString());
            modelData.insert("family", modelObject["family"].toString());

            // Set model data
            finalJson.append(modelData);
        }

        setCfgLength(jsonObject["startCfg"].toArray().count());
        
        foreach (const QJsonValue &value, jsonObject["startCfg"].toArray()) {

            // Sets value from model as Json object
            QJsonObject modelObject = value.toObject();
            
            modelCfg.insert("btnId", modelObject["btnId"].toInt());
            modelCfg.insert("start", modelObject["start"].toBool());
            // Set model data
            finalCfg.append(modelCfg);
        }

        // Sets data
        setData(finalJson);

        // Sets startCfg
        setStartCfg(finalCfg); // empty but why
        
        // Sets result by status object of model.
        setResult(jsonObject["result"].toBool());

        }

    } else {
        qWarning() << "There is no any file in this path!";
    }
}

bool MyData::saveJson(QVariantMap modelData) const
{
    QFile saveFile(QStringLiteral("save.json"));

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    //somehow use the startCfg object (upper), probably set update from qml
    // f.e. you might use setStartCfg(const QVariantList& startCfg) function
   
   
    //QJsonArray jsonArray = QJsonDocument::fromVariant(modelData.toUtf8()).array();

    //QJsonArray jsonCfgArray = QJsonDocument::fromJson(cfgData.toUtf8()).array();

    qDebug() << "Compare from qml " << modelData["startCfg"];
    qDebug() << "Compare from local " << m_startCfg;

    QJsonObject jsonObject = QJsonObject::fromVariantMap(modelData);
    //jsonObject["model"] = jsonArray;
    jsonObject["result"] = true;
    //jsonObject["startCfg"] = QJsonDocument::fromJson(m_startCfg).object();
    saveFile.write(QJsonDocument(jsonObject).toJson());

    return true;
}