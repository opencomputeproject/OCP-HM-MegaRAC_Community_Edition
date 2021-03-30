#pragma once

#include "fan.hpp"
#include "rpolicy.hpp"

#include <functional>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace presence
{

class PresenceSensor;

/**
 * @class Fallback
 * @brief Fallback redundancy policy.
 *
 * The fallback redundancy policy falls back to
 * subsequent presence sensors when the active
 * sensor indicates not present and a fallback
 * sensor indicates the fan is present.
 */
class Fallback : public RedundancyPolicy
{
  public:
    Fallback() = delete;
    Fallback(const Fallback&) = default;
    Fallback& operator=(const Fallback&) = default;
    Fallback(Fallback&&) = default;
    Fallback& operator=(Fallback&&) = default;
    ~Fallback() = default;

    /**
     * @brief Construct a fallback policy.
     *
     * @param[in] fan - The fan associated with the policy.
     * @param[in] s - The set of sensors associated with the policy.
     */
    Fallback(const Fan& fan,
             const std::vector<std::reference_wrapper<PresenceSensor>>& s) :
        RedundancyPolicy(fan),
        sensors(s)
    {
        activeSensor = sensors.begin();
    }

    /**
     * @brief stateChanged
     *
     * Update the inventory and execute the fallback
     * policy.
     *
     * @param[in] present - The new presence state according
     *             to the active sensor.
     * @param[in] sensor - The sensor that changed state.
     */
    void stateChanged(bool present, PresenceSensor& sensor) override;

    /**
     * @brief monitor
     *
     * Start monitoring the fan.
     */
    void monitor() override;

  private:
    /** @brief All presence sensors in the redundancy set. */
    std::vector<std::reference_wrapper<PresenceSensor>> sensors;

    /** @brief The active presence sensor. */
    decltype(sensors)::iterator activeSensor;
};

} // namespace presence
} // namespace fan
} // namespace phosphor
