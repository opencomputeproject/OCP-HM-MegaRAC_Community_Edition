#include "fan_pwm.hpp"

#include "env.hpp"
#include "hwmon.hpp"
#include "hwmonio.hpp"
#include "sensorset.hpp"
#include "sysfs.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <phosphor-logging/elog-errors.hpp>
#include <string>
#include <xyz/openbmc_project/Control/Device/error.hpp>

using namespace phosphor::logging;

namespace hwmon
{

uint64_t FanPwm::target(uint64_t value)
{
    using namespace std::literals;

    std::string empty;
    // Write target out to sysfs
    try
    {
        _ioAccess->write(value, _type, _id, empty, hwmonio::retries,
                         hwmonio::delay);
    }
    catch (const std::system_error& e)
    {
        using namespace sdbusplus::xyz::openbmc_project::Control::Device::Error;
        report<WriteFailure>(
            xyz::openbmc_project::Control::Device::WriteFailure::CALLOUT_ERRNO(
                e.code().value()),
            xyz::openbmc_project::Control::Device::WriteFailure::
                CALLOUT_DEVICE_PATH(_devPath.c_str()));

        auto file =
            sysfs::make_sysfs_path(_ioAccess->path(), _type, _id, empty);

        log<level::INFO>(fmt::format("Failing sysfs file: {} errno: {}", file,
                                     e.code().value())
                             .c_str());

        exit(EXIT_FAILURE);
    }

    return FanPwmObject::target(value);
}

} // namespace hwmon
