#pragma once

#include <QVariant>
#include <QJsonObject>

#include "Quartile.h"
#include "StorageQuartile.h"
#include "AddRemoveQuartile.h"
#include "ReactionQuartile.h"
#include "SecondLineQuartile.h"
#include "../Initialize.h"

// Quartile object is used to store parameters from one of four volumes
class QuartileManager : public QObject
{
    Q_OBJECT
public:
    QuartileManager(QObject *parent = nullptr);
    ~QuartileManager();

    void fillAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile);
    QStringList fillStorageQuartile(StorageQuartile* storageQuartile);
    QStringList fillReactionQuartile(ReactionQuartile* reactionQuartile);
    void fillSecondLineQuartile(SecondLineQuartile* secondLineQuartile);
private:
    QString addonsProfile;
    QJsonObject profileJson;
    void readAddons();
    void parseAddons();
};