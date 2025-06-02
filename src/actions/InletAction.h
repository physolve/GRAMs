#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QPromise>

struct guiInletAction{
    Q_GADGET
    Q_PROPERTY (double stPresTg         MEMBER m_storagePressureTarget)
    Q_PROPERTY (double c1PresTg         MEMBER m_c1PressureTarget)
    Q_PROPERTY (double c2PresTg         MEMBER m_c2PressureTarget)
    Q_PROPERTY (double c3PresTg         MEMBER m_c3PressureTarget)
    Q_PROPERTY (double timeOpenGasPort  MEMBER m_timeOpenGasPort)
public:
    double m_storagePressureTarget;
    double m_c1PressureTarget;
    double m_c2PressureTarget;
    double m_c3PressureTarget;
    double m_timeOpenGasPort;
};


class InletAction : public QObject
{
    Q_OBJECT
    // Q_PROPERTY (guiInletAction guiInletAction READ getGuiInletAction WRITE setGuiInletAction NOTIFY guiInletActionChanged)
public:
    InletAction(QObject *parent = 0);
    ~InletAction();
    void runInletAction(QPromise<int> &promise);

private:
    QElapsedTimer m_time;
    // pointers to valves
    // pointers to dataSource? interface
    // concurrent run
};



