#pragma once

#include "hwmonio.hpp"
#include "interface.hpp"
#include "sysfs.hpp"

#include <memory>

namespace hwmon
{

/**
 * @class FanSpeed
 * @brief Target fan speed control implementation
 * @details Derived FanSpeedObject type that writes the target value to sysfs
 * which in turn sets the fan speed to that target value
 */
class FanSpeed : public FanSpeedObject
{
  public:
    /**
     * @brief Constructs FanSpeed Object
     *
     * @param[in] io -  HwmonIO(instance path) (ex /sys/class/hwmon/hwmon1)
     * @param[in] devPath - The /sys/devices sysfs path
     * @param[in] id - The hwmon id
     * @param[in] bus - Dbus bus object
     * @param[in] objPath - Dbus object path
     * @param[in] defer - Dbus object registration defer
     * @param[in] target - initial target speed value
     */
    FanSpeed(std::unique_ptr<hwmonio::HwmonIOInterface> io,
             const std::string& devPath, const std::string& id,
             sdbusplus::bus::bus& bus, const char* objPath, bool defer,
             uint64_t target) :
        FanSpeedObject(bus, objPath, defer),
        _id(id), _ioAccess(std::move(io)), _devPath(devPath)
    {
        FanSpeedObject::target(target);
    }

    /**
     * @brief Set the value of target
     *
     * @return Value of target
     */
    uint64_t target(uint64_t value) override;

    /**
     * @brief Writes the pwm_enable sysfs entry if the
     *        env var with the value to write is present
     */
    void enable();

  private:
    /** @brief hwmon type */
    static constexpr auto _type = "fan";
    /** @brief hwmon id */
    std::string _id;
    /** @brief Hwmon sysfs access. */
    std::unique_ptr<hwmonio::HwmonIOInterface> _ioAccess;
    /** @brief Physical device path. */
    std::string _devPath;
};

} // namespace hwmon
