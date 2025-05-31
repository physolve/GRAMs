#include "InletAction.h"

#include <QDebug>
#include <QThread>


InletAction::InletAction(QObject *parent) : QObject(parent)
{

    // configure state of the AddRemove and Storage quartile in a time spread
}

InletAction::~InletAction(){
}

void InletAction::runInletAction(QPromise<double> &promise){
    qDebug() << "Opened valve 0";
    promise.start();
    promise.setProgressRange(0, 2000);
    QElapsedTimer runInletTime;
    runInletTime.start();
    while(runInletTime.elapsed() < 1000){
        promise.setProgressValue(static_cast<int>(runInletTime.elapsed()));
        promise.suspendIfRequested();   // support suspension
        if (promise.isCanceled())       // support cancellation
            break;
        QThread::msleep(10);
    }
    promise.addResult(runInletTime.elapsed());
    qDebug() << "Closed valve 0";
}
