#pragma once

#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace xyz
{
namespace openbmc_project
{
namespace Smbios
{
namespace MDR_V2
{
namespace Error
{

struct InvalidParameter final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Smbios.MDR_V2.Error.InvalidParameter";
    static constexpr auto errDesc = "An invalid parameter is attempted.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.Smbios.MDR_V2.Error.InvalidParameter: An invalid "
        "parameter is attempted.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct UpdateInProgress final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Smbios.MDR_V2.Error.UpdateInProgress";
    static constexpr auto errDesc = "Update is in progress.";
    static constexpr auto errWhat = "xyz.openbmc_project.Smbios.MDR_V2.Error."
                                    "UpdateInProgress: Update is in progress.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

struct InvalidId final : public sdbusplus::exception_t
{
    static constexpr auto errName =
        "xyz.openbmc_project.Smbios.MDR_V2.Error.InvalidId";
    static constexpr auto errDesc = "An invalid Id is attempted.";
    static constexpr auto errWhat = "xyz.openbmc_project.Smbios.MDR_V2.Error."
                                    "InvalidId: An invalid Id is attempted.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace Error
} // namespace MDR_V2
} // namespace Smbios
} // namespace openbmc_project
} // namespace xyz
} // namespace sdbusplus
