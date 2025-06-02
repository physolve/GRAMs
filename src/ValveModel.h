#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>

struct Valve{
    QString m_name = "";
    void setState(bool s){
        m_state = s;
    }
    bool getState() const{
       return m_state; 
    }
    Valve(const QString &name = "unknown", const bool &state = false): m_name(name), m_state(state) { }
private:
    bool m_state = false;
};

// unused 
class ValveModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole,
        State
    };

    explicit ValveModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    //bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override; // use QMultiHash

    Q_INVOKABLE void appendValves(const QVariant &valves);
    Q_INVOKABLE QVariantMap getCurStates() const; // for qml
    //void getStates();
    //QMap<QString, bool> securityValveMap(const QString &senderName, const bool &senderState);
    QMap<QString, bool> getValveMap();
    QVector<bool> getValveVector();
    void appendData(const QVector<bool> &valveList);
    void appendData(const QString & valveName, const bool &valveState);

signals:
    void channelMapListChanged();

private:
    QStringList m_valveNames;
    QMap<QString, Valve> m_valves;
    //QList<Valve*> m_valves; // not pointer because we know in start?
    QElapsedTimer m_time;
};