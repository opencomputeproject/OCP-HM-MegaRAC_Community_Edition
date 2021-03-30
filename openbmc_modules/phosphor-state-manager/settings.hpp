#pragma once

#include <sdbusplus/bus.hpp>

#include <string>

namespace settings
{

using Path = std::string;
using Service = std::string;
using Interface = std::string;

constexpr auto root = "/";
constexpr auto autoRebootIntf = "xyz.openbmc_project.Control.Boot.RebootPolicy";
constexpr auto powerRestoreIntf =
    "xyz.openbmc_project.Control.Power.RestorePolicy";

/** @class Objects
 *  @brief Fetch paths of settings d-bus objects of interest, upon construction
 */
struct Objects
{
  public:
    /** @brief Constructor - fetch settings objects
     *
     * @param[in] bus - The Dbus bus object
     */
    explicit Objects(sdbusplus::bus::bus& bus);
    Objects(const Objects&) = delete;
    Objects& operator=(const Objects&) = delete;
    Objects(Objects&&) = delete;
    Objects& operator=(Objects&&) = delete;
    ~Objects() = default;

    /** @brief Fetch d-bus service, given a path and an interface. The
     *         service can't be cached because mapper returns unique
     *         service names.
     *
     * @param[in] path - The Dbus object
     * @param[in] interface - The Dbus interface
     *
     * @return std::string - the dbus service name
     */
    Service service(const Path& path, const Interface& interface) const;

    /** @brief host auto_reboot settings object */
    Path autoReboot;

    /** @brief host power_restore_policy settings object */
    Path powerRestorePolicy;

    /** @brief The Dbus bus object */
    sdbusplus::bus::bus& bus;
};

} // namespace settings
