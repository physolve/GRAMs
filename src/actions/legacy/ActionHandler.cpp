#include "ActionHandler.h"

#include <QDebug>
#include <QtConcurrent>
#include <QFuture>

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
    // QFutureWatcher<int> dataControlWatcher;
    // QFuture<int> dataFuture = QtConcurrent::run(&DataControlAction::runDataControlAction, m_dataAcquisition); //&dataControlAction
    // dataControlWatcher.setFuture(dataFuture);
    // dataControlWatcher.future().then([](int res) {
    //     qDebug() << "dataControlWatcher. " << res;
    // }).onCanceled([=] {
    //     qDebug() << "dataControlWatcher was canceled";
    // });

    QFutureWatcher<int>* inletActionWatcher = new QFutureWatcher<int>;
    InletAction* inletAction = new InletAction();
    inletAction->valveControl = m_valveControl;
    inletAction->dataAcquisition = m_dataAcquisition;
    inletAction->addRemoveQuartile = m_addRemoveQuartile;
    connect(inletActionWatcher, &QFutureWatcher<int>::progressValueChanged,
        [](int progress) { qDebug() << "Progress:" << progress; });
    connect(inletActionWatcher, &QFutureWatcher<int>::resultReadyAt,
        [](int result) { qDebug() << "Intermediate Result index:" << result; });
    connect(inletActionWatcher, &QFutureWatcher<int>::finished,  [inletAction, inletActionWatcher]() { 
        inletAction->addRemoveQuartile = nullptr;
        inletAction->dataAcquisition = nullptr;
        inletAction->valveControl = nullptr;
        delete inletAction; 
        qDebug() << "Deleted inletAction";
        delete inletActionWatcher;
        qDebug() << "Deleted inletActionWatcher";
    });
    QFuture<int> actionFuture = QtConcurrent::run(&InletAction::runInletAction, inletAction);
    inletActionWatcher->setFuture(actionFuture);
    // inletActionWatcher->future().then(this, [](int res1){
    //     qDebug() << "1. " << res1;
    // }).onCanceled([] {
    //     qDebug() << "inletActionWatcher was canceled";
    // });
    
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