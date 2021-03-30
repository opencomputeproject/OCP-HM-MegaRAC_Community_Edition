#pragma once

#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Common
{
namespace Error
{

struct Timeout final : public sdbusplus::exception_t
{
    static constexpr auto errName = "xyz.openbmc_project.Common.Error.Timeout";
    static constexpr auto errDesc = "Operation timed out.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Common.Error.Timeout: Operation timed out.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct InternalFailure final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Common.Error.InternalFailure";
    static constexpr auto errDesc = "The operation failed internally.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Common.Error.InternalFailure: The operation "
        "failed internally.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct InvalidArgument final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Common.Error.InvalidArgument";
    static constexpr auto errDesc = "Invalid argument was given.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Common.Error.InvalidArgument: Invalid argument "
        "was given.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct InsufficientPermission final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Common.Error.InsufficientPermission";
    static constexpr auto errDesc =
        "Insufficient permission to perform operation";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Common.Error.InsufficientPermission: Insufficient "
        "permission to perform operation";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace Error
} // namespace Common
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus
