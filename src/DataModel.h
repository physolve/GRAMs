#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QColor>
#include "Sensor.h"

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
    Q_INVOKABLE Sensor* getSensor(const QString &name);
    // Q_INVOKABLE QVariantMap getCurPressureValues() const; // this is temporally, change to some ordinate way like map of names? or idx
    // Q_INVOKABLE QVariantMap getCurTempValues() const;
    Q_INVOKABLE void appendProfileSensors(const QString &controllerPurpose, const QVariantList &sensors); // QVector<double>& data??

    void initializeAcquisition();
    void appendData(const QMap<QString, QVector<double>> & dataMap);

signals:
    void channelMapListChanged();

private:
    QMap<QString, QStringList> m_controllersToSensors;
    QMap<QString, Sensor> m_sensors;
    //QList<Sensor*> m_sensors; // PURPOSE BASED Hash? pointer beacause we don't know in start?
    QElapsedTimer m_time;
};
