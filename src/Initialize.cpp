#include "Initialize.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

#include "controllers/AdvantechCtrl.h"
#include "lib/bdaqctrl.h"
using namespace Automation::BDaq;

Initialize::Initialize(QObject *parent, const QString &curInitProfile) :
    QObject(parent), m_curInitProfile(curInitProfile)
{
    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    bool checkPass = false;
    QString m_rawData;
    checkPass = readProfile(m_rawData);
    QJsonObject profileJson;
    checkPass = jsonParser(m_rawData, profileJson);
    
    //device map 
    //  -_ Advantech device map
    //if(profile advantech!!!)
    checkPass = advantechDeviceCheck();

    visualRepresentation(profileJson); // setted after gui run

    advantechCompareProfile();
    //check

}

bool Initialize::readProfile(QString &rawData){
    QDir dir("profile");
    if(!dir.exists()) return false;
    QFile file;
    file.setFileName(dir.filePath("GRAMsPfp.json"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    rawData = file.readAll();
    file.close();
    return true;
}

bool Initialize::jsonParser(QString &rawData, QJsonObject &profileJson){
    QJsonDocument document = { QJsonDocument::fromJson(rawData.toUtf8()) };
    profileJson = document.object();
    QMap<int, QString> m;
    for(auto s : profileJson.keys()) // it is just sorting thing
        m[profileJson[s].toObject()["profileId"].toInt()] = s;
    
    m_profileNames = m.values();
    m_profileJson = profileJson.toVariantMap();
    return true;
}

bool Initialize::advantechDeviceCheck(){
    auto startCheckInstance = InstantAiCtrl::Create(); // does it work with every Adv controller?
    auto allSupportedDevices = startCheckInstance->getSupportedDevices();
    if (allSupportedDevices->getCount() == 0)
    {
        qDebug() << "No advantech devices connected";
        return false;
    }
    QStringList deviceMap;
    for(int i = 0; i < allSupportedDevices->getCount(); i++){
        DeviceTreeNode const &node = allSupportedDevices->getItem(i);
        qDebug("%d, %ls", node.DeviceNumber, node.Description);
        // auto advantechDescription = QString::fromWCharArray(node.Description).split(',');
        // auto tempName = advantechDescription.value(0);
        // auto tempBID = advantechDescription.value(1);

        // it doesn't fit expectations (it should show virtual BID to distinguish from non-virtual and put into BID)
        //qDebug() << "CHECK INDEX FOR BID to SHOW Virtual " << node.ModulesIndex;

        deviceMap << QString::fromWCharArray(node.Description);
    }
    m_advantechDeviceMap = deviceMap;
    startCheckInstance->Dispose();
    allSupportedDevices->Dispose();
    return true;
}


void Initialize::visualRepresentation(const QJsonObject &profileJson){
    // knows profile
    const auto &profileObject = profileJson[m_curInitProfile].toObject();
    
//hardware
    const auto &hardwareObject = profileObject["stuff"].toObject(); 
    const auto &valves = hardwareObject["valveMap"].toVariant().toStringList();
    const auto &pressureSensorsArray = hardwareObject["pressureSensors"].toArray();
    QMap<QString,QMap<QString, double>> pressureSensors;
    for(const auto &value : pressureSensorsArray){
        QMap<QString, double> pressureSensorsValues;
        const auto &obj = value.toObject();
        pressureSensorsValues["cch"] = obj["cch"].toInt(); 
        pressureSensorsValues["A"] = obj["A"].toDouble(); 
        pressureSensorsValues["B"] = obj["B"].toDouble(); 
        pressureSensorsValues["R"] = obj["R"].toDouble();
        pressureSensors[obj["name"].toString()] = pressureSensorsValues;
    }
    const auto &tempSensorsArray = hardwareObject["temperatureSensors"].toArray();
    QStringList tempSensors;
    for(const auto &value : tempSensorsArray){
        const auto &obj = value.toObject();
        tempSensors << obj["name"].toString();
    }
    m_hardware = hardwareParameters{valves, pressureSensors, tempSensors};
//hardware

//controllers
    const auto &controllersObject = profileObject["controllers"].toObject();
    const auto &advantechArray = controllersObject["Advantech"].toArray();
    auto temp_daq = QList<daqParameters>();
    for(const auto &value : advantechArray){
        const auto &obj = value.toObject();
        const auto &device = obj["device"].toString();
        const auto &purpose = obj["purpose"].toString();
        const auto &profile = obj.contains("profile") ? obj["purpose"].toString() : "";
        const auto &defaultType = obj.contains("defaultType") ? obj["defaultType"].toString() : "";
        temp_daq << daqParameters{device, purpose, profile, defaultType, false}; 
    }
    m_daq = temp_daq;
//controllers

// quartiles
    const auto &quartilesObject = profileObject["quartiles"].toObject();
    //addRemoveQuar
    const auto &addRemoveQuarObject = quartilesObject["addRemoveQuar"].toObject(); 
    fillAddRemoveQuar(addRemoveQuarObject);
    //storageQuar
    const auto &storageQuarObject = quartilesObject["storageQuar"].toObject();
    fillStorageQuar(storageQuarObject);
    //reactionQuar
    const auto &reactionQuarObject = quartilesObject["reactionQuar"].toObject();
    fillReactionQuar(reactionQuarObject);
    //secondLineQuar
    const auto &secondLineQuarObject = quartilesObject["secondLineQuar"].toObject();
    fillSecondLineQuar(secondLineQuarObject);
// quartiles
// security
    const auto &securityObject = profileObject["security"].toObject();
    const auto &contradictionValvesObject = securityObject["contradictionValves"].toObject(); 
    QMap<QString, QStringList>  contradictionValves;
    for(const auto& key : contradictionValvesObject.keys()){
        contradictionValves[key] = contradictionValvesObject[key].toVariant().toStringList();
    }
    const auto &twoOfThree = securityObject["twoOfThree"].toVariant().toStringList();
    const auto &safetyQuarsObject = securityObject["safetyQuars"].toObject(); 
    QMap<QString, QStringList>  safetyQuars;
    for(const auto& key : safetyQuarsObject.keys()){
        safetyQuars[key] = safetyQuarsObject[key].toVariant().toStringList();
    }
    m_security = securityParameters{contradictionValves, twoOfThree, safetyQuars};
// security
}

void Initialize::fillAddRemoveQuar(const QJsonObject &addRemoveQuarObject){
    const auto &gasSupplyValves = addRemoveQuarObject["v_gasSupply"].toVariant().toStringList();
    const auto &gasDrainValves = addRemoveQuarObject["v_gasDrain"].toVariant().toStringList();
    const auto &vacuumSensor = addRemoveQuarObject["m_vacuum"].toString();
    m_addRemoveQuar = addRemoveQuarParameters{gasSupplyValves, gasDrainValves, vacuumSensor};
    //addRemoveQuar
}

void Initialize::fillStorageQuar(const QJsonObject &storageQuarObject){
    const auto &gasStoreValves = storageQuarObject["v_gasStore"].toVariant().toStringList();
    const auto &gasReleaseValve = storageQuarObject["v_gasRelease"].toString();
    const auto &pressureRangeValve = storageQuarObject["v_pressureRange"].toString();
    const auto &highPressureSensors = storageQuarObject["s_highPressure"].toVariant().toStringList();
    const auto &lowPressureSensor = storageQuarObject["s_lowPressure"].toString();
    const auto &temperatureSensors = storageQuarObject["m_temperature"].toVariant().toStringList();
    const auto &pressureRange_close = storageQuarObject["cond_pressureRange_close"].toDouble();
    const auto &pressureRange_open = storageQuarObject["cond_pressureRange_open"].toDouble();
    const auto &gasRelease = storageQuarObject["cond_gasRelease"].toDouble();
    m_storageQuar = storageQuarParameters{gasStoreValves, gasReleaseValve, pressureRangeValve, highPressureSensors, lowPressureSensor,
    temperatureSensors, pressureRange_close, pressureRange_open, gasRelease};
    //storageQuar
}

void Initialize::fillReactionQuar(const QJsonObject &reactionQuarObject){
    const auto &gasLeakageValves = reactionQuarObject["v_gasLeakage"].toVariant().toStringList();
    const auto &pressureRangeValve = reactionQuarObject["v_pressureRange"].toString();
    const auto &highPressureSensor = reactionQuarObject["s_highPressure"].toString();
    const auto &lowPressureSensors = reactionQuarObject["s_lowPressure"].toVariant().toStringList();
    const auto &temperatureSensors = reactionQuarObject["m_temperature"].toVariant().toStringList();
    const auto &pressureRange_close = reactionQuarObject["cond_pressureRange_close"].toDouble();
    const auto &pressureRange_open = reactionQuarObject["cond_pressureRange_open"].toDouble();
    const auto &gasRelease = reactionQuarObject["cond_gasRelease"].toDouble();
    m_reactionQuar = reactionQuarParameters{gasLeakageValves, pressureRangeValve, highPressureSensor, lowPressureSensors,
    temperatureSensors, pressureRange_close, pressureRange_open, gasRelease};
    //reactionQuar
}

void Initialize::fillSecondLineQuar(const QJsonObject &secondLineQuarObject){
    const auto &gasDrainValve = secondLineQuarObject["v_gasDrain"].toString();
    const auto &mass_spectr = secondLineQuarObject["mass_spectr"].toString();
    //secondLineQuar
}

// compare profile and real
void Initialize::advantechCompareProfile(){
    // m_advantechDeviceMap compare to m_daq
    // real to profile
    QStringList unrecognizedControllers;
    for(const auto& val : m_advantechDeviceMap){
        qDebug() << val;
        bool recognized = false;
        for(auto &profile : m_daq){
            if(profile.m_device != val.split(',').value(0, "")) 
                continue;
            recognized = profile.m_state = true;
        }
        if(!recognized)
            unrecognizedControllers << val;
    }
    qDebug() << "Unrecognized controllers " << unrecognizedControllers;
    // which found navi blue, which unexpected - yellow
}

// function to GUI representation
// local channelmapping

// I don't like this function
QVariantMap Initialize::advantechDeviceFill(const QString &description, const QString &type){ 
    QVariantMap advantechDeviceSettings;
    if(type == "valves"){
        advantechDeviceSettings["blank"] = "null";
    }
    else{
        AdvAIType a(description);
        auto demoPressure = AdvantechBuff(a); //AdvantechAI(a); 
        demoPressure.Initialization();
        a = demoPressure.getInfo();
        advantechDeviceSettings = a.getSettings();
    }
    return advantechDeviceSettings;
}

