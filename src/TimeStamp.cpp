#include "TimeStamp.h"

TimeStamp::TimeStamp(QObject *parent) : QObject(parent)
{

}

TimeStamp::~TimeStamp()
{
    qDebug() << "TimeStamp destructor";
}

void TimeStamp::setInitialPressure(const QDateTime& checkStampTime, const guiValsPresVirtual& guiValsPresTimeStamp)
{
    m_guiValsPresTimestamp = guiValsPresTimeStamp;
    m_checkStampTime = checkStampTime;
}