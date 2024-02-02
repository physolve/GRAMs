#include "ValveModel.h"


ValveModel::ValveModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

void ValveModel::appendValves(QVariant valves){
    QStringList names = valves.toStringList();
    for(auto name : names){
        auto valve = Valve(name, true);
        m_valves.append(&valve);
    }
}
void ValveModel::getStates(){
    for (auto i = m_valves.begin(); i != m_valves.end(); ++i){
        qDebug()<< (*i)->m_name << (*i)->m_state;
    } 
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

    auto valve = this->m_valves.at(index.row());
    if ( role == NameRole ){
        return valve->m_name;
    }
    else if ( role == State )
        return QVariant::fromValue(valve->m_state);
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

    const int count = valveList.count();//m_sensors.count(); //  m_sensors of THE controller
    auto value = false;
    for (int i = 0; i < count; ++i) {
        value = valveList.at(i);
        m_valves[i]->setState(value);
    }
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(m_valves.count() - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << State);
}