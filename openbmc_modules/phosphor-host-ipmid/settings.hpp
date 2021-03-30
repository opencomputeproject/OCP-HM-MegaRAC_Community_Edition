#pragma once

#include <sdbusplus/bus.hpp>
#include <string>
#include <tuple>

namespace settings
{

using Path = std::string;
using Service = std::string;
using Interface = std::string;

constexpr auto root = "/";

/** @class Objects
 *  @brief Fetch paths of settings d-bus objects of interest, upon construction
 */
struct Objects
{
  public:
    /** @brief Constructor - fetch settings objects
     *
     * @param[in] bus - The Dbus bus object
     * @param[in] filter - A vector of settings interfaces the caller is
     *            interested in.
     */
    Objects(sdbusplus::bus::bus& bus, const std::vector<Interface>& filter);
    Objects(const Objects&) = default;
    Objects& operator=(const Objects&) = default;
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
     * @return std::string - the dbus service
     */
    Service service(const Path& path, const Interface& interface) const;

    /** @brief map of settings objects */
    std::map<Interface, std::vector<Path>> map;

    /** @brief The Dbus bus object */
    sdbusplus::bus::bus& bus;
};

namespace boot
{

using OneTimeEnabled = bool;

/** @brief Return the one-time boot setting object path if enabled, otherwise
 *         the regular boot setting object path.
 *
 * @param[in] objects - const reference to an object of type Objects
 * @param[in] iface - boot setting interface
 *
 * @return A tuple - boot setting object path, a bool indicating whether the
 *                   returned path corresponds to the one time boot setting.
 */
std::tuple<Path, OneTimeEnabled> setting(const Objects& objects,
                                         const Interface& iface);

} // namespace boot

} // namespace settings
