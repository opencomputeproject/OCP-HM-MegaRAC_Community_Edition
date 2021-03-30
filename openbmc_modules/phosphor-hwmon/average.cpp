#include "average.hpp"

#include <cassert>

std::optional<Average::averageValue>
    Average::getAverageValue(const Average::averageKey& sensorKey) const
{
    const auto it = _previousAverageMap.find(sensorKey);
    if (it == _previousAverageMap.end())
    {
        return {};
    }

    return std::optional(it->second);
}

void Average::setAverageValue(const Average::averageKey& sensorKey,
                              const Average::averageValue& sensorValue)
{
    _previousAverageMap[sensorKey] = sensorValue;
}

std::optional<int64_t> Average::calcAverage(int64_t preAverage,
                                            int64_t preInterval,
                                            int64_t curAverage,
                                            int64_t curInterval)
{
    int64_t value = 0;
    // Estimate that the interval will overflow about 292471
    // years after it starts counting, so consider it won't
    // overflow
    int64_t delta = curInterval - preInterval;

    assert(delta >= 0);
    // 0 means the delta interval is too short, the value of
    // power*_average_interval is not changed yet
    if (delta == 0)
    {
        return {};
    }
    // Change formula (a2*i2-a1*i1)/(i2-i1) to be the
    // following formula, to avoid multiplication overflow.
    // (a2*i2-a1*i1)/(i2-i1) =
    // (a2*(i1+delta)-a1*i1)/delta =
    // (a2-a1)(i1/delta)+a2
    value =
        (curAverage - preAverage) * (static_cast<double>(preInterval) / delta) +
        curAverage;

    return value;
}
