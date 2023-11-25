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

    void initializeAcquisition(const QList<ControllerInfo>& info);
    void appendData(const QList<QVector<double>> & dataList);
    void fillSensors(const QVector<double>& data);
public slots:
    //void duplicateData(int row);
    //void removeData(int row);

private slots:
    //void testDataFoo();

    //void updateDataChanged(int idx);
private: //members
    QList<ControllerInfo> m_controllersInfo;
    //QVector<Data> m_data;
    QList<Sensor*> m_sensors; // PURPOSE BASED Hash?
    QElapsedTimer m_time;
};