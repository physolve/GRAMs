#include "ActionHandler.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFutureWatcher>

ActionHandler::ActionHandler(QObject *parent) : QObject(parent)
{
}


ActionHandler::~ActionHandler(){
}

void ActionHandler::runInletAction(){
    InletAction inletAction;
    QFutureWatcher<double> watcher;
    connect(&watcher, &QFutureWatcher<double>::progressValueChanged, [](int progress){
        qDebug() << "Progress: " << progress;
    });
    watcher.setFuture(QtConcurrent::run([&inletAction](QPromise<double> &promise) {
        inletAction.runInletAction(promise);
    }));
    // watcher.waitForFinished();
    watcher.future().then([&](double result) {
      qDebug() << "Result from myMethod:" << result;
    });
}