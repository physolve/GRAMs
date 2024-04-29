#pragma once
#include <QString>
#include <QMap>
//#include <QTimer>
//#include <QDebug>

class Sensor //: public QObject
{
    //Q_OBJECT
public:
    explicit Sensor(const QString &name = "unknown", const QMap<QString, double> &parameters = QMap<QString, double>());  //QObject *parent = nullptr  //(if we are going to use qml )
    ~Sensor();                                                          
    QString m_name;
    void appendData(qreal x, double y);
    QVector<qreal> getTime() const;
    QVector<double> getValue() const;
    qreal getCurTime() const;
    double getCurValue() const;
//signals:

//public slots:
// protected:
//     virtual void filterData() = 0;

private :
    QList<qreal> m_x; // one second data
    QList<double> m_y; // one second data
    qreal m_cX;
    double m_cY;

    void filterData(double &data);

    double m_A = 1;
    double m_B = 0;
    double m_R = 1;
};

