#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QPromise>
#include <QFutureWatcher>

#include "InletAction.h"
#include "DataControlAction.h"
#include "../ValveControl.h"
#include "../Security.h"
#include "../DataAcquisition.h"

class ActionHandler : public QObject
{
    Q_OBJECT
public:
    ActionHandler(QObject *parent = 0);
    ~ActionHandler();
    void setValveControl(ValveControl* valveControl);
    void setAddRemoveQuartile(AddRemoveQuartile* addRemoveQuartile);
    void setDataAcquisition(DataAcquisition* dataAcquisition);
    void setSecurity(Security* security);
    void prepareInletAction();
    void runInletAction();
    void finishInletAction();
private:
    QFutureWatcher<int> watcher;
    // QFutureWatcher<int> dataControlWatcher;
    // Valve Control pointer
    ValveControl* m_valveControl;
    
    // Data Acquisition
    DataAcquisition* m_dataAcquisition;
    
    // Security module pointer
    Security* m_security; // to check pressure?
    // Add Remove Quartile
    AddRemoveQuartile* m_addRemoveQuartile;

    QList<InletAction> m_inletActions;
    QList<DataControlAction> m_dataControlActions;
};
