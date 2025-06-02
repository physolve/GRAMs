#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QPromise>
#include <QFutureWatcher>

#include "InletAction.h"


class ActionHandler : public QObject
{
    Q_OBJECT
public:
    ActionHandler(QObject *parent = 0);
    ~ActionHandler();
    void runInletAction();
private:
    QFutureWatcher<int> watcher;
    // Valve Control pointer
};
