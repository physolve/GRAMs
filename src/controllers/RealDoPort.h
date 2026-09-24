#pragma once

#include "IDoPort.h"
#include "AdvantechCtrl.h"

// Плата клапанов USB-4750. Только делегирует в AdvantechDO.
class RealDoPort final : public IDoPort
{
public:
    AdvantechDO &device() { return m_device; }

    bool write(const QVector<bool> &states) override { return m_device.setData(states); }
    bool refresh() override { return m_device.refresh(); }
    QVector<bool> data() override { return m_device.getData(); }

private:
    AdvantechDO m_device;
};
