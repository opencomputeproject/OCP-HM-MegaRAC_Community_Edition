#pragma once

#include "dhcp_configuration.hpp"
#include "ethernet_interface.hpp"
#include "system_configuration.hpp"
#include "vlan_interface.hpp"
#include "xyz/openbmc_project/Network/VLAN/Create/server.hpp"

#include <filesystem>
#include <list>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>
#include <vector>
#include <xyz/openbmc_project/Common/FactoryReset/server.hpp>

namespace phosphor
{
namespace network
{

using SystemConfPtr = std::unique_ptr<SystemConfiguration>;
using DHCPConfPtr = std::unique_ptr<dhcp::Configuration>;

namespace fs = std::filesystem;
namespace details
{

template <typename T, typename U>
using ServerObject = typename sdbusplus::server::object::object<T, U>;

using VLANCreateIface = details::ServerObject<
    sdbusplus::xyz::openbmc_project::Network::VLAN::server::Create,
    sdbusplus::xyz::openbmc_project::Common::server::FactoryReset>;

} // namespace details

/** @class Manager
 *  @brief OpenBMC network manager implementation.
 */
class Manager : public details::VLANCreateIface
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] dir - Network Configuration directory path.
     */
    Manager(sdbusplus::bus::bus& bus, const char* objPath,
            const std::string& dir);

    ObjectPath vLAN(IntfName interfaceName, uint32_t id) override;

    /** @brief write the network conf file with the in-memory objects.
     */
    void writeToConfigurationFile();

    /** @brief Fetch the interface and the ipaddress details
     *         from the system and create the ethernet interraces
     *         dbus object.
     */
    virtual void createInterfaces();

    /** @brief create child interface object and the system conf object.
     */
    void createChildObjects();

    /** @brief sets the network conf directory.
     *  @param[in] dirName - Absolute path of the directory.
     */
    void setConfDir(const fs::path& dir);

    /** @brief gets the network conf directory.
     */
    fs::path getConfDir()
    {
        return confDir;
    }

    /** @brief gets the system conf object.
     *
     */
    const SystemConfPtr& getSystemConf()
    {
        return systemConf;
    }

    /** @brief gets the dhcp conf object.
     *
     */
    const DHCPConfPtr& getDHCPConf()
    {
        return dhcpConf;
    }

    /** @brief create the default network files for each interface
     *  @detail if force param is true then forcefully create the network
     *          files otherwise if network file doesn't exist then
     *          create it.
     *  @param[in] force - forcefully create the file
     *  @return true if network file created else false
     */
    bool createDefaultNetworkFiles(bool force);

    /** @brief restart the network timers. */
    void restartTimers();

    /** @brief This function gets the MAC address from the VPD and
     *  sets it on the corresponding ethernet interface during first
     *  Boot, once it sets the MAC from VPD, it creates a file named
     *  firstBoot under /var/lib to make sure we dont run this function
     *  again.
     *
     *  @param[in] ethPair - Its a pair of ethernet interface name & the
     * corresponding MAC Address from the VPD
     *
     *  return - NULL
     */
    void setFistBootMACOnInterface(
        const std::pair<std::string, std::string>& ethPair);

    /** @brief Restart the systemd unit
     *  @param[in] unit - systemd unit name which needs to be
     *                    restarted.
     */
    virtual void restartSystemdUnit(const std::string& unit);

    /** @brief Returns the number of interfaces under this manager.
     *
     * @return the number of interfaces managed by this manager.
     */
    int getInterfaceCount()
    {
        return interfaces.size();
    }

    /** @brief Does the requested interface exist under this manager?
     *
     * @param[in] intf - the interface name to check.
     * @return true if found, false otherwise.
     */
    bool hasInterface(const std::string& intf)
    {
        return (interfaces.find(intf) != interfaces.end());
    }

  protected:
    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Persistent map of EthernetInterface dbus objects and their names
     */
    std::map<IntfName, std::shared_ptr<EthernetInterface>> interfaces;

    /** @brief BMC network reset - resets network configuration for BMC. */
    void reset() override;

    /** @brief Path of Object. */
    std::string objectPath;

    /** @brief pointer to system conf object. */
    SystemConfPtr systemConf = nullptr;

    /** @brief pointer to dhcp conf object. */
    DHCPConfPtr dhcpConf = nullptr;

    /** @brief Network Configuration directory. */
    fs::path confDir;
};

} // namespace network
} // namespace phosphor
