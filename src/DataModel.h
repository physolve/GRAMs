#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QColor>

struct Data {
    Data() {}
    Data( const QString& name, QList<quint64> x, QList<double> y)
        : name(name), x(x), y(y) {}
    QString name;
    QList<quint64> x; // one second data
    QList<double> y; // one second data


};

class MyModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::UserRole,
        Time,
        Value
    };

    explicit MyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void duplicateData(int row);
    void removeData(int row);
    void appendPoints(QList<quint64> pointX, QList<double> pointY);
    void startTestTimer(bool start);

private slots:
    void testDataFoo();
private: //members
    QVector<Data> m_data;
    QElapsedTimer m_time;
    QTimer *testDataTimer;
};