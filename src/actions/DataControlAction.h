#pragma once

#include <QPromise>
#include "../DataAcquisition.h"

struct DataControlAction
{
    static void runDataControlAction(QPromise<int> &promise, DataAcquisition* dataAcquisition);
};



