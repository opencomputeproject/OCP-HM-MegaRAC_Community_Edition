#pragma once

#include "sensorset.hpp"

#include <cstdlib>

namespace phosphor
{
namespace utility
{
/** @struct Free
 *  @brief A malloc cleanup type for use with smart pointers.
 */
template <typename T>
struct Free
{
    void operator()(T* ptr) const
    {
        free(ptr);
    }
};

/** @brief Check if AVERAGE_power* is set to be true in env
 *
 *  @param[in] sensor - Sensor details
 *
 *  @return bool - true or false
 */
inline bool isAverageEnvSet(const SensorSet::key_type& sensor)
{
    return env::getEnv("AVERAGE", sensor.first, sensor.second) == "true";
}
} // namespace utility
} // namespace phosphor
// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
