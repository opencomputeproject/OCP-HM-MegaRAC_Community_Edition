#include "nvmes.hpp"

namespace phosphor
{
namespace nvme
{

void NvmeSSD::checkSensorThreshold()
{
    int8_t value = ValueIface::value();
    int8_t criticalHigh = CriticalInterface::criticalHigh();
    int8_t criticalLow = CriticalInterface::criticalLow();
    int8_t warningHigh = WarningInterface::warningHigh();
    int8_t warningLow = WarningInterface::warningLow();

    CriticalInterface::criticalAlarmHigh(value > criticalHigh);

    CriticalInterface::criticalAlarmLow(value < criticalLow);

    WarningInterface::warningAlarmHigh(value > warningHigh);

    WarningInterface::warningAlarmLow(value < warningLow);
}

void NvmeSSD::setSensorThreshold(int8_t criticalHigh, int8_t criticalLow,
                                 int8_t maxValue, int8_t minValue,
                                 int8_t warningHigh, int8_t warningLow)
{

    CriticalInterface::criticalHigh(criticalHigh);
    CriticalInterface::criticalLow(criticalLow);

    WarningInterface::warningHigh(warningHigh);
    WarningInterface::warningLow(warningLow);

    ValueIface::maxValue(maxValue);
    ValueIface::minValue(minValue);
}

void NvmeSSD::setSensorValueToDbus(const int8_t value)
{
    ValueIface::value(value);
}

} // namespace nvme
} // namespace phosphor