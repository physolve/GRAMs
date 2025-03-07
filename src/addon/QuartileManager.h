#pragma once

#include <QVariant>
#include <QJsonObject>

#include "Quartile.h"

// Quartile object is used to store parameters from one of four volumes
class QuartileManager : public QObject
{
    Q_OBJECT
public:
    QuartileManager(QObject *parent = nullptr);
    ~QuartileManager();
    void readAddons();
    void parseAddons();
    void fillAddRemoveQuartile(AddRemoveQuartile& addRemoveQuartile);
    void fillStorageQuartile(StorageQuartile& storageQuartile);
    void fillReactionQuartile(ReactionQuartile& reactionQuartile);
    void fillSecondLineQuartile(SecondLineQuartile& secondLineQuartile);
private:
    QString addonsProfile;
    QJsonObject profileJson;

};