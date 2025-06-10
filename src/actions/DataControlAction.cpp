#include "DataControlAction.h"

#include <QDebug>
#include <QThread>

void DataControlAction::runDataControlAction(QPromise<int> &promise, DataAcquisition* dataAcquisition){
    if(!dataAcquisition->getGRAMsIntegrity()){
        promise.addResult(0);
        // promise.future().suspend(); //?
        qDebug() << "Canceling Data Control Action due connection lost";
        promise.future().cancel(); //?
        // promise.finish();
        return;
    }
    dataAcquisition->setFastBufferRead(true);
    while(true){ // !forceStopFlag
        promise.suspendIfRequested();   // support suspension
        if (promise.isCanceled())       // support cancellation
            break;
        dataAcquisition->runSupplyAction();
        QThread::msleep(250);
    }
    dataAcquisition->setFastBufferRead(false);
    promise.finish();
}