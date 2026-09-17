#include "ValveControl.h"

Q_LOGGING_CATEGORY(lcValves, "grams.valves")

ValveControl::ValveControl(QObject *parent) :
    QObject(parent)
{
    valveController = false;
    actionInterrupted = true;
}

ValveControl::~ValveControl(){
    valveController = false;
}

void ValveControl::setValvePointers(const QVector<Valve*>& ptr){
    m_valves = ptr;
}

void ValveControl::setChamberValvePointer(Valve* chamberValve){
    m_chamberValve = chamberValve;
}

void ValveControl::setAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile){
    m_addRemoveQuartile = addRemoveQuartile;
}

void ValveControl::setReactionQuartile(ReactionQuartile* reactionQuartile){
    m_reactionQuartile = reactionQuartile;
}

void ValveControl::setDatabase(GramStateDB* gramStateDB){
    m_gramStateDB = gramStateDB;
}

void ValveControl::setSafeModule(Security* safeModule){
    m_safeModule = safeModule;
}

void ValveControl::setGasSupplyValves(const QStringList& gasSupplyValves){
    m_gasSupplyValves = gasSupplyValves;
}

void ValveControl::setGasStoreValves(const QStringList& gasStoreValves){
    m_gasStoreValves = gasStoreValves;
}

void ValveControl::initDaqDO(const daqParameters &parameter){
    // pass real info from Initialize
    auto port = std::make_unique<RealDoPort>();
    AdvDOType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    port->device().setInfo(a);
    port->device().ConfigureDeviceDO();
    port->device().readData();
    setDoPort(std::move(port));
}

void ValveControl::setDoPort(std::unique_ptr<IDoPort> port){
    m_doPort = std::move(port);
    valveNameList.clear();
    // valve objects
    const auto &readData = m_doPort->data();
    // if ok
    for(int i = 0; i < m_valves.count() && i < readData.count(); ++i){
        m_valves[i]->setState(readData[i]);
        valveNameList << m_valves[i]->m_name;
    }
    qCDebug(lcValves) << "setDoPort: клапанов" << m_valves.count() << "битов порта" << readData.count()
                      << "valveNameList" << valveNameList.size() << "состояние" << readData;
    valveController = true;
    // compare to data base timestamp
    emit guiValsValveChanged();
}

void ValveControl::setValveState(bool state, int index){ // excluding chamber
    // signal from GUI to change state of object
    // Индекс приходит из QML. Без порта (нет платы и нет --sim) valveNameList
    // пуст: команду отклоняем здесь, а не падаем на valveNameList.at().
    if(index < 0 || index >= m_valves.count() || index >= valveNameList.size()){
        qCWarning(lcValves) << "setValveState: индекс" << index << "вне реестра (клапанов"
                            << m_valves.count() << ", порт" << (valveController ? "есть" : "нет")
                            << ") — отклонено";
        emit guiValsValveChanged();   // QML возвращается к фактическому состоянию
        return;
    }
    Valve *valve = m_valves[index];
    const bool originalState = valve->getState();
    qCDebug(lcValves) << "setValveState (GUI)" << valve->m_name << "индекс" << index
                      << "запрос" << state << "было" << originalState;
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state, valveStates(), &m_lastRefusal);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
        qCWarning(lcValves) << "setValveState" << valve->m_name << "запись не прошла — откат к" << originalState;
    }
    else if(!actionInterrupted){
        actionInterrupted = true;
    }
    qCDebug(lcValves) << "setValveState" << valve->m_name << "итог" << valve->getState()
                      << "valveNameList" << valveNameList.size();
    valveChangeUpdater(valveNameList.at(index), state);
    emit guiValsValveChanged();
}

bool ValveControl::sendValveStates(){
    if(!valveController){
        qCWarning(lcValves) << "sendValveStates: порт клапанов не поставлен — запись отклонена";
        return false;
    }
    QVector<bool> changedState;
    // if changedState > 8*portCount!
    for(int i = 0; i < m_valves.count(); ++i){
        changedState << m_valves[i]->getState();
    }
    // handler to unsuccessful set (true / false)
    const bool ok = m_doPort->write(changedState);
    qCDebug(lcValves) << "sendValveStates: маска" << changedState << "write" << ok;
    return ok;
}

// V-02 (REQ-082/084) — см. комментарий в заголовке.
bool ValveControl::confirmValve(const QString& name, bool expected){
    if(!valveController)
        return false;          // платы нет — состояние недоказуемо
    const int index = valveNameList.indexOf(name);
    if(index == -1)
        return false;
    if(!m_doPort->refresh())
        return false;          // чтение не удалось — не выдаём старые данные за факт
    const auto &readData = m_doPort->data();
    if(index >= readData.count())
        return false;
    const bool actual = readData[index];
    // Расхождение модели и платы — сигнал сам по себе: приводим модель к факту,
    // иначе интерфейс продолжит показывать желаемое вместо действительного.
    if(m_valves[index]->getState() != actual){
        qCWarning(lcValves) << "confirmValve" << name << "модель" << m_valves[index]->getState()
                            << "плата" << actual << "— модель приведена к плате";
        m_valves[index]->setState(actual);
        emit guiValsValveChanged();
    }
    return actual == expected;
}

bool ValveControl::valveState(const QString& name, bool* known) const{
    const int index = valveNameList.indexOf(name);
    if(index == -1){
        if(known) *known = false;
        return false;
    }
    if(known) *known = true;
    return m_valves[index]->getState();
}

QMap<QString, bool> ValveControl::valveStates() const{
    QMap<QString, bool> states;
    for(const Valve* valve : m_valves)
        states.insert(valve->m_name, valve->getState());
    return states;
}

QVariantMap ValveControl::getGuiValsValve() const{
    QVariantMap valveState;
    for(auto valve : m_valves){
        valveState[valve->m_name] = valve->getState();
    }
    valveState[m_chamberValve->m_name] = m_chamberValve->getState();
    return valveState;
}

void ValveControl::setManualChamberValve(bool state){
    if(!checkOpenChamber(state))
        return;
    m_chamberValve->setState(state);
    m_reactionQuartile->setChamberStatus(state);
    m_reactionQuartile->updateChamberToQuartile();
    // data base update
    // m_gramStateDB->writeTimeStamp();
    emit guiValsValveChanged();
}

bool ValveControl::checkOpenChamber(bool state){
    if(state){ // && safeModule->canOpenChamber;
    }
    return true;
}

void ValveControl::valveChangeUpdater(const QString& valveName, bool newState){
    // from profile supply
    // if(m_gasSupplyValves.contains(valveName)){
    //     m_addRemoveQuartile->updatePortState();
    // }
    if(false) { // m_gasStoreValves.contains(valveName) && newState == false
        if(m_gramStateDB->writeTimeStamp())
            qDebug() << "Time Stamp has been written";
        else 
            qDebug() << "Time Stamp has not been written";
    }
}

bool ValveControl::isControlRunning(){
    return valveController;
}

void ValveControl::beginAction(){
    actionInterrupted = false;
}

void ValveControl::endAction(){
    actionInterrupted = true;
}

bool ValveControl::isActionInterrupted() const{
    return actionInterrupted;
}

bool ValveControl::setValveFromAction(bool state, const QString& name){
    // signal from GUI to change state of object
    Valve *valve = nullptr;
    int index = valveNameList.indexOf(name);
    if(index == -1){
        qCWarning(lcValves) << "setValveFromAction" << name << "нет в valveNameList (размер"
                            << valveNameList.size() << ") — отклонено";
        return false;
    }
    valve = m_valves[index];
    const bool originalState = valve->getState();
    qCDebug(lcValves) << "setValveFromAction (режим)" << name << "запрос" << state << "было" << originalState;
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state, valveStates(), &m_lastRefusal);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
        qCWarning(lcValves) << "setValveFromAction" << name << "запись не прошла — откат к" << originalState;
    }
    qCDebug(lcValves) << "setValveFromAction" << name << "итог" << valve->getState();
    emit guiValsValveChanged();
    valveChangeUpdater(name, state);
    return valve->getState() == state;
}