#include "FilterView.h"

FilterView::FilterView(QObject *parent) : QObject(parent)
{
}

FilterView::~FilterView(){
}

void FilterView::setFilterSize(int channelCount){
    // argument put count to sensor container
    m_channelsData.resize(channelCount);
}
