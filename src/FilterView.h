#pragma once
#include <QVariant>
#include "controllers/VoltageFilter.h"
#include "DataCollection.h"

class FilterView : public QObject
{
    Q_OBJECT 
    Q_PROPERTY (QList<double> uiA READ uiA WRITE setUiA NOTIFY uiAChanged)
    Q_PROPERTY (QList<double> uiC READ uiC WRITE setUiC NOTIFY uiCChanged)
    Q_PROPERTY (QList<double> uiQ READ uiQ WRITE setUiQ NOTIFY uiQChanged)
    Q_PROPERTY (double uiR READ uiR WRITE setUiR NOTIFY uiRChanged)
    Q_PROPERTY (QList<double> uiP READ uiP WRITE setUiP NOTIFY uiPChanged)
public:
    explicit FilterView(QObject *parent  = nullptr);
    ~FilterView();
    void readKalman();
    void parseKalman();

    Q_INVOKABLE void startAppend(bool state);
    // function to update values somewhere
    // function to link Kalman parameters with View
    // void appendDataToView(int viewN, const QVector<qreal> &time, const QVector<double> &data);
    // void appendDataToXhatS(int viewN, const QVector<qreal> &time, const QVector<double> &data);
    // void appendDataToXhatT(int viewN, const QVector<qreal> &time, const QVector<double> &data);

    void safeCheckOn();
    bool getSafeCheck();
    void saveToFile(const QVector<double> &data);
    bool getAppendCheck();
    void appendToFile(const QVector<double> &data);

    void setUiA(const QList<double> &ui_A);
    QList<double> uiA() const;
    void setUiC(const QList<double> &ui_C);
    QList<double>  uiC() const;
    void setUiQ(QList<double> ui_Q);
    QList<double> uiQ() const;
    void setUiR(double ui_R);
    double uiR() const;
    void setUiP(QList<double> ui_P);
    QList<double> uiP() const;

    FilterMatrix getNewFilterParameters() const;
    FilterMatrix getJsonMatrix() const;
signals:
    void changeFilterMatrix();

    // void updateView();
    // void updateXhatS();
    // void updateXhatT();

    void uiAChanged(QList<double>);
    void uiCChanged(QList<double> );
    void uiQChanged(QList<double>);
    void uiRChanged(double);
    void uiPChanged(QList<double>);

private:
    // should be separate window with Custom plot and Kalman Filter's parameters
    // QList<QSharedPointer<Sensor>> m_channelsData; // change to pointer of object from parent 
    QList<FilterData*> m_channelsData;
    // QList<QSharedPointer<Sensor>> m_channelsXhatS; // change to pointer of object from parent
    QList<FilterData*> m_channelsXhatS;
    // QList<QSharedPointer<Sensor>> m_channelsXhatT; // change to pointer of object from parent
    QList<FilterData*> m_channelsXhatT;

    QString kalmanProfile;

    QList<double> ui_mA;
    QList<double> ui_mC;
    QList<double> ui_mQ;
    double ui_mR;
    QList<double> ui_mP;
    bool safeCheck;
    bool appendCheck;

    // make it 8
    FilterMatrix jsonMatrix;
};