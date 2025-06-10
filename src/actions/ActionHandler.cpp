#include "ActionHandler.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFuture>

ActionHandler::ActionHandler(QObject *parent) : QObject(parent)
{
    // connect(&watcher, &QFutureWatcher<int>::progressValueChanged,
    //     [](int progress) { qDebug() << "Progress:" << progress; });
    // connect(&watcher, &QFutureWatcher<int>::resultReadyAt,
    //     [](int result) { qDebug() << "Intermediate Result index:" << result; });
    // connect(&watcher, &QFutureWatcher<int>::finished, this,
    //     []() { qDebug() << "Finished: watcher"; });

}

ActionHandler::~ActionHandler(){
}

void ActionHandler::setValveControl(ValveControl* valveControl){
    m_valveControl = valveControl;
}
void ActionHandler::setSecurity(Security* security){
    m_security = security;
}

void ActionHandler::setDataAcquisition(DataAcquisition* dataAcquisition){
    m_dataAcquisition = dataAcquisition;
}

void ActionHandler::setAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile){
    m_addRemoveQuartile = addRemoveQuartile;
}

void ActionHandler::prepareInletAction(){

}

void ActionHandler::runInletAction(){
    // InletAction inletAction;
    // DataControlAction dataControlAction;
    QFutureWatcher<int> dataControlWatcher;
    QFuture<int> dataFuture = QtConcurrent::run(&DataControlAction::runDataControlAction, m_dataAcquisition); //&dataControlAction
    dataControlWatcher.setFuture(dataFuture);
    dataControlWatcher.future().then([](int res) {
        qDebug() << "dataControlWatcher. " << res;
    }).onCanceled([] {
        qDebug() << "dataControlWatcher was canceled";
    });

    QFutureWatcher<int> inletActionWatcher;
    QFuture<int> actionFuture = QtConcurrent::run(&InletAction::runInletAction, m_valveControl, m_addRemoveQuartile);
    inletActionWatcher.setFuture(actionFuture);
    inletActionWatcher.future().then(this, [](int res1){
        qDebug() << "1. " << res1;
    }).onCanceled([] {
        qDebug() << "inletActionWatcher was canceled";
    });
    connect(&inletActionWatcher, &QFutureWatcher<int>::finished, &dataControlWatcher, &QFutureWatcher<int>::cancel);
    
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