#include "ValveControl.h"

#include <QTime>

#include <algorithm>

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
    m_sources.clear();
    // valve objects
    const auto &readData = m_doPort->data();
    // if ok
    for(int i = 0; i < m_valves.count() && i < readData.count(); ++i){
        m_valves[i]->setState(readData[i]);
        valveNameList << m_valves[i]->m_name;
        // Открыт до запуска программы — команды на него не было.
        setSource(m_valves[i]->m_name, readData[i], ValveSource::Board);
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
    if(state && !prepareTransferOpen(valve->m_name)){
        m_lastRefusal = QStringLiteral("transfer_equilibrium");
        qCWarning(lcValves) << "setValveState" << valve->m_name
                            << "не открыт: клапан диапазона закрыть не удалось";
        emit guiValsValveChanged();
        return;
    }
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state, valveStates(), &m_lastRefusal);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
        qCWarning(lcValves) << "setValveState" << valve->m_name << "запись не прошла — откат к" << originalState;
    }
    else{
        setSource(valve->m_name, valve->getState(), ValveSource::Manual);
        if(!actionInterrupted)
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
        setSource(name, actual, ValveSource::Board);
        emit guiValsValveChanged();
    }
    // Клапаны диапазона сверяются с той же прочитанной маской: открытый на
    // плате без команды S4/R4 закрывается (Т2). Результат confirmValve — только
    // про запрошенный клапан (REQ-082), закрытие Security его не меняет.
    bool rangeMismatch = false;
    for(const QString& range : (m_safeModule ? m_safeModule->rangeValves() : QStringList())){
        const int i = valveNameList.indexOf(range);
        if(i == -1 || i == index || i >= readData.count() || m_valves[i]->getState() == readData[i])
            continue;
        qCWarning(lcValves) << "confirmValve" << name << ": на плате" << range << readData[i]
                            << ", модель" << m_valves[i]->getState() << "— модель приведена к плате";
        m_valves[i]->setState(readData[i]);
        setSource(range, readData[i], ValveSource::Board);
        rangeMismatch = true;
    }
    if(rangeMismatch)
        emit guiValsValveChanged();
    enforceSecurity(QStringLiteral("confirmValve"));
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

ValveSource ValveControl::valveSource(const QString& name) const{
    return m_sources.value(name, ValveSource::None);
}

QMap<QString, ValveSource> ValveControl::valveSources() const{
    QMap<QString, ValveSource> sources;
    for(const Valve* valve : m_valves)
        sources.insert(valve->m_name, valveSource(valve->m_name));
    return sources;
}

void ValveControl::setSource(const QString& name, bool open, ValveSource commandSource){
    const ValveSource source = open ? commandSource : ValveSource::None;
    if(valveSource(name) != source)
        qCDebug(lcValves) << "источник" << name << toString(valveSource(name)) << "→" << toString(source);
    m_sources.insert(name, source);
}

QVariantMap ValveControl::getGuiValsValve() const{
    QVariantMap valveState;
    for(auto valve : m_valves){
        valveState[valve->m_name] = valve->getState();
    }
    valveState[m_chamberValve->m_name] = m_chamberValve->getState();
    return valveState;
}

void ValveControl::enforceSecurity(const QString& origin){
    if(!valveController || !m_safeModule)
        return;
    QList<SecurityIssue> warnings;
    QList<SecurityIssue> closures = m_safeModule->rangeValveClosures(valveStates(), valveSources(),
                                                                     m_regimeActive, origin, &warnings);
    // Прогноз равновесия — для клапанов, которые давление само не закрыло.
    for(const SecurityIssue& issue : m_safeModule->transferClosures(valveStates(), origin, &warnings)){
        const bool already = std::any_of(closures.cbegin(), closures.cend(),
                                         [&](const SecurityIssue& c){ return c.valve == issue.valve; });
        if(!already)
            closures << issue;
    }
    for(const SecurityIssue& warning : warnings){
        setSecurityMessage(warning);
        emit securityWarning(warning);
    }
    bool changed = false;
    for(const SecurityIssue& issue : closures)
        changed = closeBySecurity(issue) || changed;
    if(changed)
        emit guiValsValveChanged();
}

bool ValveControl::closeBySecurity(const SecurityIssue& issue){
    const int index = valveNameList.indexOf(issue.valve);
    if(index == -1)
        return false;
    Valve* valve = m_valves[index];
    // Уже закрыт — например, вложенным закрытием по концу режима, который
    // завершился из обработчика предыдущего закрытия.
    if(!valve->getState())
        return false;
    const ValveSource previous = valveSource(issue.valve);
    valve->setState(false);
    if(!sendValveStates()){
        // Клапан физически открыт: модель остаётся «открыт», попытка — на
        // следующем такте. Сообщаем один раз, а не каждые 500 мс.
        valve->setState(true);
        if(!m_securityCloseFailed.contains(issue.valve)){
            m_securityCloseFailed.insert(issue.valve);
            qCCritical(lcSecurity) << "Security: НЕ удалось закрыть" << issue.toString()
                                   << "— запись в порт не прошла, повтор на следующем такте";
        }
        return false;
    }
    m_securityCloseFailed.remove(issue.valve);
    setSource(issue.valve, false, ValveSource::None);
    qCWarning(lcSecurity) << "Security закрыл" << issue.toString() << "источник" << toString(previous)
                          << "обнаружено:" << issue.origin;
    setSecurityMessage(issue);
    emit securityClosed(issue, previous);
    return true;
}

void ValveControl::setSecurityMessage(const SecurityIssue& issue){
    auto bar = [](double v){ return QString::number(v, 'f', 2); };
    const QString time = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    QString text;
    if(issue.reason == u"pressure_range_autoclose")
        text = QStringLiteral("Security закрыл %1: давление %2 = %3 бар выше порога %4 бар")
                   .arg(issue.valve, issue.sensor, bar(issue.pressure), bar(issue.limit));
    else if(issue.reason == u"transfer_equilibrium")
        text = QStringLiteral("Security закрыл %1: давление равновесия при перепуске %2 бар выше порога %3 бар (%4)")
                   .arg(issue.valve, bar(issue.pressure), bar(issue.limit), issue.detail);
    else if(issue.reason == u"unauthorized_open"){
        const QString where = issue.origin == u"startup"      ? QStringLiteral("открыт на плате при старте")
                            : issue.origin == u"confirmValve" ? QStringLiteral("открыт на плате без команды")
                            : issue.origin == u"regime_end"   ? QStringLiteral("режим, открывший его, завершён")
                                                              : QStringLiteral("открыт без команды оператора или режима");
        text = QStringLiteral("Security закрыл %1: %2").arg(issue.valve, where);
    }
    else if(issue.reason == u"pressure_invalid")
        text = QStringLiteral("%1 открыт, давление недостоверно (%2) — клапан не закрыт")
                   .arg(issue.valve, issue.sensor.isEmpty() ? issue.detail : issue.sensor);
    else
        text = issue.toString();
    m_securityMessage = time + u"  " + text;
    emit securityMessageChanged();
}

void ValveControl::setRegimeActive(bool active){
    if(m_regimeActive == active)
        return;
    m_regimeActive = active;
    qCDebug(lcValves) << "режим" << (active ? "идёт" : "не идёт");
    if(!active)
        enforceSecurity(QStringLiteral("regime_end"));
}

bool ValveControl::prepareTransferOpen(const QString& name){
    if(!m_safeModule || !m_safeModule->isTransferValve(name))
        return true;
    QMap<QString, bool> image = valveStates();
    if(image.value(name))
        return true;           // уже открыт — прогноз ведёт такт softEvent
    image[name] = true;
    QList<SecurityIssue> warnings;
    const QList<SecurityIssue> closures = m_safeModule->transferClosures(image, QStringLiteral("command"), &warnings);
    for(const SecurityIssue& warning : warnings)
        emit securityWarning(warning);
    bool ok = true;
    for(const SecurityIssue& issue : closures)
        ok = closeBySecurity(issue) && ok;
    if(!closures.isEmpty())
        emit guiValsValveChanged();
    return ok;
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
    if(state && !prepareTransferOpen(name)){
        m_lastRefusal = QStringLiteral("transfer_equilibrium");
        qCWarning(lcValves) << "setValveFromAction" << name
                            << "не открыт: клапан диапазона закрыть не удалось";
        emit guiValsValveChanged();
        return false;
    }
    // check pressure!!!
    bool safe_state = m_safeModule->checkValveAction(valve->m_name, state, valveStates(), &m_lastRefusal);
    valve->setState(safe_state);
    if(!sendValveStates()){
        valve->setState(originalState);
        qCWarning(lcValves) << "setValveFromAction" << name << "запись не прошла — откат к" << originalState;
    }
    else
        setSource(name, valve->getState(), ValveSource::Regime);
    qCDebug(lcValves) << "setValveFromAction" << name << "итог" << valve->getState();
    emit guiValsValveChanged();
    valveChangeUpdater(name, state);
    return valve->getState() == state;
}