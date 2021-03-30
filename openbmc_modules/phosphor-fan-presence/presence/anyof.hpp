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
 * @class AnyOf
 * @brief AnyOf redundancy policy.
 *
 * The any of redundancy policy monitors all sensor
 * states in the redundancy set and reports true when any
 * sensor in the set reports true.
 */
class AnyOf : public RedundancyPolicy
{
  public:
    AnyOf() = delete;
    AnyOf(const AnyOf&) = default;
    AnyOf& operator=(const AnyOf&) = default;
    AnyOf(AnyOf&&) = default;
    AnyOf& operator=(AnyOf&&) = default;
    ~AnyOf() = default;

    /**
     * @brief Construct an any of bitwise policy.
     *
     * @param[in] fan - The fan associated with the policy.
     * @param[in] s - The set of sensors associated with the policy.
     */
    AnyOf(const Fan& fan,
          const std::vector<std::reference_wrapper<PresenceSensor>>& s);

    /**
     * @brief stateChanged
     *
     * Update the inventory and execute the fallback
     * policy.
     *
     * @param[in] present - The new presence state according
     *             to the specified sensor.
     * @param[in] sensor - The sensor reporting the new state.
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
    std::vector<std::tuple<std::reference_wrapper<PresenceSensor>, bool>> state;
};

} // namespace presence
} // namespace fan
} // namespace phosphor
