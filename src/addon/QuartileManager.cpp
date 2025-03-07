#include "QuartileManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariantList>

QuartileManager::QuartileManager(QObject *parent){
    
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

void QuartileManager::fillAddRemoveQuartile(AddRemoveQuartile& addRemoveQuartile){
    QJsonObject addRemoveQuar = profileJson["addRemoveQuar"].toObject();
    addRemoveQuartile.setVolume(addRemoveQuar["volume"].toDouble());

}

void QuartileManager::fillStorageQuartile(StorageQuartile& storageQuartile){
    QJsonObject storageQuar = profileJson["storageQuar"].toObject();
    QVariantMap volumes = storageQuar["volume"].toObject().toVariantMap();
    for(const auto& [key,value] : volumes.asKeyValueRange()){
        storageQuartile.addVolume(key,value.toDouble());
    }
    storageQuartile.calculateTotalVolume();
}

void QuartileManager::fillReactionQuartile(ReactionQuartile& reactionQuartile){
    QJsonObject reactionQuar = profileJson["reactionQuar"].toObject();
    QVariantMap volumes = reactionQuar["volume"].toObject().toVariantMap();
    for(const auto& [key,value] : volumes.asKeyValueRange()){
        reactionQuartile.addVolume(key,value.toDouble());
    }
    reactionQuartile.calculateTotalVolume();
    reactionQuartile.setChamber(reactionQuar["chamber"].toString());
}

void QuartileManager::fillSecondLineQuartile(SecondLineQuartile& secondLineQuartile){
    QJsonObject secondLineQuar = profileJson["secondLineQuar"].toObject();
    secondLineQuartile.setVolume(secondLineQuar["volume"].toDouble());
}
