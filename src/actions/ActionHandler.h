#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QPromise>

#include "InletAction.h"


class ActionHandler : public QObject
{
    Q_OBJECT
public:
    ActionHandler(QObject *parent = 0);
    ~ActionHandler();
    void runInletAction();

};


