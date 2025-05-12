#pragma once
#include <QDebug>
#include <QVariant>
#include <QDateTime>

struct guiValsPresVirtual{
    Q_GADGET
    Q_PROPERTY (double prSQ    MEMBER g_prSQ)
    Q_PROPERTY (double prRQ    MEMBER g_prRQ)
    Q_PROPERTY (double prSC1   MEMBER g_prSC1)
    Q_PROPERTY (double prSC2   MEMBER g_prSC2)
    Q_PROPERTY (double prSC3   MEMBER g_prSC3)
    Q_PROPERTY (double prSB    MEMBER g_prSB)
    Q_PROPERTY (double prSD1   MEMBER g_prSD1)
    Q_PROPERTY (double prRE    MEMBER g_prRE)
    Q_PROPERTY (double prRD2   MEMBER g_prRD2)
    Q_PROPERTY (double prRF    MEMBER g_prRF)
public:
    double g_prSQ;
    double g_prRQ;
    double g_prSC1;
    double g_prSC2;
    double g_prSC3;
    double g_prSB;
    double g_prSD1;
    double g_prRE;
    double g_prRD2;
    double g_prRF;
};


class TimeStamp : public QObject
{
    Q_OBJECT
    Q_PROPERTY (guiValsPresVirtual guiPresTimestamp MEMBER m_guiValsPresTimestamp CONSTANT)
    Q_PROPERTY (QDateTime checkStampTime MEMBER m_checkStampTime CONSTANT)
public:
    explicit TimeStamp(QObject *parent  = nullptr);
    ~TimeStamp();
    void setInitialPressure(const QDateTime& checkStampTime, const guiValsPresVirtual& guiValsPresTimestamp); // gui struct as value?
private:
    guiValsPresVirtual m_guiValsPresTimestamp;
    QDateTime m_checkStampTime;
};