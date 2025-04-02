#include "QuartileManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariantList>

QuartileManager::QuartileManager(QObject *parent){
    readAddons();
    parseAddons();
}

QuartileManager::~QuartileManager(){

}


void QuartileManager::readAddons(){
    QDir dir("profile");
    if(!dir.exists()) return;
    QFile file;
    file.setFileName(dir.filePath("addons.json"));
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    addonsProfile = file.readAll();
    file.close();
}

void QuartileManager::parseAddons(){
    QJsonDocument document = { QJsonDocument::fromJson(addonsProfile.toUtf8()) };
    profileJson = document.object();
}

void QuartileManager::fillAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile){
    QJsonObject addRemoveQuar = profileJson["addRemoveQuar"].toObject();
    addRemoveQuartile->setVolume(addRemoveQuar["volume"].toDouble());
    // flow coefficient parameters
    // inlet pressure paramters
    // supply speed [3]
    // drain speed [2]
}

QStringList QuartileManager::fillStorageQuartile(StorageQuartile* storageQuartile){
    QJsonObject storageQuar = profileJson["storageQuar"].toObject();
    QString mainVolume = storageQuar["mainVolume"].toString();
    storageQuartile->setMainVolume(mainVolume);
    QVariantMap volumes = storageQuar["volume"].toObject().toVariantMap();
    QStringList volumeNames;
    for(const auto& [key,value] : volumes.asKeyValueRange()){
        storageQuartile->addVolume(key,value.toDouble());
        if(key == mainVolume) volumeNames.prepend(key);
        else volumeNames << key;
    }
    storageQuartile->calculateTotalVolume();
    QVariantMap volumeToValve = storageQuar["volumeToValve"].toObject().toVariantMap();
    QMap<QString,QString> volumeToValveMap;
    for(const auto& [key,value] : volumeToValve.asKeyValueRange()){
        volumeToValveMap[key] = value.toString();
    }
    storageQuartile->fillVolumePairs(volumeToValveMap);
    return volumeNames; // make order!
}


QStringList QuartileManager::fillReactionQuartile(ReactionQuartile* reactionQuartile){
    QJsonObject reactionQuar = profileJson["reactionQuar"].toObject();
    QString mainVolume = reactionQuar["mainVolume"].toString();
    reactionQuartile->setMainVolume(mainVolume);
    QVariantMap volumes = reactionQuar["volume"].toObject().toVariantMap();
    QStringList volumeNames;
    for(const auto& [key,value] : volumes.asKeyValueRange()){
        reactionQuartile->addVolume(key,value.toDouble());
        if(key == mainVolume) volumeNames.prepend(key);
        else volumeNames << key;
    }
    reactionQuartile->calculateTotalVolume();

    QVariantMap volumeToValve = reactionQuar["volumeToValve"].toObject().toVariantMap();
    QMap<QString,QString> volumeToValveMap;
    for(const auto& [key,value] : volumeToValve.asKeyValueRange()){
        volumeToValveMap[key] = value.toString();
    }
    reactionQuartile->fillVolumePairs(volumeToValveMap);
    reactionQuartile->setChamber(reactionQuar["chamber"].toString());
    return volumeNames; // make order!
}

void QuartileManager::fillSecondLineQuartile(SecondLineQuartile* secondLineQuartile){
    QJsonObject secondLineQuar = profileJson["secondLineQuar"].toObject();
    secondLineQuartile->setVolume(secondLineQuar["volume"].toDouble());
}
