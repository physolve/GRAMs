#pragma once

#include "controllers/AdvantechCtrl.h"
#include "Initialize.h" 
#include "addon/AddRemoveQuartile.h"
#include "addon/ReactionQuartile.h"
#include "db/GramStateDB.h"
#include "DataCollection.h"
#include "ValveModel.h" // Vlave object
#include "Security.h"

class ValveControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QVariantMap guiValve READ getGuiValsValve NOTIFY guiValsValveChanged)
public:
    ValveControl(QObject *parent = 0);
    ~ValveControl();
    void setValvePointers(const QVector<Valve*>& ptr);
    void setChamberValvePointer(Valve* chamberValve);
    void setAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile);
    // void setStorageQuartile(StorageQuartile* storageQuartile);
    void setReactionQuartile(ReactionQuartile* reactionQuartile);
    void setDatabase(GramStateDB* gramStateDB);
    void setSafeModule(Security* safeModule);
    void setSafeModuleInitialValveState();
    void setGasSupplyValves(const QStringList& gasSupplyValves);
    void setGasStoreValves(const QStringList& gasStoreValves);
    void initDaqDO(const daqParameters &parameterDO); 
    void beginAction();
    void endAction();
    bool isActionInterrupted() const;
    bool setValveFromAction(bool state, const QString& name);
    Q_INVOKABLE void setManualChamberValve(bool state);
    Q_INVOKABLE void setValveState(bool state, int valveId);
    bool sendValveStates();
    bool isControlRunning();
    void valveChangeUpdater(const QString& valveName);

signals:
    void guiValsValveChanged();

private:
    AdvantechDO reqValveDO;
    bool valveController;
    bool actionInterrupted;
    // valve pointers
    QVector<Valve*> m_valves;
    Valve* m_chamberValve;
    Security* m_safeModule;

    QVariantMap getGuiValsValve() const;
    QStringList valveNameList;
    QStringList m_gasSupplyValves;
    QStringList m_gasStoreValves;
    // add remove pointer
    AddRemoveQuartile* m_addRemoveQuartile;
    // storage pointer
    // reaction pointer
    ReactionQuartile* m_reactionQuartile;
    // database pointer
    GramStateDB* m_gramStateDB;
    bool checkOpenChamber(bool state);
};