#pragma once

#include "fan.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

class PresenceSensor;

/**
 * @class RedundancyPolicy
 * @brief Redundancy policy interface.
 *
 * Provide concrete implementations of RedundancyPolicy to realize
 * new redundancy logic.
 *
 * A fan can have multiple ways to detect whether or not it is present.
 * The redundancy policy encapsulates the logic to aggregate those
 * inputs into a single yes or no the fan is present.
 */
class RedundancyPolicy
{
  public:
    RedundancyPolicy(const RedundancyPolicy&) = default;
    RedundancyPolicy& operator=(const RedundancyPolicy&) = default;
    RedundancyPolicy(RedundancyPolicy&&) = default;
    RedundancyPolicy& operator=(RedundancyPolicy&&) = default;
    virtual ~RedundancyPolicy() = default;

    /**
     * @brief Construct a new Redundancy Policy.
     *
     * @param[in] fan - The fan associated with this policy.
     */
    explicit RedundancyPolicy(const Fan& f) : fan(f)
    {}

    /**
     * @brief stateChanged
     *
     * Typically invoked by presence sensors to signify
     * a change of presence.  Implementations should update
     * the inventory and execute their policy logic.
     *
     * @param[in] present - The new state of the sensor.
     * @param[in] sensor - The sensor that changed state.
     */
    virtual void stateChanged(bool present, PresenceSensor& sensor) = 0;

    /**
     * @brief monitor
     *
     * Implementations should start monitoring the sensors
     * associated with the fan.
     */
    virtual void monitor() = 0;

  protected:
    /** @brief Fan name and inventory path. */
    const Fan& fan;
};

/**
 * @class PolicyAccess
 * @brief Policy association.
 *
 * PolicyAccess can be used to associate a redundancy policy
 * with something else.
 *
 * Wrap the type to be associated with a policy with PolicyAccess.
 *
 * @tparam T - The type to associate with a redundancy policy.
 * @tparam Policy - An array type where the policy is stored.
 */
template <typename T, typename Policy>
class PolicyAccess : public T
{
  public:
    PolicyAccess() = default;
    PolicyAccess(const PolicyAccess&) = default;
    PolicyAccess& operator=(const PolicyAccess&) = default;
    PolicyAccess(PolicyAccess&&) = default;
    PolicyAccess& operator=(PolicyAccess&&) = default;
    ~PolicyAccess() = default;

    /**
     * @brief Construct a new PolicyAccess wrapped object.
     *
     * @param[in] index - The array index in Policy.
     * @tparam Args - Forwarded to wrapped type constructor.
     */
    template <typename... Args>
    PolicyAccess(size_t index, Args&&... args) :
        T(std::forward<Args>(args)...), policy(index)
    {}

  private:
    /**
     * @brief Get the associated policy.
     */
    RedundancyPolicy& getPolicy() override
    {
        return *Policy::get()[policy];
    }

    /** The associated policy index. */
    size_t policy;
};
} // namespace presence
} // namespace fan
} // namespace phosphor
