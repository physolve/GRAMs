#include "ValveModel.h"


ValveModel::ValveModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

void ValveModel::appendValves(const QVariant &valves){
    m_valveNames = valves.toStringList();
    for(const auto &valveName : m_valveNames){
        //auto valve = Valve(name, false);
        //m_valves.append(&valve);
        m_valves.insert(valveName, Valve(valveName));
    }
}
// void ValveModel::getStates(){
//     for (auto i = m_valves.begin(); i != m_valves.end(); ++i){
//         qDebug()<< (*i)->m_name << (*i)->m_state;
//     } 
// }

QVariantMap ValveModel::getCurStates() const{
    QVariantMap curStates;
    for(auto name : m_valveNames){
        curStates[name] = m_valves[name].getState();
    }
    // use lambda instead for!!! 
    return curStates;
}

// QMap<QString, bool> ValveModel::securityValveMap(const QString &senderName, const bool &senderState){
//     auto cur_valveMap = controllerValveMap();
//     cur_valveMap[senderName] = senderState;
//     return cur_valveMap;
// }

QMap<QString, bool> ValveModel::getValveMap(){
    QMap<QString, bool> cur_valveMap;
    for(const auto &name : m_valveNames){ // I don't like it
        cur_valveMap[name] = m_valves[name].getState();
    }
    return cur_valveMap;
}

QVector<bool> ValveModel::getValveVector(){
    const int count = m_valveNames.count();
    QVector<bool> valveVector(16,false);
    for (int i = 0; i < count; ++i) {
        valveVector[i] = m_valves[m_valveNames.at(i)].getState();
    }
    // use lambda instead of count method!!!
    return valveVector;
}

int ValveModel::rowCount( const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_valves.count();
}

QVariant ValveModel::data(const QModelIndex &index, int role) const
{
    if ( !index.isValid() )
        return QVariant();

    auto valve = this->m_valves[m_valveNames.at(index.row())];
    if ( role == NameRole ){
        return valve.m_name;
    }
    else if ( role == State )
        return QVariant::fromValue(valve.m_state);
    else
        return QVariant();
}

//--> slide
QHash<int, QByteArray> ValveModel::roleNames() const
{
    static QHash<int, QByteArray> mapping {
        {NameRole, "name"},
        {State, "state"}
    };
    return mapping;
}


void ValveModel::appendData(const QVector<bool> & valveList){
    if(m_valves.isEmpty())
        return; // better to do smth
    const int count = valveList.count();//m_sensors.count(); //  m_sensors of THE controller
    auto value = false;
    for (int i = 0; i < count; ++i) {
        value = valveList.at(i);
        m_valves[m_valveNames.at(i)].setState(value);
        // use lambda instead of count method!!!
    }
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_valves.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << State);
}
void ValveModel::appendData(const QString & valveName, const bool &valveState){
    m_valves[valveName].setState(valveState);
    
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_valves.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << State);
}