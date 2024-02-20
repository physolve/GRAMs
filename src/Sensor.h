#pragma once

#include <QObject>
//#include <QTimer>
#include <QDebug>

class Sensor : public QObject
{
    Q_OBJECT
public:
    explicit Sensor(const QString &name, const QMap<QString,double> &parameters, QObject *parent = nullptr);  // later create QObject child with other properties 
                                                                        //(if we are going to use qml )
    QString m_name;
    void appendData(quint64 x, double y);
    QList<quint64>& getTime();
    QList<double>& getValue();
    quint64 getCurTime();
    double getCurValue();
//signals:

//public slots:
// protected:
//     virtual void filterData() = 0;

private :
    QList<quint64> m_x; // one second data
    QList<double> m_y; // one second data
    quint64 m_cX;
    double m_cY;

    void filterData(double &data);

    double m_A = 1;
    double m_B = 0;
    double m_R = 1;
};

