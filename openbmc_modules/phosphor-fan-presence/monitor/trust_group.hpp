#pragma once
#include "tach_sensor.hpp"

#include <memory>

namespace phosphor
{
namespace fan
{
namespace trust
{

constexpr auto sensorName = 0;
constexpr auto inTrust = 1;
using GroupDefinition = std::tuple<std::string, bool>;

struct GroupSensor
{
    std::shared_ptr<monitor::TachSensor> sensor;
    bool inTrust;
};

/**
 * @class Group
 *
 * An abstract sensor trust group base class.
 *
 * Supports the ability to know if a fan speed sensor value can
 * be trusted or not, where if it isn't trusted then it shouldn't
 * be used to determine if the fan is faulted or not.
 *
 * It's a group in that there can be multiple sensors in the group
 * and the trust of all sensors depends on something about those sensors.
 * For example, if all sensors in the group report a speed of zero,
 * then no sensor in the group is trusted.  All sensors in the group
 * have the same trust value.
 *
 * Trust is calculated when checkTrust() is called after a group
 * sensor's tach value changes.
 *
 * A derived class must override checkGroupTrust().
 */
class Group
{
  public:
    Group() = delete;
    virtual ~Group() = default;
    Group(const Group&) = delete;
    Group& operator=(const Group&) = delete;
    Group(Group&&) = default;
    Group& operator=(Group&&) = default;

    /**
     * Constructor
     *
     * @param[in] names - the names and inclusion of sensors in the group
     */
    explicit Group(const std::vector<GroupDefinition>& names) : _names(names)
    {}

    /**
     * Used to register a TachSensor object with the group.
     * It's only added to the group if the sensor's name is
     * in the group's list of names.
     *
     * @param[in] sensor - the TachSensor to register
     */
    void registerSensor(std::shared_ptr<monitor::TachSensor>& sensor)
    {
        auto found = std::find_if(
            _names.begin(), _names.end(), [&sensor](const auto& name) {
                return monitor::FAN_SENSOR_PATH + std::get<sensorName>(name) ==
                       sensor->name();
            });

        if (found != _names.end())
        {
            _sensors.push_back({sensor, std::get<inTrust>(*found)});
        }
    }

    /**
     * Says if a sensor belongs to the group.
     *
     * After all sensors have registered, this can be
     * used to say if a TachSensor is in the group.
     *
     * @param[in] sensor - the TachSensor object
     */
    bool inGroup(const monitor::TachSensor& sensor)
    {
        return (std::find_if(_sensors.begin(), _sensors.end(),
                             [&sensor](const auto& s) {
                                 return sensor.name() == s.sensor->name();
                             }) != _sensors.end());
    }

    /**
     * Stops the timers on all sensors in the group.
     *
     * Called when the group just changed to not trusted,
     * so that its sensors' timers can't fire a callback
     * that may cause them to be considered faulted.
     */
    void stopTimers()
    {
        std::for_each(_sensors.begin(), _sensors.end(),
                      [](const auto& s) { s.sensor->stopTimer(); });
    }

    /**
     * Starts the timers on all functional sensors in the group if
     * their target and input values do not match.
     *
     * Called when the group just changed to trusted.
     */
    void startTimers()
    {
        std::for_each(_sensors.begin(), _sensors.end(), [](const auto& s) {
            // If a sensor isn't functional, then its timer
            // already expired so don't bother starting it again
            if (s.sensor->functional() &&
                static_cast<uint64_t>(s.sensor->getInput()) !=
                    s.sensor->getTarget())
            {
                s.sensor->startTimer(
                    phosphor::fan::monitor::TimerMode::nonfunc);
            }
        });
    }

    /**
     * Determines the trust for this group based on this
     * sensor's latest status.
     *
     * Calls the derived class's checkGroupTrust function
     * and updates the class with the results.
     *
     * If this is called with a sensor not in the group,
     * it will be considered trusted.
     *
     * @param[in] sensor - TachSensor object
     *
     * @return tuple<bool, bool> -
     *   field 0 - the trust value
     *   field 1 - if that trust value changed since last call
     *             to checkTrust
     */
    auto checkTrust(const monitor::TachSensor& sensor)
    {
        if (inGroup(sensor))
        {
            auto trust = checkGroupTrust();

            setTrust(trust);

            return std::tuple<bool, bool>(_trusted, _stateChange);
        }
        return std::tuple<bool, bool>(true, false);
    }

    /**
     * Says if all sensors in the group are currently trusted,
     * as determined by the last call to checkTrust().
     *
     * @return bool - if the group's sensors are trusted or not
     */
    inline auto getTrust() const
    {
        return _trusted;
    }

    /**
     * Says if the trust value changed in the last call to
     * checkTrust()
     *
     * @return bool - if the trust changed or not
     */
    inline auto trustChanged() const
    {
        return _stateChange;
    }

  protected:
    /**
     * The sensor objects and their trust inclusion in the group.
     *
     * Added by registerSensor().
     */
    std::vector<GroupSensor> _sensors;

  private:
    /**
     * Checks if the group's sensors are trusted.
     *
     * The derived class must override this function
     * to provide custom functionality.
     *
     * @return bool - if group is trusted or not
     */
    virtual bool checkGroupTrust() = 0;

    /**
     * Sets the trust value on the object.
     *
     * @param[in] trust - the new trust value
     */
    inline void setTrust(bool trust)
    {
        _stateChange = (trust != _trusted);
        _trusted = trust;
    }

    /**
     * The current trust state of the group
     */
    bool _trusted = true;

    /**
     * If the trust value changed in the last call to checkTrust
     */
    bool _stateChange = false;

    /**
     * The names of the sensors and whether it is included in
     * determining trust for this group
     */
    const std::vector<GroupDefinition> _names;
};

} // namespace trust
} // namespace fan
} // namespace phosphor
