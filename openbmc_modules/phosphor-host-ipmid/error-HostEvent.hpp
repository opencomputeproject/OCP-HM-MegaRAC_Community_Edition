#pragma once

#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace org
{
namespace open_power
{
namespace Host
{
namespace Error
{

struct Event final : public sdbusplus::exception_t
{
    static constexpr auto errName = "org.open_power.Host.Error.Event";
    static constexpr auto errDesc = "A host system event was received";
    static constexpr auto errWhat =
        "org.open_power.Host.Error.Event: A host system event was received";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct MaintenanceProcedure final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "org.open_power.Host.Error.MaintenanceProcedure";
    static constexpr auto errDesc =
        "A host system event with a procedure callout";
    static constexpr auto errWhat =
        "org.open_power.Host.Error.MaintenanceProcedure: A host system event "
        "with a procedure callout";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace Error
} // namespace Host
} // namespace open_power
} // namespace org
} // namespace sdbusplus
