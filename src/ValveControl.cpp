#include "ValveControl.h"

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

void ValveControl::setSafeModuleInitialValveState(){
    for(int i = 0; i < m_valves.count(); i++){
        m_safeModule->setInitialState(m_valves[i]->m_name, m_valves[i]->getState());
    }
}

void ValveControl::setGasSupplyValves(const QStringList& gasSupplyValves){
    m_gasSupplyValves = gasSupplyValves;
}

void ValveControl::setGasStoreValves(const QStringList& gasStoreValves){
    m_gasStoreValves = gasStoreValves;
}

void ValveControl::initDaqDO(const daqParameters &parameter){
    // pass real info from Initialize
    AdvDOType a(parameter.fullName);
    a.setProfilePath(parameter.m_profile);
    reqValveDO.setInfo(a);
    reqValveDO.ConfigureDeviceDO();
    reqValveDO.readData();
    // valve objects
    const auto &readData = reqValveDO.getData();
    // if ok
    for(int i = 0; i < m_valves.count(); ++i){
        m_valves[i]->setState(readData[i]);
        valveNameList << m_valves[i]->m_name;
    }
    valveController = true;
    emit guiValsValveChanged();
}

void ValveControl::setValveState(bool state, int index){ // excluding chamber
    // signal from GUI to change state of object
    Valve *valve = m_valves[index];
    const bool originalState = valve->getState();
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
    }
    else if(!actionInterrupted){
        actionInterrupted = true;
    }
    emit guiValsValveChanged();
}

bool ValveControl::sendValveStates(){
    if(!valveController)
        return false;
    QVector<bool> changedState;
    // if changedState > 8*portCount!
    for(int i = 0; i < m_valves.count(); ++i){
        changedState << m_valves[i]->getState();
    }
    // handler to unsuccessful set (true / false)
    return reqValveDO.setData(changedState);
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
    emit guiValsValveChanged();
}

bool ValveControl::checkOpenChamber(bool state){
    if(state){ // && safeModule->canOpenChamber;
    }
    return true;
}

void ValveControl::valveChangeUpdater(const QString& valveName){
    // from profile supply
    if(m_gasSupplyValves.contains(valveName)){
        m_addRemoveQuartile->updatePortState();
    }
    if(m_gasStoreValves.contains(valveName)){
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
        return false;
    }
    valve = m_valves[index];
    const bool originalState = valve->getState();
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
    }
    emit guiValsValveChanged();
    return valve->getState() == state;
}