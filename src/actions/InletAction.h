#pragma once

#include <QPromise>

#include "../DataAcquisition.h"
#include "../ValveControl.h"
#include "../addon/AddRemoveQuartile.h"

struct InletAction
{
   ValveControl* valveControl = nullptr;
   DataAcquisition* dataAcquisition = nullptr;
   AddRemoveQuartile* addRemoveQuartile = nullptr;
   void runInletAction(QPromise<int> &promise);
};



