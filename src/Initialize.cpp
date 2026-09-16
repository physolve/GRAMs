#include "Initialize.h"

#include <QDir>
#include <QDebug>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

#include "controllers/AdvantechCtrl.h"
#include "controllers/SerialInfo.h"

// #include "lib/bdaqctrl.h"
// using namespace Automation::BDaq;

Initialize::Initialize(QObject *parent, const QString &curInitProfile) :
    QObject(parent), m_curInitProfile(curInitProfile), initializeOk(false)
{
    // сначала определяешь все, что подключено
    // а потом сверяешь с тем, что в профиле

    // fill the Map using the same properties as name and profile
    // like: current device : [{name_controller},{}]
    // later compare the maps to approve working state
    bool checkPass = true;
    QString m_rawData;
    checkPass = checkPass&&readProfile(m_rawData);
    QJsonObject profileJson;
    checkPass = checkPass&&jsonParser(m_rawData, profileJson);
    
    //device map 
    //  -_ Advantech device map
    //if(profile advantech!!!)
    QStringList advantechDeviceNames;
    // Без biodaq.dll SDK вызывает функцию по нулевому адресу — сначала
    // проверяем, что библиотека вообще загружается.
    m_biodaqAvailable = biodaqAvailable();
    if(!m_biodaqAvailable)
        qWarning() << "biodaq.dll не найдена — платы Advantech недоступны";
    checkPass = checkPass*(m_biodaqAvailable && AdvantechCtrl::advantechDeviceCheck(advantechDeviceNames));
    QStringList serialNames;
    checkPass = checkPass*SerialInfo::serialPortsInfo(serialNames);
    visualRepresentation(profileJson); // setted after gui run
    detectHardware(advantechDeviceNames, serialNames);   // после разбора профиля: нужны порты
    bool initAdvantech = false, initVacuum = false;
    if(checkPass){
        initAdvantech = advantechCompareProfile(advantechDeviceNames);
        initVacuum = serialCompareProfile(serialNames);
    }
    else qDebug() << "checkPass problem";
    initializeOk = initAdvantech*initVacuum;
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

void Initialize::visualRepresentation(const QJsonObject &profileJson){
    // knows profile
    const auto &profileObject = profileJson[m_curInitProfile].toObject();
    
//hardware
    const auto &hardwareObject = profileObject["stuff"].toObject(); 
    const auto &valves = hardwareObject["valveMap"].toVariant().toStringList();
    const auto &pressureSensorsArray = hardwareObject["pressureSensors"].toArray();
    // m_hardware.m_pressureSensors.empty
    QList<PressureSensor> pressureSensors;
    for(const auto &value : pressureSensorsArray){
        PressureSensor pressureSensor;
        const auto &obj = value.toObject();
        pressureSensor.m_sensorName = obj["name"].toString();
        pressureSensor.m_cch = obj["cch"].toInt();
        pressureSensor.m_A = obj["A"].toDouble();
        pressureSensor.m_B = obj["B"].toDouble();
        pressureSensor.m_R = obj["R"].toDouble();
        pressureSensors << pressureSensor;
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
    // Advantech
    const auto &advantechArray = controllersObject["Advantech"].toArray();
    auto temp_daq = QList<daqParameters>();
    for(const auto &value : advantechArray){
        const auto &obj = value.toObject();
        const auto &device = obj["device"].toString();
        const auto &purpose = obj["purpose"].toString();
        const auto &profile = obj.contains("profile") ? obj["profile"].toString() : "";
        const auto &defaultType = obj.contains("defaultType") ? obj["defaultType"].toInt() : 0;
        temp_daq << daqParameters{device, purpose, profile, defaultType, false}; 
    }
    m_daq = temp_daq;
    // vacuum
    const auto &vacuumObject = controllersObject["Vacuum"].toObject();
    const auto& portName = vacuumObject["portName"].toString();
    const auto& description = vacuumObject["description"].toString();
    const auto& baudRate = vacuumObject["baudRate"].toInt();
    const auto& dataBits = vacuumObject["dataBits"].toInt();
    const auto& stopBits = vacuumObject["stopBits"].toInt();
    const auto& parity = vacuumObject["parity"].toInt();
    const auto& timeout = vacuumObject["timeout"].toInt();
    m_vacuum = vacuumParameters{portName, description, baudRate, dataBits, stopBits, parity, timeout};

    // ДВ302 (второй тракт, турбо). Секция опциональна: на стенде без второго
    // вакуумметра приложение обязано стартовать как раньше. Параметры обмена
    // по умолчанию берём от ДВ301 — приборы однотипные.
    const auto &turboObject = controllersObject["VacuumTurbo"].toObject();
    if(!turboObject.isEmpty()){
        m_vacuumTurbo = vacuumParameters{
            turboObject["portName"].toString(),
            turboObject["description"].toString(),
            turboObject.contains("baudRate") ? turboObject["baudRate"].toInt() : baudRate,
            turboObject.contains("dataBits") ? turboObject["dataBits"].toInt() : dataBits,
            turboObject.contains("stopBits") ? turboObject["stopBits"].toInt() : stopBits,
            turboObject.contains("parity")   ? turboObject["parity"].toInt()   : parity,
            turboObject.contains("timeout")  ? turboObject["timeout"].toInt()  : timeout};
    } 
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

    // Пороги безопасности вакуумного тракта. Секция опциональна: если её нет,
    // остаются ориентиры ТЗ из значений по умолчанию структуры. Каждый ключ
    // читается через contains(), иначе отсутствие ключа молча дало бы 0 —
    // а нулевой порог перехода на турбонасос недостижим и подвесил бы этап.
    const auto &vacuumSafetyObject = profileObject["vacuumSafety"].toObject();
    if(!vacuumSafetyObject.isEmpty()){
        vacuumSafetyParameters vs;
        if(vacuumSafetyObject.contains("turboSwitchPressurePa"))
            vs.m_turboSwitchPressurePa = vacuumSafetyObject["turboSwitchPressurePa"].toDouble();
        if(vacuumSafetyObject.contains("turboSwitchHoldSec"))
            vs.m_turboSwitchHoldSec = vacuumSafetyObject["turboSwitchHoldSec"].toInt();
        if(vacuumSafetyObject.contains("turboReturnPressurePa"))
            vs.m_turboReturnPressurePa = vacuumSafetyObject["turboReturnPressurePa"].toDouble();
        if(vacuumSafetyObject.contains("turboTimeoutSec"))
            vs.m_turboTimeoutSec = vacuumSafetyObject["turboTimeoutSec"].toInt();
        if(vacuumSafetyObject.contains("overrangeWaitSec"))
            vs.m_overrangeWaitSec = vacuumSafetyObject["overrangeWaitSec"].toInt();
        if(vacuumSafetyObject.contains("overrangeWaitSec2"))
            vs.m_overrangeWaitSec2 = vacuumSafetyObject["overrangeWaitSec2"].toInt();
        if(vacuumSafetyObject.contains("valveReadbackTimeoutMs"))
            vs.m_valveReadbackTimeoutMs = vacuumSafetyObject["valveReadbackTimeoutMs"].toInt();
        if(vacuumSafetyObject.contains("testEvacTimeSec"))
            vs.m_testEvacTimeSec = vacuumSafetyObject["testEvacTimeSec"].toInt();
        if(vacuumSafetyObject.contains("leakTestDurationSec"))
            vs.m_leakTestDurationSec = vacuumSafetyObject["leakTestDurationSec"].toInt();
        // Порог герметичности читается поканально и БЕЗ значения по умолчанию:
        // выдумать его нельзя, а отсутствие обязано выключить этап 11.8,
        // а не пропустить его как «пройденный» (REQ-060/062).
        if(vacuumSafetyObject.contains("dP_leak_max"))
            vs.m_dPLeakMax = vacuumSafetyObject["dP_leak_max"].toObject().toVariantMap();
        m_vacuumSafety = vs;
    }

    // Наборы клапанов этапов 11.7б/11.8/11.9/11.10 (REQ-055/059/064/066).
    // Секции нет ⇒ списки пусты ⇒ соответствующие этапы пропускаются с причиной.
    const auto &vacuumTractObject = profileObject["vacuumTract"].toObject();
    if(!vacuumTractObject.isEmpty()){
        vacuumTractParameters vt;
        vt.m_generalPumping = vacuumTractObject["generalPumping"].toVariant().toStringList();
        vt.m_leakTest       = vacuumTractObject["leakTest"].toVariant().toStringList();
        vt.m_finalPumping   = vacuumTractObject["finalPumping"].toVariant().toStringList();
        m_vacuumTract = vt;
    }
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
bool Initialize::advantechCompareProfile(const QStringList& advantechDeviceNames){
    // real to profile
    QStringList unrecognizedControllers;
    int recognizedCnt = 0;
    for(const auto& val : advantechDeviceNames){
        qDebug() << val;
        bool recognized = false;
        for(auto &profile : m_daq){
            if(profile.m_device != val.split(',').value(0, "")) 
                continue;
            recognized = profile.m_state = true;
            profile.fullName = val;
            recognizedCnt++;
        }
        if(!recognized)
            unrecognizedControllers << val;
    }
    if(!unrecognizedControllers.isEmpty())
        qDebug() << "Unrecognized controllers " << unrecognizedControllers;
    // which found navi blue, which unexpected - yellow
    return recognizedCnt >= m_daq.length();
}

void Initialize::getParametersDO(daqParameters &params){
    for(auto &profile : m_daq){
        if(profile.m_device == "USB-4750"&&profile.m_state){
            params = profile;
            return;
        } // from profile
    }
}

void Initialize::getParametersAIpres(daqParameters &params){
    for(auto &profile : m_daq){
        if(profile.m_device == "USB-4716"&&profile.m_state){
            params = profile;
            return;
        } // from profile
    }
}

void Initialize::getParametersAItemp(daqParameters &params){
    for(auto &profile : m_daq){
        if(profile.m_device == "USB-4718"&&profile.m_state){
            params = profile;
            return;
        } // from profile
    }
}

vacuumParameters Initialize::getVacuumParameters() const{
    return m_vacuum;
}

QList<PressureSensor> Initialize::getPressureSensors() const{
    return m_hardware.m_pressureSensors;
}

QStringList Initialize::getTempSensors() const{
    return m_hardware.m_tempSensors;
}

bool Initialize::serialCompareProfile(const QStringList& serialNames){
    // real to profile
    qDebug() << serialNames;
    bool foundVacuum = false;
    m_vacuumTurboFound = false;
    for(const auto& serial : serialNames){
        // Формат SerialInfo::serialPortsInfo — "<description>, <portName>".
        // Описание само может содержать ", ", поэтому режем по последней запятой.
        const int sep = serial.lastIndexOf(", ");
        if(sep < 0)
            continue;
        const QString description = serial.left(sep);
        const QString portName    = serial.mid(sep + 2);
        if(m_vacuum.m_description == description && m_vacuum.m_portName == portName){
            qDebug() << "Found Vacuum (ДВ301) on" << portName;
            foundVacuum = true;
        }
        if(!m_vacuumTurbo.m_portName.isEmpty()
           && m_vacuumTurbo.m_description == description
           && m_vacuumTurbo.m_portName == portName){
            qDebug() << "Found VacuumTurbo (ДВ302) on" << portName;
            m_vacuumTurboFound = true;
        }
    }
    if(!m_vacuumTurbo.m_portName.isEmpty() && !m_vacuumTurboFound)
        qWarning() << "ДВ302 описан в профиле, но порт" << m_vacuumTurbo.m_portName
                   << "не найден — перехода на турбомолекулярный насос не будет";
    // Результат определяет ТОЛЬКО ДВ301: отсутствие опционального ДВ302
    // не должно блокировать запуск приложения.
    return foundVacuum;
}

vacuumSafetyParameters Initialize::getVacuumSafetyParameters() const{
    return m_vacuumSafety;
}

vacuumTractParameters Initialize::getVacuumTractParameters() const{
    return m_vacuumTract;
}

vacuumParameters Initialize::getVacuumTurboParameters() const{
    return m_vacuumTurbo;
}

bool Initialize::hasVacuumTurbo() const{
    return m_vacuumTurboFound;
}

bool Initialize::biodaqAvailable(){
#if defined(_WIN32)
    // Та же загрузка, что делает bdaqctrl.h (DNL_Instance); модуль остаётся
    // загруженным и дальше используется SDK.
    return LoadLibraryW(L"biodaq.dll") != nullptr;
#else
    return true;   // на Linux biodaq линкуется, без неё приложение не соберётся
#endif
}

// Железо считается подключённым, если видна плата Advantech из профиля или
// порт любого вакуумметра из профиля. Это строже, чем !isInitializeOk():
// при живой плате клапанов и отсутствующем COM-порте демо-режим недопустим.
void Initialize::detectHardware(const QStringList& advantechDeviceNames, const QStringList& serialNames){
    QStringList found;
    // Платы — только модели из профиля (USB-4716/4718/4750). Виртуальные
    // DemoDevice из DAQNavi ничем не управляют и демо-режим не запрещают.
    for(const auto& name : advantechDeviceNames){
        const QString model = name.split(',').value(0);
        for(const auto& daq : m_daq){
            if(daq.m_device == model){
                found << name;
                break;
            }
        }
    }
    for(const auto& serial : serialNames){
        const int sep = serial.lastIndexOf(", ");
        if(sep < 0)
            continue;
        const QString description = serial.left(sep);
        const QString portName    = serial.mid(sep + 2);
        for(const auto* gauge : {&m_vacuum, &m_vacuumTurbo}){
            if(!gauge->m_portName.isEmpty() && gauge->m_portName == portName
               && gauge->m_description == description)
                found << QStringLiteral("%1 (%2)").arg(portName, description);
        }
    }
    m_detectedHardware = found;
}

bool Initialize::hardwareDetected() const{
    return !m_detectedHardware.isEmpty();
}

QString Initialize::hardwareDetectedReason() const{
    if(m_detectedHardware.isEmpty())
        return {};
    return QStringLiteral("подключено железо: %1").arg(m_detectedHardware.join(", "));
}


bool Initialize::isInitializeOk() const{
    return initializeOk;
}
// function to GUI representation
// local channelmapping

// create controller info for next constructor


// I don't like this function move settings for controllers later
// QVariantMap Initialize::advantechDeviceFill(const QString &description, const QString &type){ 
//     QVariantMap advantechDeviceSettings;
//     if(type == "valves"){
//         advantechDeviceSettings["blank"] = "null";
//     }
//     else{
//         AdvAIType a(description);
//         auto demoPressure = AdvantechBuff(a); //AdvantechAI(a); 
//         demoPressure.Initialization();
//         a = demoPressure.getInfo();
//         advantechDeviceSettings = a.getSettings();
//     }
//     return advantechDeviceSettings;
// }

