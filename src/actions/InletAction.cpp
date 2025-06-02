#include "InletAction.h"

#include <QDebug>
#include <QThread>


InletAction::InletAction(QObject *parent) : QObject(parent)
{

    // configure state of the AddRemove and Storage quartile in a time spread
}

InletAction::~InletAction(){
}

void InletAction::runInletAction(QPromise<int> &promise){
    qDebug() << "Opened valve 0";
    QElapsedTimer runInletTime;
    runInletTime.start();
    promise.setProgressRange(0, 2000);
    promise.start();
    promise.addResult(0);
    while(runInletTime.elapsed() < 2000){
        promise.setProgressValue(runInletTime.elapsed());
        promise.suspendIfRequested();   // support suspension
        if (promise.isCanceled())       // support cancellation
            break;
        QThread::msleep(10);
    }
    promise.addResult(runInletTime.elapsed());
    qDebug() << "Closed valve 0";
}
