#include "ActionHandler.h"

#include <QDebug>
#include <QtConcurrent>

ActionHandler::ActionHandler(QObject *parent) : QObject(parent)
{
    connect(&watcher, &QFutureWatcher<int>::progressValueChanged,
        [](int progress) { qDebug() << "Progress:" << progress; });
    connect(&watcher, &QFutureWatcher<int>::resultReadyAt,
        [](int result) { qDebug() << "Intermediate Result index:" << result; });
    connect(&watcher, &QFutureWatcher<int>::finished, this,
        []() { qDebug() << "Finished: watcher"; });

}

ActionHandler::~ActionHandler(){
}

void ActionHandler::runInletAction(){
    InletAction inletAction;
    QFuture<int> future = QtConcurrent::run(&InletAction::runInletAction, &inletAction);
    watcher.setFuture(future);
    watcher.future().then(this, [](int res1){
        qDebug() << "1. " << res1;
    });
    // .then(this, [&](){
    //     qDebug() << future.results();
    // });
    // watcher.future().then([](int res) {
    //     qDebug() << "1. " << res;
    // }).then([](int res) {
    //     qDebug() << "2. " << res;
    // }).onCanceled([] {
    //     qDebug() << "Canceled";
    // });
}