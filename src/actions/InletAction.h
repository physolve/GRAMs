#pragma once

#include <QPromise>

#include "../ValveControl.h"
#include "../addon/AddRemoveQuartile.h"

struct InletAction
{
   static void runInletAction(QPromise<int> &promise, ValveControl* valveControl, AddRemoveQuartile* addRemoveQuartile);
};



