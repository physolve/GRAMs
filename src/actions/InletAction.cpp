#include "InletAction.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>

void InletAction::runInletAction(QPromise<int> &promise, ValveControl* valveControl, AddRemoveQuartile* addRemoveQuartile){
    if(!valveControl->isControlRunning()){
        promise.addResult(0);
        // promise.future().suspend(); //?
        promise.future().cancel(); //?
        // promise.finish();
        qDebug() << "Canceling Inlet Action due connection lost";
        return;
    }
    QElapsedTimer m_time;
    const auto& inletStrategy = addRemoveQuartile->getInletStrategy();
    QElapsedTimer runInletTime;
    runInletTime.start();
    promise.setProgressRange(0, inletStrategy.m_openTime);
    promise.start();
    promise.addResult(0);
    bool setOK = valveControl->setValveFromAction(true, inletStrategy.m_usePort);
    if(!setOK){
        qDebug() << "Canceling Inlet Action due unable to setValveFromAction";
        return;
    }
    qDebug() << "Opened valve " + inletStrategy.m_usePort;
    while(runInletTime.elapsed() < inletStrategy.m_openTime && addRemoveQuartile->checkSupplyAction()){
        promise.setProgressValue(runInletTime.elapsed());
        promise.suspendIfRequested();   // support suspension
        if (promise.isCanceled())       // support cancellation
            break;
        QThread::msleep(10);
    }
    setOK = valveControl->setValveFromAction(false, inletStrategy.m_usePort);
    promise.addResult(runInletTime.elapsed());
    promise.finish();
    qDebug() << "Closed valve " + inletStrategy.m_usePort;
}
