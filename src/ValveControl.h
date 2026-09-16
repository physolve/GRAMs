#pragma once

#include "controllers/AdvantechCtrl.h"
#include "controllers/IDoPort.h"
#include "controllers/RealDoPort.h"
#include "Initialize.h"

#include <memory>
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
    // Порт выходов клапанов. initDaqDO ставит плату USB-4750; демо-режим —
    // эхо записи (sim::SimValveEcho). Security и логика клапанов те же.
    void setDoPort(std::unique_ptr<IDoPort> port);
    void beginAction();
    void endAction();
    bool isActionInterrupted() const;
    bool setValveFromAction(bool state, const QString& name);
    // Подтверждение ФАКТА состояния клапана (V-02, REQ-082/084).
    //
    // setValveFromAction сообщает результат КОМАНДЫ: «Security не запретил и
    // Write вернул Success». Этого мало для перехода на турбомолекулярный
    // насос — открыть К179 на непрощавшемся форвакуумном тракте значит
    // испортить насос. confirmValve перечитывает состояние с платы.
    //
    // Ограничение: чтение DO — это регистр-защёлка выхода, не датчик положения
    // (подробности в AdvantechDO::refresh). Без сконфигурированной платы
    // возвращает false: недоказанное состояние считаем неподтверждённым.
    bool confirmValve(const QString& name, bool expected);
    // Кэш последней команды (без обращения к железу). known = false, если
    // имени нет в реестре.
    bool valveState(const QString& name, bool* known = nullptr) const;
    Q_INVOKABLE void setManualChamberValve(bool state);
    Q_INVOKABLE void setValveState(bool state, int valveId);
    bool sendValveStates();
    bool isControlRunning();
    void valveChangeUpdater(const QString& valveName, bool newState);
    QVariantMap getGuiValsValve() const;

signals:
    void guiValsValveChanged();

private:
    std::unique_ptr<IDoPort> m_doPort;   // nullptr, пока порт не поставлен
    bool valveController;
    bool actionInterrupted;
    // valve pointers
    QVector<Valve*> m_valves;
    Valve* m_chamberValve;
    Security* m_safeModule;

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