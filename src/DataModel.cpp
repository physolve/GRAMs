#include "DataModel.h"

#include <QByteArray>
#include <QTimer>
#include <QtCore/QRandomGenerator>
#include <QtCore/QtMath>
#include <cstdlib>

MyModel::MyModel(QObject *parent) :
    QAbstractListModel(parent)
{
    m_data
        << Data("Series A", {0}, {0})
        << Data("Series B", {0}, {0})
        << Data("Series C", {0}, {0})
        << Data("Series D", {0}, {0})
        << Data("Series E", {0}, {0});

    testDataTimer = new QTimer(this);
    connect(testDataTimer , &QTimer::timeout, this, &MyModel::testDataFoo);
    //testDataTimer->start(1000);

    m_time.start();
}

void MyModel::startTestTimer(bool start){
    if(!testDataTimer->isActive()&&start)
        testDataTimer->start(1000);
    else
        testDataTimer->stop();
}

int MyModel::rowCount( const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return m_data.count();
}

QVariant MyModel::data(const QModelIndex &index, int role) const
{
    if ( !index.isValid() )
        return QVariant();

    const Data &data = m_data.at(index.row());
    if ( role == NameRole ){
        return data.name;
    }
    else if ( role == Time )
        return QVariant::fromValue(data.x);
    else if ( role == Value )
        return QVariant::fromValue(data.y);
    else if ( role == CurTime )
        return QVariant::fromValue(data.x.last());
    else if ( role == CurValue )
        return QVariant::fromValue(data.y.last());
    else
        return QVariant();
}

//--> slide
QHash<int, QByteArray> MyModel::roleNames() const
{
    static QHash<int, QByteArray> mapping {
        {NameRole, "name"},
        {Time, "x"},
        {Value, "y"},
        {CurTime, "ct"},
        {CurValue, "cv"}
    };
    return mapping;
}
//<-- slide

void MyModel::duplicateData(int row)
{
    if (row < 0 || row >= m_data.count())
        return;

    const Data data = m_data[row];
    const int rowOfInsert = row + 1;

    beginInsertRows(QModelIndex(), rowOfInsert, rowOfInsert);
    m_data.insert(rowOfInsert, data);
    endInsertRows();
}

void MyModel::removeData(int row)
{
    if (row < 0 || row >= m_data.count())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_data.removeAt(row);
    endRemoveRows();
}

void MyModel::testDataFoo(){
    static int t;
    double U, V;
    const int count = m_data.count();
    for (int i = 0; i < count; ++i) {
        //U = qCos(M_PI / 50 * t) + 0.5 + QRandomGenerator::global()->generateDouble();
        V = qSin(M_PI / 50 * t) + 0.5 + QRandomGenerator::global()->generateDouble();
        m_data[i].x.append(m_time.elapsed()/1000);
        m_data[i].y.append(V);
    }
    t++;
    // we've just updated all rows...
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(count - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value << CurTime << CurValue );
}

void MyModel::appendPoints(QList<quint64> pointX, QList<double> pointY)
{
    const int count = m_data.count();
    for (int i = 0; i < count; ++i) {
        m_data[i].x = (pointX);
        m_data[i].y = (pointY);
    }
    // we've just updated all rows...
    const QModelIndex startIndex = index(0, 0);
    const QModelIndex endIndex   = index(count - 1, 0);

    // ...but only the population field
    emit dataChanged(startIndex, endIndex, QVector<int>() << Time << Value);
}