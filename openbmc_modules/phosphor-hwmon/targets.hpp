#pragma once

#include "env.hpp"
#include "fan_pwm.hpp"
#include "fan_speed.hpp"
#include "hwmonio.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <memory>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Sensor/Device/error.hpp>

enum class targetType
{
    DEFAULT,
    RPM,
    PWM
};

static constexpr auto RPM_TARGET = "RPM";
static constexpr auto PWM_TARGET = "PWM";

/** @class Targets
 *  @brief Target type traits.
 *
 *  @tparam T - The target type.
 */
template <typename T>
struct Targets
{
    static void fail()
    {
        static_assert(sizeof(Targets) == -1, "Unsupported Target type");
    }
};

/**@brief Targets specialization for fan speed. */
template <>
struct Targets<hwmon::FanSpeed>
{
    static constexpr InterfaceType type = InterfaceType::FAN_SPEED;
};

template <>
struct Targets<hwmon::FanPwm>
{
    static constexpr InterfaceType type = InterfaceType::FAN_PWM;
};

/** @brief addTarget
 *
 *  Creates the target type interface
 *
 *  @tparam T - The target type
 *
 *  @param[in] sensor - A sensor type and name
 *  @param[in] ioAccess - hwmon sysfs access object
 *  @param[in] devPath - The /sys/devices sysfs path
 *  @param[in] info - The sdbusplus server connection and interfaces
 *
 *  @return A shared pointer to the target interface object
 *          Will be empty if no interface was created
 */
template <typename T>
std::shared_ptr<T> addTarget(const SensorSet::key_type& sensor,
                             const hwmonio::HwmonIOInterface* ioAccess,
                             const std::string& devPath, ObjectInfo& info)
{
    std::shared_ptr<T> target;
    namespace fs = std::filesystem;

    auto& obj = std::get<InterfaceMap>(info);
    auto& objPath = std::get<std::string>(info);
    auto type = Targets<T>::type;

    // Check if target sysfs file exists
    std::string sysfsFullPath;
    std::string targetName = sensor.first;
    std::string targetId = sensor.second;
    std::string entry = hwmon::entry::target;

    using namespace std::literals;
    const std::string pwm = "pwm"s;
    const std::string empty = ""s;

    if (InterfaceType::FAN_PWM == type)
    {
        targetName = pwm;
        // If PWM_TARGET is set, use the specified pwm id
        auto id = env::getEnv("PWM_TARGET", sensor);
        if (!id.empty())
        {
            targetId = id;
        }
        entry = empty;
    }

    sysfsFullPath =
        sysfs::make_sysfs_path(ioAccess->path(), targetName, targetId, entry);
    if (fs::exists(sysfsFullPath))
    {
        auto useTarget = true;
        auto tmEnv = env::getEnv("TARGET_MODE");
        if (!tmEnv.empty())
        {
            std::string mode{tmEnv};
            std::transform(mode.begin(), mode.end(), mode.begin(), toupper);

            if (mode == RPM_TARGET)
            {
                if (type != InterfaceType::FAN_SPEED)
                {
                    useTarget = false;
                }
            }
            else if (mode == PWM_TARGET)
            {
                if (type != InterfaceType::FAN_PWM)
                {
                    useTarget = false;
                }
            }
            else
            {
                using namespace phosphor::logging;
                log<level::ERR>(
                    "Invalid TARGET_MODE env var found",
                    phosphor::logging::entry("TARGET_MODE=%s", tmEnv.c_str()),
                    phosphor::logging::entry("DEVPATH=%s", devPath.c_str()));
            }
        }

        if (useTarget)
        {
            uint32_t targetSpeed = 0;

            try
            {
                targetSpeed = ioAccess->read(targetName, targetId, entry,
                                             hwmonio::retries, hwmonio::delay);
            }
            catch (const std::system_error& e)
            {
                using namespace phosphor::logging;
                using namespace sdbusplus::xyz::openbmc_project::Sensor::
                    Device::Error;
                using metadata =
                    xyz::openbmc_project::Sensor::Device::ReadFailure;

                report<ReadFailure>(
                    metadata::CALLOUT_ERRNO(e.code().value()),
                    metadata::CALLOUT_DEVICE_PATH(devPath.c_str()));

                log<level::INFO>(fmt::format("Failing sysfs file: {} errno: {}",
                                             sysfsFullPath, e.code().value())
                                     .c_str());
            }

            static constexpr bool deferSignals = true;
            auto& bus = *std::get<sdbusplus::bus::bus*>(info);

            // ioAccess->path() is a path like: /sys/class/hwmon/hwmon1
            // NOTE: When unit-testing, the target won't have an inject-ible
            // ioAccess: fan_pwm/fan_speed.
            target = std::make_shared<T>(
                std::move(std::make_unique<hwmonio::HwmonIO>(ioAccess->path())),
                devPath, targetId, bus, objPath.c_str(), deferSignals,
                targetSpeed);
            obj[type] = target;
        }
    }

    return target;
}
