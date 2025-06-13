#include "InletAction.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>

void InletAction::runInletAction(QPromise<int> &promise){
    if(!valveControl->isControlRunning()||!dataAcquisition->getGRAMsIntegrity()){
        promise.addResult(0);
        // promise.future().suspend(); //?
        promise.future().cancel(); //?
        // promise.finish();
        qDebug() << "Canceling Inlet Action due connection lost";
        return;
    }
    InletStrategy inletStrategy = addRemoveQuartile->getInletStrategy();
    promise.setProgressRange(0, inletStrategy.m_openTime);
    promise.start();
    promise.addResult(0);
    valveControl->beginAction();
    dataAcquisition->beginAction();
    addRemoveQuartile->fillSupplyActionData();
    QThread::msleep(1000);
    bool setOK = valveControl->setValveFromAction(true, inletStrategy.m_usePort);
    if(!setOK){
        qDebug() << "Canceling Inlet Action due unable to setValveFromAction";
        return;
    }
    QElapsedTimer runInletTime;
    runInletTime.start();
    qDebug() << "Opened valve " + inletStrategy.m_usePort;
    while(runInletTime.elapsed() < inletStrategy.m_openTime && addRemoveQuartile->checkSupplyAction()){
        promise.setProgressValue(runInletTime.elapsed());
        promise.suspendIfRequested();   // support suspension
        if (promise.isCanceled())       // support cancellation
            break;
        dataAcquisition->fastBufferRead();
        dataAcquisition->runSupplyAction();
        addRemoveQuartile->fillSupplyPortData();
        if(valveControl->isActionInterrupted()){
            // suspend?
            qDebug() << "Other valve were clicked";
            valveControl->beginAction();
        }
        QThread::msleep(100);
    }
    setOK = valveControl->setValveFromAction(false, inletStrategy.m_usePort);
    valveControl->endAction();
    dataAcquisition->endAction();
    addRemoveQuartile->saveSupplyActionData();
    promise.addResult(runInletTime.elapsed());
    promise.finish();
    qDebug() << "Closed valve " + inletStrategy.m_usePort;
}
