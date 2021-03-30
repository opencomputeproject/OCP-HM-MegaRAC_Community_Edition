#pragma once
#include "types.hpp"

#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace fan
{
namespace control
{

/**
 * @class Fan
 *
 * Represents a fan.  It has sensors used for setting speeds
 * on all of the contained rotors.  There may or may not be
 * a 1 to 1 correspondence between rotors and sensors, depending
 * on how the hardware and hwmon is configured.
 *
 */
class Fan
{
  public:
    Fan() = delete;
    Fan(const Fan&) = delete;
    Fan(Fan&&) = default;
    Fan& operator=(const Fan&) = delete;
    Fan& operator=(Fan&&) = default;
    ~Fan() = default;

    /**
     * Creates a fan object with sensors specified by
     * the fan definition data.
     *
     * @param[in] bus - the dbus object
     * @param[in] def - the fan definition data
     */
    Fan(sdbusplus::bus::bus& bus, const FanDefinition& def);

    /**
     * Sets the speed value on all contained sensors
     *
     * @param[in] speed - the value to set
     */
    void setSpeed(uint64_t speed);

    /**
     * @brief Get the current fan target speed
     *
     * @return - The target speed of the fan
     */
    inline auto getTargetSpeed() const
    {
        return _targetSpeed;
    }

  private:
    /**
     * The dbus object
     */
    sdbusplus::bus::bus& _bus;

    /**
     * The inventory name of the fan
     */
    std::string _name;

    /**
     * Map of hwmon target sensors to the service providing them
     */
    std::map<std::string, std::string> _sensors;

    /**
     * The interface of the fan target
     */
    const std::string _interface;

    /**
     * Target speed for this fan
     */
    uint64_t _targetSpeed;
};

} // namespace control
} // namespace fan
} // namespace phosphor
