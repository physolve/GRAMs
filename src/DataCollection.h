#pragma once

#include <QString>
#include <QDebug>

enum DataType{
    Dimensionless,
    Pressure,
    Temperature,
    Mass
};

class DataCollection //: public QObject
{
    //Q_OBJECT
public:
    DataCollection(const QString &name = "unknown");
    virtual ~DataCollection();
    void clearPoints();
    void addPoint(const double &val_y);
    QVector<double> getValue() const;
    QVector<double> getLastToChart() const;
    double getCurValue() const;
    void setAltUnitCoef(const double &coef);
    double getAltUnit() const;
    QString m_name;
    DataType m_type;
    // alternative unit

protected:
    QList<double> m_y; // one second data
    double m_curValue;
    double m_altUnitA;
};

class ControllerData : public DataCollection
{
public:
    ControllerData(const QString &name = "unknown", const double& A = 1, const double& B = 0);
    virtual ~ControllerData();
    void setCoeffs(const double& A, const double& B);
    void addValue(const double &val_y);
    void addValue(const double &val_y, const double &minimalValue);
    double getLin_A() const;
    double getLin_B() const;
private:
    double lin_A;
    double lin_B;
};

class FilterData : public DataCollection
{
public:
    FilterData(const QString &name = "unknown");
    virtual ~FilterData();
    void setData(const QVector<double> &y);
    void addData(const QVector<double> &y);
    bool isCumulativeReady() const;
    QVector<double> getCumulativeData() const;
    int getCumulativeCount();
    void clearCumulative();
private:
    QVector<double> cumulativeData;
    int cumulativeCount{0};
};

class QuartileData : public DataCollection{
public:
    QuartileData(const QString &name = "unknown");
    virtual ~QuartileData();
    // returns and store pressure from other ControllerData
     
};

class NodeData : public DataCollection
{
public:
    NodeData(const QString &name = "unknown");
    virtual ~NodeData();
};

class MolesData : public DataCollection{
public:
    MolesData(const QString &name = "unknown");
    virtual ~MolesData();
};

// class Sensor //: public QObject
// {
//     //Q_OBJECT
// public:
//     explicit Sensor(const QString &name = "unknown", const QMap<QString, double> &parameters = QMap<QString, double>({{"A", 1}, {"B", 0}, {"R", 1}}));  //QObject *parent = nullptr  //(if we are going to use qml )
//     ~Sensor();                                                          
//     QString m_name;
//     void appendData(qreal x, double y);
//     void setData(const QVector<qreal> &x, const QVector<double> &y);
//     QVector<qreal> getTime() const; 
//     QVector<double> getValue() const;
//     qreal getCurTime() const;
//     double getCurValue() const;
// //signals:

// //public slots:
// // protected:
// //     virtual void filterData() = 0;

// private :
//     QList<qreal> m_x; // one second data
//     QList<double> m_y; // one second data
//     //qreal m_cX;
//     //double m_cY;

//     void filterVoltage(double &voltage);

//     void filterData(double &data);

//     double m_A = 1;
//     double m_B = 0;
//     double m_R = 1;
// };

