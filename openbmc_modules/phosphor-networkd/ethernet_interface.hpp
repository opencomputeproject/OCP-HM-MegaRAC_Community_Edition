#pragma once

#include "types.hpp"
#include "util.hpp"
#include "xyz/openbmc_project/Network/IP/Create/server.hpp"
#include "xyz/openbmc_project/Network/Neighbor/CreateStatic/server.hpp"

#include <filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>
#include <xyz/openbmc_project/Collection/DeleteAll/server.hpp>
#include <xyz/openbmc_project/Network/EthernetInterface/server.hpp>
#include <xyz/openbmc_project/Network/MACAddress/server.hpp>

namespace phosphor
{
namespace network
{

using Ifaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Network::server::EthernetInterface,
    sdbusplus::xyz::openbmc_project::Network::server::MACAddress,
    sdbusplus::xyz::openbmc_project::Network::IP::server::Create,
    sdbusplus::xyz::openbmc_project::Network::Neighbor::server::CreateStatic,
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll>;

using IP = sdbusplus::xyz::openbmc_project::Network::server::IP;

using EthernetInterfaceIntf =
    sdbusplus::xyz::openbmc_project::Network::server::EthernetInterface;
using MacAddressIntf =
    sdbusplus::xyz::openbmc_project::Network::server::MACAddress;

using ServerList = std::vector<std::string>;
using ObjectPath = sdbusplus::message::object_path;

namespace fs = std::filesystem;

class Manager; // forward declaration of network manager.

class TestEthernetInterface;

class VlanInterface;

class IPAddress;

class Neighbor;

using LinkSpeed = uint16_t;
using DuplexMode = uint8_t;
using Autoneg = uint8_t;
using LinkUp = bool;
using NICEnabled = bool;
using VlanId = uint32_t;
using InterfaceName = std::string;
using InterfaceInfo =
    std::tuple<LinkSpeed, DuplexMode, Autoneg, LinkUp, NICEnabled>;
using AddressMap = std::map<std::string, std::shared_ptr<IPAddress>>;
using NeighborMap = std::map<std::string, std::shared_ptr<Neighbor>>;
using VlanInterfaceMap =
    std::map<InterfaceName, std::unique_ptr<VlanInterface>>;

/** @class EthernetInterface
 *  @brief OpenBMC Ethernet Interface implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Network.EthernetInterface DBus API.
 */
class EthernetInterface : public Ifaces
{
  public:
    EthernetInterface() = delete;
    EthernetInterface(const EthernetInterface&) = delete;
    EthernetInterface& operator=(const EthernetInterface&) = delete;
    EthernetInterface(EthernetInterface&&) = delete;
    EthernetInterface& operator=(EthernetInterface&&) = delete;
    virtual ~EthernetInterface() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] dhcpEnabled - is dhcp enabled(true/false).
     *  @param[in] parent - parent object.
     *  @param[in] emitSignal - true if the object added signal needs to be
     *                          send.
     */
    EthernetInterface(sdbusplus::bus::bus& bus, const std::string& objPath,
                      DHCPConf dhcpEnabled, Manager& parent,
                      bool emitSignal = true);

    /** @brief Function used to load the nameservers.
     */
    virtual void loadNameServers();

    /** @brief Function to create ipaddress dbus object.
     *  @param[in] addressType - Type of ip address.
     *  @param[in] ipaddress- IP address.
     *  @param[in] prefixLength - Length of prefix.
     *  @param[in] gateway - Gateway ip address.
     */

    ObjectPath iP(IP::Protocol addressType, std::string ipaddress,
                  uint8_t prefixLength, std::string gateway) override;

    /** @brief Function to create static neighbor dbus object.
     *  @param[in] ipAddress - IP address.
     *  @param[in] macAddress - Low level MAC address.
     */
    ObjectPath neighbor(std::string iPAddress, std::string mACAddress) override;

    /* @brief delete the dbus object of the given ipaddress.
     * @param[in] ipaddress - IP address.
     */
    void deleteObject(const std::string& ipaddress);

    /* @brief delete the dbus object of the given ipaddress.
     * @param[in] ipaddress - IP address.
     */
    void deleteStaticNeighborObject(const std::string& ipAddress);

    /* @brief delete the vlan dbus object of the given interface.
     *        Also deletes the device file and the network file.
     * @param[in] interface - VLAN Interface.
     */
    void deleteVLANObject(const std::string& interface);

    /* @brief creates the dbus object(IPaddres) given in the address list.
     * @param[in] addrs - address list for which dbus objects needs
     *                    to create.
     */
    void createIPAddressObjects();

    /* @brief creates the dbus object(Neighbor) given in the neighbor list.
     */
    void createStaticNeighborObjects();

    /* @brief Gets the index of the interface on the system
     */
    unsigned ifIndex() const;

    /* @brief Gets all the ip addresses.
     * @returns the list of ipaddress.
     */
    const AddressMap& getAddresses() const
    {
        return addrs;
    }

    /* @brief Gets all the static neighbor entries.
     * @returns Static neighbor map.
     */
    const NeighborMap& getStaticNeighbors() const
    {
        return staticNeighbors;
    }

    /** Set value of DHCPEnabled */
    DHCPConf dHCPEnabled(DHCPConf value) override;

    /** @brief Selectively disables DHCP
     *  @param[in] protocol - The IPv4 or IPv6 protocol to return to static
     *                        addressing mode
     */
    void disableDHCP(IP::Protocol protocol);

    /** Retrieve Link State */
    bool linkUp() const override;

    /** Retrieve NIC State */
    bool nICEnabled() const override;

    /** Set value of NICEnabled */
    bool nICEnabled(bool value) override;

    /** @brief sets the MAC address.
     *  @param[in] value - MAC address which needs to be set on the system.
     *  @returns macAddress of the interface or throws an error.
     */
    std::string mACAddress(std::string value) override;

    /** @brief get the IPv6AcceptRA flag from the network configuration file
     *
     */
    bool getIPv6AcceptRAFromConf();

    /** @brief check conf file for Router Advertisements
     *
     */
    bool iPv6AcceptRA(bool value) override;

    /** @brief sets the NTP servers.
     *  @param[in] value - vector of NTP servers.
     */
    ServerList nTPServers(ServerList value) override;

    /** @brief sets the DNS/nameservers.
     *  @param[in] value - vector of DNS servers.
     */
    ServerList nameservers(ServerList value) override;

    /** @brief sets the Static DNS/nameservers.
     *  @param[in] value - vector of DNS servers.
     */

    ServerList staticNameServers(ServerList value) override;

    /** @brief create Vlan interface.
     *  @param[in] id- VLAN identifier.
     */
    ObjectPath createVLAN(VlanId id);

    /** @brief load the vlan info from the system
     *         and creates the ip address dbus objects.
     *  @param[in] vlanID- VLAN identifier.
     */
    void loadVLAN(VlanId vlanID);

    /** @brief write the network conf file with the in-memory objects.
     */
    void writeConfigurationFile();

    /** @brief delete all dbus objects.
     */
    void deleteAll();

    using EthernetInterfaceIntf::dHCPEnabled;
    using EthernetInterfaceIntf::interfaceName;
    using EthernetInterfaceIntf::linkUp;
    using EthernetInterfaceIntf::nICEnabled;
    using MacAddressIntf::mACAddress;

    /** @brief Absolute path of the resolv conf file */
    static constexpr auto resolvConfFile = "/etc/resolv.conf";

  protected:
    /** @brief get the info of the ethernet interface.
     *  @return tuple having the link speed,autonegotiation,duplexmode .
     */
    InterfaceInfo getInterfaceInfo() const;

    /* @brief delete the vlan interface from system.
     * @param[in] interface - vlan Interface.
     */
    void deleteVLANFromSystem(const std::string& interface);

    /** @brief get the mac address of the interface.
     *  @param[in] interfaceName - Network interface name.
     *  @return macaddress on success
     */

    std::string getMACAddress(const std::string& interfaceName) const;

    /** @brief construct the ip address dbus object path.
     *  @param[in] addressType - Type of ip address.
     *  @param[in] ipaddress - IP address.
     *  @param[in] prefixLength - Length of prefix.
     *  @param[in] gateway - Gateway address.

     *  @return path of the address object.
     */

    std::string generateObjectPath(IP::Protocol addressType,
                                   const std::string& ipaddress,
                                   uint8_t prefixLength,
                                   const std::string& gateway) const;

    std::string
        generateStaticNeighborObjectPath(const std::string& iPAddress,
                                         const std::string& mACAddress) const;

    /** @brief generates the id by doing hash of ipaddress,
     *         prefixlength and the gateway.
     *  @param[in] ipaddress - IP address.
     *  @param[in] prefixLength - Length of prefix.
     *  @param[in] gateway - Gateway address.
     *  @return hash string.
     */

    static std::string generateId(const std::string& ipaddress,
                                  uint8_t prefixLength,
                                  const std::string& gateway);

    /** @brief generates the id by doing hash of ipaddress
     *         and the mac address.
     *  @param[in] iPAddress  - IP address.
     *  @param[in] mACAddress - Gateway address.
     *  @return hash string.
     */
    static std::string generateNeighborId(const std::string& iPAddress,
                                          const std::string& mACAddress);

    /** @brief write the dhcp section **/
    void writeDHCPSection(std::fstream& stream);

    /** @brief get the NTP server list from the network conf
     *
     */
    ServerList getNTPServersFromConf();

    /** @brief get the name server details from the network conf
     *
     */
    virtual ServerList getNameServerFromResolvd();
    ServerList getstaticNameServerFromConf();

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief Network Manager object. */
    Manager& manager;

    /** @brief Persistent map of IPAddress dbus objects and their names */
    AddressMap addrs;

    /** @brief Persistent map of Neighbor dbus objects and their names */
    NeighborMap staticNeighbors;

    /** @brief Persistent map of VLAN interface dbus objects and their names */
    VlanInterfaceMap vlanInterfaces;

    /** @brief Dbus object path */
    std::string objPath;

    friend class TestEthernetInterface;

  private:
    /** @brief Determines if DHCP is active for the IP::Protocol supplied.
     *  @param[in] protocol - Either IPv4 or IPv6
     *  @param[in] ignoreProtocol - Allows IPv4 and IPv6 to be checked using a
     *                              single call.
     *  @returns true/false value if DHCP is active for the input protocol
     */
    bool dhcpIsEnabled(IP::Protocol protocol, bool ignoreProtocol = false);

    /** @brief Determines if DHCP will be active following next reconfig
     *  @param[in] protocol - Either IPv4 or IPv6
     *  @param[in] nextDHCPState - The new DHCP mode to take affect
     *  @returns true/false value if DHCP is active for the input protocol
     */
    bool dhcpToBeEnabled(IP::Protocol family, const std::string& nextDHCPState);

    /** @brief Determines if the address is manually assigned
     *  @param[in] origin - The origin entry of the IP::Address
     *  @returns true/false value if the address is static
     */
    bool originIsManuallyAssigned(IP::AddressOrigin origin);
};

} // namespace network
} // namespace phosphor
