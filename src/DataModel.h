#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QColor>
#include "Sensor.h"
#include "Controller.h"

// struct Data {
//     Data() {}
//     Data( const QString& name, QList<quint64> x, QList<double> y)
//         : name(name), x(x), y(y) {}
//     QString name;
//     QList<quint64> x; // one second data
//     QList<double> y; // one second data
// };

class MyModel : public QAbstractListModel
{
    Q_OBJECT
    //Q_PROPERTY(QVariantMap channelMapList MEMBER m_channelMapList NOTIFY channelMapListChanged)
public:
    enum Roles {
        NameRole = Qt::UserRole,
        Time,
        Value,
        CurTime,
        CurValue,
    };

    explicit MyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    //bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override; // use QMultiHash
    Q_INVOKABLE QVariant getCurValues() const;
    Q_INVOKABLE QVariant getCurPressureValues() const; // this is temporally, change to some ordinate way like map of names? or idx
    Q_INVOKABLE QVariant getCurTempValues() const;
    Q_INVOKABLE void appendProfileSensors(QVariantMap sensors); // QVector<double>& data??

    void initializeAcquisition(); //const QList<ControllerInfo>& info
    void appendData(const QList<QVector<double>> & dataList);
    //void fillSensors(const QVector<double>& data);

signals:
    void channelMapListChanged();

public slots:
    //void duplicateData(int row);
    //void removeData(int row);

private slots:
    //void testDataFoo();

    //void updateDataChanged(int idx);

private: //members
    //QList<ControllerInfo> m_controllersInfo;
    //QVector<Data> m_data;
    QList<Sensor*> m_sensors; // PURPOSE BASED Hash?
    QElapsedTimer m_time;
};