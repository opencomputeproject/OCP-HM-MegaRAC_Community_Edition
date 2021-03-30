#include "fan_speed.hpp"

#include "env.hpp"
#include "hwmon.hpp"
#include "hwmonio.hpp"
#include "sensorset.hpp"
#include "sysfs.hpp"

#include <fmt/format.h>

#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Control/Device/error.hpp>

using namespace phosphor::logging;

namespace hwmon
{

uint64_t FanSpeed::target(uint64_t value)
{
    auto curValue = FanSpeedObject::target();

    if (curValue != value)
    {
        // Write target out to sysfs
        try
        {
            _ioAccess->write(value, _type, _id, entry::target, hwmonio::retries,
                             hwmonio::delay);
        }
        catch (const std::system_error& e)
        {
            using namespace sdbusplus::xyz::openbmc_project::Control::Device::
                Error;
            report<WriteFailure>(
                xyz::openbmc_project::Control::Device::WriteFailure::
                    CALLOUT_ERRNO(e.code().value()),
                xyz::openbmc_project::Control::Device::WriteFailure::
                    CALLOUT_DEVICE_PATH(_devPath.c_str()));

            auto file = sysfs::make_sysfs_path(_ioAccess->path(), _type, _id,
                                               entry::target);

            log<level::INFO>(fmt::format("Failing sysfs file: {} errno: {}",
                                         file, e.code().value())
                                 .c_str());

            exit(EXIT_FAILURE);
        }
    }

    return FanSpeedObject::target(value);
}

void FanSpeed::enable()
{
    auto enable = env::getEnv("ENABLE", _type, _id);
    if (!enable.empty())
    {
        auto val = std::stoul(enable);

        try
        {
            _ioAccess->write(val, type::pwm, _id, entry::enable,
                             hwmonio::retries, hwmonio::delay);
        }
        catch (const std::system_error& e)
        {
            using namespace sdbusplus::xyz::openbmc_project::Control::Device::
                Error;
            phosphor::logging::report<WriteFailure>(
                xyz::openbmc_project::Control::Device::WriteFailure::
                    CALLOUT_ERRNO(e.code().value()),
                xyz::openbmc_project::Control::Device::WriteFailure::
                    CALLOUT_DEVICE_PATH(_devPath.c_str()));

            auto fullPath = sysfs::make_sysfs_path(_ioAccess->path(), type::pwm,
                                                   _id, entry::enable);

            log<level::INFO>(fmt::format("Failing sysfs file: {} errno: {}",
                                         fullPath, e.code().value())
                                 .c_str());

            exit(EXIT_FAILURE);
        }
    }
}

} // namespace hwmon
