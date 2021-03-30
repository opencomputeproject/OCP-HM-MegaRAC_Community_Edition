#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>
#include <xyz/openbmc_project/Network/SystemConfiguration/server.hpp>

namespace phosphor
{
namespace network
{

using SystemConfigIntf =
    sdbusplus::xyz::openbmc_project::Network::server::SystemConfiguration;

using Iface = sdbusplus::server::object::object<SystemConfigIntf>;

class Manager; // forward declaration of network manager.

/** @class SystemConfiguration
 *  @brief Network system configuration.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Network.SystemConfiguration DBus API.
 */
class SystemConfiguration : public Iface
{
  public:
    SystemConfiguration() = default;
    SystemConfiguration(const SystemConfiguration&) = delete;
    SystemConfiguration& operator=(const SystemConfiguration&) = delete;
    SystemConfiguration(SystemConfiguration&&) = delete;
    SystemConfiguration& operator=(SystemConfiguration&&) = delete;
    virtual ~SystemConfiguration() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] parent - Parent object.
     */
    SystemConfiguration(sdbusplus::bus::bus& bus, const std::string& objPath,
                        Manager& parent);

    /** @brief set the hostname of the system.
     *  @param[in] name - host name of the system.
     */
    std::string hostName(std::string name) override;

    /** @brief set the default v4 gateway of the system.
     *  @param[in] gateway - default v4 gateway of the system.
     */
    std::string defaultGateway(std::string gateway) override;

    using SystemConfigIntf::defaultGateway;

    /** @brief set the default v6 gateway of the system.
     *  @param[in] gateway - default v6 gateway of the system.
     */
    std::string defaultGateway6(std::string gateway) override;

    using SystemConfigIntf::defaultGateway6;

  private:
    /** @brief get the hostname from the system by doing
     *         dbus call to hostnamed service.
     */
    std::string getHostNameFromSystem() const;

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Network Manager object. */
    Manager& manager;
};

} // namespace network
} // namespace phosphor
