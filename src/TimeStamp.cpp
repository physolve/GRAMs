#include "TimeStamp.h"

TimeStamp::TimeStamp(QObject *parent) : QObject(parent)
{

}

TimeStamp::~TimeStamp()
{
    qDebug() << "TimeStamp destructor";
}

void TimeStamp::setInitialPressure(const QDateTime& checkStampTime, const guiValsPresVirtual& guiValsPresVirtual)
{
    m_guiValsPresVirtual = guiValsPresVirtual;
    m_checkStampTime = checkStampTime;
}