#pragma once

#include "tach_sensor.hpp"
#include "trust_manager.hpp"
#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdeventplus/event.hpp>

#include <tuple>
#include <vector>

namespace phosphor
{
namespace fan
{
namespace monitor
{

/**
 * @class InvalidSensorError
 *
 * An exception type for sensors that don't exist or
 * are otherwise inaccessible.
 */
class InvalidSensorError : public std::exception
{};

/**
 * @class Fan
 *
 * Represents a fan, which can contain 1 or more sensors which
 * loosely correspond to rotors.  See below.
 *
 * There is a sensor when hwmon exposes one, which means there is a
 * speed value to be read.  Sometimes there is a sensor per rotor,
 * and other times multiple rotors just use 1 sensor total where
 * the sensor reports the slowest speed of all of the rotors.
 *
 * A rotor's speed is set by writing the Target value of a sensor.
 * Sometimes each sensor in a fan supports having a Target, and other
 * times not all of them do.  A TachSensor object knows if it supports
 * the Target property.
 *
 * The strategy for monitoring fan speeds is as follows:
 *
 * Every time a Target (new speed written) or Input (actual speed read)
 * sensor changes, check if the input value is within some range of the target
 * value.  If it isn't, start a timer at the end of which the sensor will be
 * set to not functional.  If enough sensors in the fan are now nonfunctional,
 * set the whole fan to nonfunctional in the inventory.
 *
 * When sensor inputs come back within a specified range of the target,
 * stop its timer if running, make the sensor functional again if it wasn't,
 * and if enough sensors in the fan are now functional set the whole fan
 * back to functional in the inventory.
 */
class Fan
{
    using Property = std::string;
    using Value = std::variant<bool>;
    using PropertyMap = std::map<Property, Value>;

    using Interface = std::string;
    using InterfaceMap = std::map<Interface, PropertyMap>;

    using Object = sdbusplus::message::object_path;
    using ObjectMap = std::map<Object, InterfaceMap>;

  public:
    Fan() = delete;
    Fan(const Fan&) = delete;
    Fan(Fan&&) = default;
    Fan& operator=(const Fan&) = delete;
    Fan& operator=(Fan&&) = default;
    ~Fan() = default;

    /**
     * @brief Constructor
     *
     * @param mode - mode of fan monitor
     * @param bus - the dbus object
     * @param event - event loop reference
     * @param trust - the tach trust manager
     * @param def - the fan definition structure
     */
    Fan(Mode mode, sdbusplus::bus::bus& bus, const sdeventplus::Event& event,
        std::unique_ptr<trust::Manager>& trust, const FanDefinition& def);

    /**
     * @brief Callback function for when an input sensor changes
     *
     * Starts a timer, where if it expires then the sensor
     * was out of range for too long and can be considered not functional.
     */
    void tachChanged(TachSensor& sensor);

    /**
     * @brief Calls tachChanged(sensor) on each sensor
     */
    void tachChanged();

    /**
     * @brief The callback function for the timer
     *
     * Sets the sensor to not functional.
     * If enough sensors are now not functional,
     * updates the functional status of the whole
     * fan in the inventory.
     *
     * @param[in] sensor - the sensor whose timer expired
     */
    void timerExpired(TachSensor& sensor);

    /**
     * @brief Get the name of the fan
     *
     * @return - The fan name
     */
    inline const std::string& getName() const
    {
        return _name;
    }

    /**
     * @brief Finds the target speed of this fan
     *
     * Finds the target speed from the list of sensors that make up this
     * fan. At least one sensor should contain a target speed value.
     *
     * @return - The target speed found from the list of sensors on the fan
     */
    uint64_t findTargetSpeed();

  private:
    /**
     * @brief Returns true if the sensor input is not within
     * some deviation of the target.
     *
     * @param[in] sensor - the sensor to check
     */
    bool outOfRange(const TachSensor& sensor);

    /**
     * @brief Returns true if too many sensors are nonfunctional
     *        as defined by _numSensorFailsForNonFunc
     */
    bool tooManySensorsNonfunctional();

    /**
     * @brief Updates the Functional property in the inventory
     *        for the fan based on the value passed in.
     *
     * @param[in] functional - If the Functional property should
     *                         be set to true or false.
     */
    void updateInventory(bool functional);

    /**
     * @brief the dbus object
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The inventory name of the fan
     */
    const std::string _name;

    /**
     * @brief The percentage that the input speed must be below
     *        the target speed to be considered an error.
     *        Between 0 and 100.
     */
    const size_t _deviation;

    /**
     * The number of sensors that must be nonfunctional at the
     * same time in order for the fan to be set to nonfunctional
     * in the inventory.
     */
    const size_t _numSensorFailsForNonFunc;

    /**
     * @brief The current functional state of the fan
     */
    bool _functional = true;

    /**
     * The sensor objects for the fan
     */
    std::vector<std::shared_ptr<TachSensor>> _sensors;

    /**
     * The tach trust manager object
     */
    std::unique_ptr<trust::Manager>& _trustManager;
};

} // namespace monitor
} // namespace fan
} // namespace phosphor
