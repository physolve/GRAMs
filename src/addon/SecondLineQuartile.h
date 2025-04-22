#pragma once

#include "Quartile.h"

class SecondLineQuartile : public Quartile
{
    Q_OBJECT
public:
    explicit SecondLineQuartile(QObject *parent = nullptr);
    virtual ~SecondLineQuartile();

private:
    QList<VolumeObject> m_volumeObjects;
    // pressure sensor pointers
};