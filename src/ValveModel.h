#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>

struct Valve{
    QString m_name = "";
    bool m_state = false;
    void setState(bool s){
        m_state = s;
    }
    Valve(QString name, bool state): m_name(name), m_state(state) { }
};


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

    Q_INVOKABLE void appendValves(QVariant valves); 
    void getStates();

    void appendData(const QVector<bool> & valveList);

signals:
    void channelMapListChanged();

private:
    QList<Valve*> m_valves; // not pointer beacause we know in start?
    QElapsedTimer m_time;
};