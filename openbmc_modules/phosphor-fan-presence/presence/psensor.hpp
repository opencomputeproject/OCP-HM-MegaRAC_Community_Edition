#pragma once
#include <cstdint>

namespace phosphor
{
namespace fan
{
namespace presence
{

/**
 * @class PresenceSensor
 * @brief PresenceSensor interface.
 *
 * Provide concrete implementations of PresenceSensor to realize
 * new presence detection methods.
 *
 * Note that implementations drive the inventory update process via
 * a redundancy policy (rpolicy.hpp) - it is not enough to implement
 * the interfaces below.
 */
class PresenceSensor
{
  public:
    PresenceSensor(const PresenceSensor&) = default;
    PresenceSensor& operator=(const PresenceSensor&) = default;
    PresenceSensor(PresenceSensor&&) = default;
    PresenceSensor& operator=(PresenceSensor&&) = default;
    virtual ~PresenceSensor() = default;
    PresenceSensor() : id(nextId)
    {
        nextId++;
    }

    /**
     * @brief start
     *
     * Implementations should peform any preparation
     * for detecting presence.  Typical implementations
     * might register signal callbacks or start
     * a polling loop.
     *
     * @return The state of the sensor.
     */
    virtual bool start() = 0;

    /**
     * @brief stop
     *
     * Implementations should stop issuing presence
     * state change notifications.  Typical implementations
     * might de-register signal callbacks or terminate
     * polling loops.
     */
    virtual void stop() = 0;

    /**
     * @brief Check the sensor.
     *
     * Implementations should perform an offline (the start
     * method has not been invoked) query of the presence
     * state.
     *
     * @return The state of the sensor.
     */
    virtual bool present() = 0;

    /**
     * @brief Mark the sensor as failed.
     *
     * Implementations should log an an event if the
     * system policy requires it.
     *
     * Provide a default noop implementation.
     */
    virtual void fail()
    {}

    friend bool operator==(const PresenceSensor& l, const PresenceSensor& r);

  private:
    /** @brief Unique sensor ID. */
    std::size_t id;

    /** @brief The next unique sensor ID. */
    static std::size_t nextId;
};

inline bool operator==(const PresenceSensor& l, const PresenceSensor& r)
{
    return l.id == r.id;
}

} // namespace presence
} // namespace fan
} // namespace phosphor
