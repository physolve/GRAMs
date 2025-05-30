#include "InletAction.h"

static const double Rgas = 8.31446;
static const double abscTemp = 273.15;

InletAction::InletAction(QObject *parent) : QObject(parent), m_timer()
{
    connect(&m_timer, &QTimer::timeout, this, &InletAction::actionEvent);

    // configure state of the AddRemove and Storage quartile in a time spread
}

InletAction::~InletAction(){
   if(m_timer.isActive()) m_timer.stop();
}





