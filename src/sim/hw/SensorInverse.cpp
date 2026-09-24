#include "SensorInverse.h"

namespace sim::SensorInverse {

namespace {
double gain(const ChannelBinding &b)
{
    return b.A / b.R * 1000.0;   // как Grams::initAnalogData
}
} // namespace

double pressureCardVolts(const ChannelBinding &binding, ChannelKind kind, double wireValue)
{
    const double engineering = kind == ChannelKind::Temperature ? wireValue : wireValue / 1e5;
    return (engineering - binding.B) / gain(binding);
}

double pressureCardWire(const ChannelBinding &binding, ChannelKind kind, double volts)
{
    const double engineering = gain(binding) * volts + binding.B;
    return kind == ChannelKind::Temperature ? engineering : engineering * 1e5;
}

GaugeReading gauge(double pa)
{
    const double torr = pa / kTorrToPa;
    if (torr > kGaugeMaxTorr)
        return {kGaugeMaxTorr, Quality::OverRange};
    if (torr < kGaugeMinTorr * (1.0 - 1e-9))
        return {kGaugeMinTorr, Quality::UnderRange};
    return {torr, Quality::Valid};
}

} // namespace sim::SensorInverse
