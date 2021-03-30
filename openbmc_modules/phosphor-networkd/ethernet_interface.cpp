#include "config.h"

#include "ethernet_interface.hpp"

#include "config_parser.hpp"
#include "neighbor.hpp"
#include "network_manager.hpp"
#include "vlan_interface.hpp"

#include <arpa/inet.h>
#include <linux/ethtool.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <stdplus/raw.hpp>
#include <string>
#include <string_view>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using NotAllowedArgument = xyz::openbmc_project::Common::NotAllowed;
using Argument = xyz::openbmc_project::Common::InvalidArgument;
constexpr auto RESOLVED_SERVICE = "org.freedesktop.resolve1";
constexpr auto RESOLVED_INTERFACE = "org.freedesktop.resolve1.Link";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto RESOLVED_SERVICE_PATH = "/org/freedesktop/resolve1/link/";
constexpr auto METHOD_GET = "Get";

struct EthernetIntfSocket
{
    EthernetIntfSocket(int domain, int type, int protocol)
    {
        if ((sock = socket(domain, type, protocol)) < 0)
        {
            log<level::ERR>("socket creation failed:",
                            entry("ERROR=%s", strerror(errno)));
        }
    }

    ~EthernetIntfSocket()
    {
        if (sock >= 0)
        {
            close(sock);
        }
    }

    int sock{-1};
};

std::map<EthernetInterface::DHCPConf, std::string> mapDHCPToSystemd = {
    {EthernetInterface::DHCPConf::both, "true"},
    {EthernetInterface::DHCPConf::v4, "ipv4"},
    {EthernetInterface::DHCPConf::v6, "ipv6"},
    {EthernetInterface::DHCPConf::none, "false"}};

EthernetInterface::EthernetInterface(sdbusplus::bus::bus& bus,
                                     const std::string& objPath,
                                     DHCPConf dhcpEnabled, Manager& parent,
                                     bool emitSignal) :
    Ifaces(bus, objPath.c_str(), true),
    bus(bus), manager(parent), objPath(objPath)
{
    auto intfName = objPath.substr(objPath.rfind("/") + 1);
    std::replace(intfName.begin(), intfName.end(), '_', '.');
    interfaceName(intfName);
    EthernetInterfaceIntf::dHCPEnabled(dhcpEnabled);
    EthernetInterfaceIntf::iPv6AcceptRA(getIPv6AcceptRAFromConf());
    // Don't get the mac address from the system as the mac address
    // would be same as parent interface.
    if (intfName.find(".") == std::string::npos)
    {
        MacAddressIntf::mACAddress(getMACAddress(intfName));
    }
    EthernetInterfaceIntf::nTPServers(getNTPServersFromConf());

    EthernetInterfaceIntf::linkUp(linkUp());
    EthernetInterfaceIntf::nICEnabled(nICEnabled());

#if NIC_SUPPORTS_ETHTOOL
    InterfaceInfo ifInfo = EthernetInterface::getInterfaceInfo();

    EthernetInterfaceIntf::autoNeg(std::get<2>(ifInfo));
    EthernetInterfaceIntf::speed(std::get<0>(ifInfo));
#endif

    // Emit deferred signal.
    if (emitSignal)
    {
        this->emit_object_added();
    }
}

static IP::Protocol convertFamily(int family)
{
    switch (family)
    {
        case AF_INET:
            return IP::Protocol::IPv4;
        case AF_INET6:
            return IP::Protocol::IPv6;
    }

    throw std::invalid_argument("Bad address family");
}

void EthernetInterface::disableDHCP(IP::Protocol protocol)
{
    DHCPConf dhcpState = EthernetInterfaceIntf::dHCPEnabled();
    if (dhcpState == EthernetInterface::DHCPConf::both)
    {
        if (protocol == IP::Protocol::IPv4)
        {
            dHCPEnabled(EthernetInterface::DHCPConf::v6);
        }
        else if (protocol == IP::Protocol::IPv6)
        {
            dHCPEnabled(EthernetInterface::DHCPConf::v4);
        }
    }
    else if ((dhcpState == EthernetInterface::DHCPConf::v4) &&
             (protocol == IP::Protocol::IPv4))
    {
        dHCPEnabled(EthernetInterface::DHCPConf::none);
    }
    else if ((dhcpState == EthernetInterface::DHCPConf::v6) &&
             (protocol == IP::Protocol::IPv6))
    {
        dHCPEnabled(EthernetInterface::DHCPConf::none);
    }
}

bool EthernetInterface::dhcpIsEnabled(IP::Protocol family, bool ignoreProtocol)
{
    return ((EthernetInterfaceIntf::dHCPEnabled() ==
             EthernetInterface::DHCPConf::both) ||
            ((EthernetInterfaceIntf::dHCPEnabled() ==
              EthernetInterface::DHCPConf::v6) &&
             ((family == IP::Protocol::IPv6) || ignoreProtocol)) ||
            ((EthernetInterfaceIntf::dHCPEnabled() ==
              EthernetInterface::DHCPConf::v4) &&
             ((family == IP::Protocol::IPv4) || ignoreProtocol)));
}

bool EthernetInterface::dhcpToBeEnabled(IP::Protocol family,
                                        const std::string& nextDHCPState)
{
    return ((nextDHCPState == "true") ||
            ((nextDHCPState == "ipv6") && (family == IP::Protocol::IPv6)) ||
            ((nextDHCPState == "ipv4") && (family == IP::Protocol::IPv4)));
}

bool EthernetInterface::originIsManuallyAssigned(IP::AddressOrigin origin)
{
    return (
#ifdef LINK_LOCAL_AUTOCONFIGURATION
        (origin == IP::AddressOrigin::Static)
#else
        (origin == IP::AddressOrigin::Static ||
         origin == IP::AddressOrigin::LinkLocal)
#endif

    );
}

void EthernetInterface::createIPAddressObjects()
{
    addrs.clear();

    auto addrs = getInterfaceAddrs()[interfaceName()];

    for (auto& addr : addrs)
    {
        IP::Protocol addressType = convertFamily(addr.addrType);
        IP::AddressOrigin origin = IP::AddressOrigin::Static;
        if (dhcpIsEnabled(addressType))
        {
            origin = IP::AddressOrigin::DHCP;
        }
        if (isLinkLocalIP(addr.ipaddress))
        {
            origin = IP::AddressOrigin::LinkLocal;
        }
        // Obsolete parameter
        std::string gateway = "";

        std::string ipAddressObjectPath = generateObjectPath(
            addressType, addr.ipaddress, addr.prefix, gateway);

        this->addrs.emplace(addr.ipaddress,
                            std::make_shared<phosphor::network::IPAddress>(
                                bus, ipAddressObjectPath.c_str(), *this,
                                addressType, addr.ipaddress, origin,
                                addr.prefix, gateway));
    }
}

void EthernetInterface::createStaticNeighborObjects()
{
    staticNeighbors.clear();

    NeighborFilter filter;
    filter.interface = ifIndex();
    filter.state = NUD_PERMANENT;
    auto neighbors = getCurrentNeighbors(filter);
    for (const auto& neighbor : neighbors)
    {
        if (!neighbor.mac)
        {
            continue;
        }
        std::string ip = toString(neighbor.address);
        std::string mac = mac_address::toString(*neighbor.mac);
        std::string objectPath = generateStaticNeighborObjectPath(ip, mac);
        staticNeighbors.emplace(ip,
                                std::make_shared<phosphor::network::Neighbor>(
                                    bus, objectPath.c_str(), *this, ip, mac,
                                    Neighbor::State::Permanent));
    }
}

unsigned EthernetInterface::ifIndex() const
{
    unsigned idx = if_nametoindex(interfaceName().c_str());
    if (idx == 0)
    {
        throw std::system_error(errno, std::generic_category(),
                                "if_nametoindex");
    }
    return idx;
}

ObjectPath EthernetInterface::iP(IP::Protocol protType, std::string ipaddress,
                                 uint8_t prefixLength, std::string gateway)
{

    if (dhcpIsEnabled(protType))
    {
        log<level::INFO>("DHCP enabled on the interface"),
            entry("INTERFACE=%s", interfaceName().c_str());
        disableDHCP(protType);
    }

    IP::AddressOrigin origin = IP::AddressOrigin::Static;

    int addressFamily = (protType == IP::Protocol::IPv4) ? AF_INET : AF_INET6;

    if (!isValidIP(addressFamily, ipaddress))
    {
        log<level::ERR>("Not a valid IP address"),
            entry("ADDRESS=%s", ipaddress.c_str());
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("ipaddress"),
                              Argument::ARGUMENT_VALUE(ipaddress.c_str()));
    }

    // Gateway is an obsolete parameter
    gateway = "";

    if (!isValidPrefix(addressFamily, prefixLength))
    {
        log<level::ERR>("PrefixLength is not correct "),
            entry("PREFIXLENGTH=%" PRIu8, prefixLength);
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("prefixLength"),
            Argument::ARGUMENT_VALUE(std::to_string(prefixLength).c_str()));
    }

    std::string objectPath =
        generateObjectPath(protType, ipaddress, prefixLength, gateway);
    this->addrs.emplace(ipaddress,
                        std::make_shared<phosphor::network::IPAddress>(
                            bus, objectPath.c_str(), *this, protType, ipaddress,
                            origin, prefixLength, gateway));

    manager.writeToConfigurationFile();
    return objectPath;
}

ObjectPath EthernetInterface::neighbor(std::string iPAddress,
                                       std::string mACAddress)
{
    if (!isValidIP(AF_INET, iPAddress) && !isValidIP(AF_INET6, iPAddress))
    {
        log<level::ERR>("Not a valid IP address",
                        entry("ADDRESS=%s", iPAddress.c_str()));
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("iPAddress"),
                              Argument::ARGUMENT_VALUE(iPAddress.c_str()));
    }
    if (!mac_address::isUnicast(mac_address::fromString(mACAddress)))
    {
        log<level::ERR>("Not a valid MAC address",
                        entry("MACADDRESS=%s", iPAddress.c_str()));
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("mACAddress"),
                              Argument::ARGUMENT_VALUE(mACAddress.c_str()));
    }

    std::string objectPath =
        generateStaticNeighborObjectPath(iPAddress, mACAddress);
    staticNeighbors.emplace(iPAddress,
                            std::make_shared<phosphor::network::Neighbor>(
                                bus, objectPath.c_str(), *this, iPAddress,
                                mACAddress, Neighbor::State::Permanent));
    manager.writeToConfigurationFile();
    return objectPath;
}

#if NIC_SUPPORTS_ETHTOOL
/*
  Enable this code if your NIC driver supports the ETHTOOL features.
  Do this by adding the following to your phosphor-network*.bbappend file.
     EXTRA_OECONF_append = " --enable-nic-ethtool=yes"
  The default compile mode is to omit getInterfaceInfo()
*/
InterfaceInfo EthernetInterface::getInterfaceInfo() const
{
    ifreq ifr{0};
    ethtool_cmd edata{0};
    LinkSpeed speed{0};
    Autoneg autoneg{0};
    DuplexMode duplex{0};
    LinkUp linkState{false};
    NICEnabled nicEnabled{false};
    EthernetIntfSocket eifSocket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (eifSocket.sock < 0)
    {
        return std::make_tuple(speed, duplex, autoneg, linkState, nicEnabled);
    }

    std::strncpy(ifr.ifr_name, interfaceName().c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = reinterpret_cast<char*>(&edata);

    edata.cmd = ETHTOOL_GSET;
    if (ioctl(eifSocket.sock, SIOCETHTOOL, &ifr) >= 0)
    {
        speed = edata.speed;
        duplex = edata.duplex;
        autoneg = edata.autoneg;
    }

    nicEnabled = nICEnabled();
    linkState = linkUp();

    return std::make_tuple(speed, duplex, autoneg, linkState, nicEnabled);
}
#endif

/** @brief get the mac address of the interface.
 *  @return macaddress on success
 */

std::string
    EthernetInterface::getMACAddress(const std::string& interfaceName) const
{
    std::string activeMACAddr = MacAddressIntf::mACAddress();
    EthernetIntfSocket eifSocket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (eifSocket.sock < 0)
    {
        return activeMACAddr;
    }

    ifreq ifr{0};
    std::strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);
    if (ioctl(eifSocket.sock, SIOCGIFHWADDR, &ifr) != 0)
    {
        log<level::ERR>("ioctl failed for SIOCGIFHWADDR:",
                        entry("ERROR=%s", strerror(errno)));
        elog<InternalFailure>();
    }

    static_assert(sizeof(ifr.ifr_hwaddr.sa_data) >= sizeof(ether_addr));
    std::string_view hwaddr(reinterpret_cast<char*>(ifr.ifr_hwaddr.sa_data),
                            sizeof(ifr.ifr_hwaddr.sa_data));
    return mac_address::toString(stdplus::raw::copyFrom<ether_addr>(hwaddr));
}

std::string EthernetInterface::generateId(const std::string& ipaddress,
                                          uint8_t prefixLength,
                                          const std::string& gateway)
{
    std::stringstream hexId;
    std::string hashString = ipaddress;
    hashString += std::to_string(prefixLength);
    hashString += gateway;

    // Only want 8 hex digits.
    hexId << std::hex << ((std::hash<std::string>{}(hashString)) & 0xFFFFFFFF);
    return hexId.str();
}

std::string EthernetInterface::generateNeighborId(const std::string& iPAddress,
                                                  const std::string& mACAddress)
{
    std::stringstream hexId;
    std::string hashString = iPAddress + mACAddress;

    // Only want 8 hex digits.
    hexId << std::hex << ((std::hash<std::string>{}(hashString)) & 0xFFFFFFFF);
    return hexId.str();
}

void EthernetInterface::deleteObject(const std::string& ipaddress)
{
    auto it = addrs.find(ipaddress);
    if (it == addrs.end())
    {
        log<level::ERR>("DeleteObject:Unable to find the object.");
        return;
    }
    this->addrs.erase(it);
    manager.writeToConfigurationFile();
}

void EthernetInterface::deleteStaticNeighborObject(const std::string& iPAddress)
{
    auto it = staticNeighbors.find(iPAddress);
    if (it == staticNeighbors.end())
    {
        log<level::ERR>(
            "DeleteStaticNeighborObject:Unable to find the object.");
        return;
    }
    staticNeighbors.erase(it);
    manager.writeToConfigurationFile();
}

void EthernetInterface::deleteVLANFromSystem(const std::string& interface)
{
    auto confDir = manager.getConfDir();
    fs::path networkFile = confDir;
    networkFile /= systemd::config::networkFilePrefix + interface +
                   systemd::config::networkFileSuffix;

    fs::path deviceFile = confDir;
    deviceFile /= interface + systemd::config::deviceFileSuffix;

    // delete the vlan network file
    if (fs::is_regular_file(networkFile))
    {
        fs::remove(networkFile);
    }

    // delete the vlan device file
    if (fs::is_regular_file(deviceFile))
    {
        fs::remove(deviceFile);
    }

    // TODO  systemd doesn't delete the virtual network interface
    // even after deleting all the related configuartion.
    // https://github.com/systemd/systemd/issues/6600
    try
    {
        deleteInterface(interface);
    }
    catch (InternalFailure& e)
    {
        commit<InternalFailure>();
    }
}

void EthernetInterface::deleteVLANObject(const std::string& interface)
{
    auto it = vlanInterfaces.find(interface);
    if (it == vlanInterfaces.end())
    {
        log<level::ERR>("DeleteVLANObject:Unable to find the object",
                        entry("INTERFACE=%s", interface.c_str()));
        return;
    }

    deleteVLANFromSystem(interface);
    // delete the interface
    vlanInterfaces.erase(it);

    manager.writeToConfigurationFile();
}

std::string EthernetInterface::generateObjectPath(
    IP::Protocol addressType, const std::string& ipaddress,
    uint8_t prefixLength, const std::string& gateway) const
{
    std::string type = convertForMessage(addressType);
    type = type.substr(type.rfind('.') + 1);
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    std::filesystem::path objectPath;
    objectPath /= objPath;
    objectPath /= type;
    objectPath /= generateId(ipaddress, prefixLength, gateway);
    return objectPath.string();
}

std::string EthernetInterface::generateStaticNeighborObjectPath(
    const std::string& iPAddress, const std::string& mACAddress) const
{
    std::filesystem::path objectPath;
    objectPath /= objPath;
    objectPath /= "static_neighbor";
    objectPath /= generateNeighborId(iPAddress, mACAddress);
    return objectPath.string();
}

bool EthernetInterface::iPv6AcceptRA(bool value)
{
    if (value == EthernetInterfaceIntf::iPv6AcceptRA())
    {
        return value;
    }
    EthernetInterfaceIntf::iPv6AcceptRA(value);
    manager.writeToConfigurationFile();
    return value;
}

EthernetInterface::DHCPConf EthernetInterface::dHCPEnabled(DHCPConf value)
{
    if (value == EthernetInterfaceIntf::dHCPEnabled())
    {
        return value;
    }

    EthernetInterfaceIntf::dHCPEnabled(value);
    manager.writeToConfigurationFile();
    return value;
}

bool EthernetInterface::linkUp() const
{
    EthernetIntfSocket eifSocket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    bool value = EthernetInterfaceIntf::linkUp();

    if (eifSocket.sock < 0)
    {
        return value;
    }

    ifreq ifr{0};
    std::strncpy(ifr.ifr_name, interfaceName().c_str(), IF_NAMESIZE - 1);
    if (ioctl(eifSocket.sock, SIOCGIFFLAGS, &ifr) == 0)
    {
        value = static_cast<bool>(ifr.ifr_flags & IFF_RUNNING);
    }
    else
    {
        log<level::ERR>("ioctl failed for SIOCGIFFLAGS:",
                        entry("ERROR=%s", strerror(errno)));
    }
    return value;
}

bool EthernetInterface::nICEnabled() const
{
    EthernetIntfSocket eifSocket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    bool value = EthernetInterfaceIntf::nICEnabled();

    if (eifSocket.sock < 0)
    {
        return value;
    }

    ifreq ifr{0};
    std::strncpy(ifr.ifr_name, interfaceName().c_str(), IF_NAMESIZE - 1);
    if (ioctl(eifSocket.sock, SIOCGIFFLAGS, &ifr) == 0)
    {
        value = static_cast<bool>(ifr.ifr_flags & IFF_UP);
    }
    else
    {
        log<level::ERR>("ioctl failed for SIOCGIFFLAGS:",
                        entry("ERROR=%s", strerror(errno)));
    }
    return value;
}

bool EthernetInterface::nICEnabled(bool value)
{
    if (value == EthernetInterfaceIntf::nICEnabled())
    {
        return value;
    }

    EthernetIntfSocket eifSocket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (eifSocket.sock < 0)
    {
        return EthernetInterfaceIntf::nICEnabled();
    }

    ifreq ifr{0};
    std::strncpy(ifr.ifr_name, interfaceName().c_str(), IF_NAMESIZE - 1);
    if (ioctl(eifSocket.sock, SIOCGIFFLAGS, &ifr) != 0)
    {
        log<level::ERR>("ioctl failed for SIOCGIFFLAGS:",
                        entry("ERROR=%s", strerror(errno)));
        return EthernetInterfaceIntf::nICEnabled();
    }

    ifr.ifr_flags &= ~IFF_UP;
    ifr.ifr_flags |= value ? IFF_UP : 0;

    if (ioctl(eifSocket.sock, SIOCSIFFLAGS, &ifr) != 0)
    {
        log<level::ERR>("ioctl failed for SIOCSIFFLAGS:",
                        entry("ERROR=%s", strerror(errno)));
        return EthernetInterfaceIntf::nICEnabled();
    }
    EthernetInterfaceIntf::nICEnabled(value);
    writeConfigurationFile();

    return value;
}

ServerList EthernetInterface::nameservers(ServerList /*value*/)
{
    elog<NotAllowed>(NotAllowedArgument::REASON("ReadOnly Property"));
    return EthernetInterfaceIntf::nameservers();
}

ServerList EthernetInterface::staticNameServers(ServerList value)
{
    for (const auto& nameserverip : value)
    {
        if (!isValidIP(AF_INET, nameserverip) &&
            !isValidIP(AF_INET6, nameserverip))
        {
            log<level::ERR>("Not a valid IP address"),
                entry("ADDRESS=%s", nameserverip.c_str());
            elog<InvalidArgument>(
                Argument::ARGUMENT_NAME("StaticNameserver"),
                Argument::ARGUMENT_VALUE(nameserverip.c_str()));
        }
    }
    try
    {
        EthernetInterfaceIntf::staticNameServers(value);
        writeConfigurationFile();
        // resolved reads the DNS server configuration from the
        // network file.
        manager.restartSystemdUnit(networkdService);
    }
    catch (InternalFailure& e)
    {
        log<level::ERR>("Exception processing DNS entries");
    }
    return EthernetInterfaceIntf::staticNameServers();
}

void EthernetInterface::loadNameServers()
{
    EthernetInterfaceIntf::nameservers(getNameServerFromResolvd());
    EthernetInterfaceIntf::staticNameServers(getstaticNameServerFromConf());
}

ServerList EthernetInterface::getstaticNameServerFromConf()
{
    fs::path confPath = manager.getConfDir();

    std::string fileName = systemd::config::networkFilePrefix +
                           interfaceName() + systemd::config::networkFileSuffix;
    confPath /= fileName;
    ServerList servers;
    config::Parser parser(confPath.string());
    auto rc = config::ReturnCode::SUCCESS;

    std::tie(rc, servers) = parser.getValues("Network", "DNS");
    if (rc != config::ReturnCode::SUCCESS)
    {
        log<level::DEBUG>("Unable to get the value for network[DNS]",
                          entry("RC=%d", rc));
    }
    return servers;
}

ServerList EthernetInterface::getNameServerFromResolvd()
{
    ServerList servers;
    std::string OBJ_PATH = RESOLVED_SERVICE_PATH + std::to_string(ifIndex());

    /*
      The DNS property under org.freedesktop.resolve1.Link interface contains
      an array containing all DNS servers currently used by resolved. It
      contains similar information as the DNS server data written to
      /run/systemd/resolve/resolv.conf.

      Each structure in the array consists of a numeric network interface index,
      an address family, and a byte array containing the DNS server address
      (either 4 bytes in length for IPv4 or 16 bytes in lengths for IPv6).
      The array contains DNS servers configured system-wide, including those
      possibly read from a foreign /etc/resolv.conf or the DNS= setting in
      /etc/systemd/resolved.conf, as well as per-interface DNS server
      information either retrieved from systemd-networkd or configured by
      external software via SetLinkDNS().
    */

    using type = std::vector<std::tuple<int32_t, std::vector<uint8_t>>>;
    std::variant<type> name; // Variable to capture the DNS property
    auto method = bus.new_method_call(RESOLVED_SERVICE, OBJ_PATH.c_str(),
                                      PROPERTY_INTERFACE, METHOD_GET);

    method.append(RESOLVED_INTERFACE, "DNS");
    auto reply = bus.call(method);

    try
    {
        reply.read(name);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to get DNS information from Systemd-Resolved");
    }
    auto tupleVector = std::get_if<type>(&name);
    for (auto i = tupleVector->begin(); i != tupleVector->end(); ++i)
    {
        std::vector<uint8_t> ipaddress = std::get<1>(*i);
        std::string address;
        for (auto byte : ipaddress)
        {
            address += std::to_string(byte) + ".";
        }
        address.pop_back();
        servers.push_back(address);
    }
    return servers;
}

void EthernetInterface::loadVLAN(VlanId id)
{
    std::string vlanInterfaceName = interfaceName() + "." + std::to_string(id);
    std::string path = objPath;
    path += "_" + std::to_string(id);

    DHCPConf dhcpEnabled =
        getDHCPValue(manager.getConfDir().string(), vlanInterfaceName);
    auto vlanIntf = std::make_unique<phosphor::network::VlanInterface>(
        bus, path.c_str(), dhcpEnabled, EthernetInterfaceIntf::nICEnabled(), id,
        *this, manager);

    // Fetch the ip address from the system
    // and create the dbus object.
    vlanIntf->createIPAddressObjects();
    vlanIntf->createStaticNeighborObjects();

    this->vlanInterfaces.emplace(std::move(vlanInterfaceName),
                                 std::move(vlanIntf));
}

ObjectPath EthernetInterface::createVLAN(VlanId id)
{
    std::string vlanInterfaceName = interfaceName() + "." + std::to_string(id);
    std::string path = objPath;
    path += "_" + std::to_string(id);

    // Pass the parents nICEnabled property, so that the child
    // VLAN interface can inherit.

    auto vlanIntf = std::make_unique<phosphor::network::VlanInterface>(
        bus, path.c_str(), EthernetInterface::DHCPConf::none,
        EthernetInterfaceIntf::nICEnabled(), id, *this, manager);

    // write the device file for the vlan interface.
    vlanIntf->writeDeviceFile();

    this->vlanInterfaces.emplace(vlanInterfaceName, std::move(vlanIntf));
    // write the new vlan device entry to the configuration(network) file.
    manager.writeToConfigurationFile();

    return path;
}

bool EthernetInterface::getIPv6AcceptRAFromConf()
{
    fs::path confPath = manager.getConfDir();

    std::string fileName = systemd::config::networkFilePrefix +
                           interfaceName() + systemd::config::networkFileSuffix;
    confPath /= fileName;
    config::ValueList values;
    config::Parser parser(confPath.string());
    auto rc = config::ReturnCode::SUCCESS;
    std::tie(rc, values) = parser.getValues("Network", "IPv6AcceptRA");
    if (rc != config::ReturnCode::SUCCESS)
    {
        log<level::DEBUG>("Unable to get the value for Network[IPv6AcceptRA]",
                          entry("rc=%d", rc));
        return false;
    }
    return (values[0] == "true");
}

ServerList EthernetInterface::getNTPServersFromConf()
{
    fs::path confPath = manager.getConfDir();

    std::string fileName = systemd::config::networkFilePrefix +
                           interfaceName() + systemd::config::networkFileSuffix;
    confPath /= fileName;

    ServerList servers;
    config::Parser parser(confPath.string());
    auto rc = config::ReturnCode::SUCCESS;

    std::tie(rc, servers) = parser.getValues("Network", "NTP");
    if (rc != config::ReturnCode::SUCCESS)
    {
        log<level::DEBUG>("Unable to get the value for Network[NTP]",
                          entry("rc=%d", rc));
    }

    return servers;
}

ServerList EthernetInterface::nTPServers(ServerList servers)
{
    auto ntpServers = EthernetInterfaceIntf::nTPServers(servers);

    writeConfigurationFile();
    // timesynchd reads the NTP server configuration from the
    // network file.
    manager.restartSystemdUnit(networkdService);
    return ntpServers;
}
// Need to merge the below function with the code which writes the
// config file during factory reset.
// TODO openbmc/openbmc#1751

void EthernetInterface::writeConfigurationFile()
{
    // write all the static ip address in the systemd-network conf file

    using namespace std::string_literals;
    namespace fs = std::filesystem;

    // if there is vlan interafce then write the configuration file
    // for vlan also.

    for (const auto& intf : vlanInterfaces)
    {
        intf.second->writeConfigurationFile();
    }

    fs::path confPath = manager.getConfDir();

    std::string fileName = systemd::config::networkFilePrefix +
                           interfaceName() + systemd::config::networkFileSuffix;
    confPath /= fileName;
    std::fstream stream;

    stream.open(confPath.c_str(), std::fstream::out);
    if (!stream.is_open())
    {
        log<level::ERR>("Unable to open the file",
                        entry("FILE=%s", confPath.c_str()));
        elog<InternalFailure>();
    }

    // Write the device
    stream << "[Match]\n";
    stream << "Name=" << interfaceName() << "\n";

    auto addrs = getAddresses();

    // Write the link section
    stream << "[Link]\n";
    auto mac = MacAddressIntf::mACAddress();
    if (!mac.empty())
    {
        stream << "MACAddress=" << mac << "\n";
    }

    if (!EthernetInterfaceIntf::nICEnabled())
    {
        stream << "Unmanaged=yes\n";
    }

    // write the network section
    stream << "[Network]\n";
#ifdef LINK_LOCAL_AUTOCONFIGURATION
    stream << "LinkLocalAddressing=yes\n";
#else
    stream << "LinkLocalAddressing=no\n";
#endif
    stream << std::boolalpha
           << "IPv6AcceptRA=" << EthernetInterfaceIntf::iPv6AcceptRA() << "\n";

    // Add the VLAN entry
    for (const auto& intf : vlanInterfaces)
    {
        stream << "VLAN=" << intf.second->EthernetInterface::interfaceName()
               << "\n";
    }
    // Add the NTP server
    for (const auto& ntp : EthernetInterfaceIntf::nTPServers())
    {
        stream << "NTP=" << ntp << "\n";
    }

    // Add the DNS entry
    for (const auto& dns : EthernetInterfaceIntf::staticNameServers())
    {
        stream << "DNS=" << dns << "\n";
    }

    // Add the DHCP entry
    stream << "DHCP="s +
                  mapDHCPToSystemd[EthernetInterfaceIntf::dHCPEnabled()] + "\n";

    // Static IP addresses
    for (const auto& addr : addrs)
    {
        if (originIsManuallyAssigned(addr.second->origin()) &&
            !dhcpIsEnabled(addr.second->type()))
        {
            // Process all static addresses
            std::string address = addr.second->address() + "/" +
                                  std::to_string(addr.second->prefixLength());

            // build the address entries. Do not use [Network] shortcuts to
            // insert address entries.
            stream << "[Address]\n";
            stream << "Address=" << address << "\n";
        }
    }

    if (manager.getSystemConf())
    {
        stream << "[Route]\n";
        const auto& gateway = manager.getSystemConf()->defaultGateway();
        if (!gateway.empty())
        {
            stream << "Gateway=" << gateway << "\n";
        }
        const auto& gateway6 = manager.getSystemConf()->defaultGateway6();
        if (!gateway6.empty())
        {
            stream << "Gateway=" << gateway6 << "\n";
        }
    }

    // Write the neighbor sections
    for (const auto& neighbor : staticNeighbors)
    {
        stream << "[Neighbor]"
               << "\n";
        stream << "Address=" << neighbor.second->iPAddress() << "\n";
        stream << "MACAddress=" << neighbor.second->mACAddress() << "\n";
    }

    // Write the dhcp section irrespective of whether DHCP is enabled or not
    writeDHCPSection(stream);

    stream.close();
}

void EthernetInterface::writeDHCPSection(std::fstream& stream)
{
    using namespace std::string_literals;
    // write the dhcp section
    stream << "[DHCP]\n";

    // Hardcoding the client identifier to mac, to address below issue
    // https://github.com/openbmc/openbmc/issues/1280
    stream << "ClientIdentifier=mac\n";
    if (manager.getDHCPConf())
    {
        auto value = manager.getDHCPConf()->dNSEnabled() ? "true"s : "false"s;
        stream << "UseDNS="s + value + "\n";

        value = manager.getDHCPConf()->nTPEnabled() ? "true"s : "false"s;
        stream << "UseNTP="s + value + "\n";

        value = manager.getDHCPConf()->hostNameEnabled() ? "true"s : "false"s;
        stream << "UseHostname="s + value + "\n";

        value =
            manager.getDHCPConf()->sendHostNameEnabled() ? "true"s : "false"s;
        stream << "SendHostname="s + value + "\n";
    }
}

std::string EthernetInterface::mACAddress(std::string value)
{
    ether_addr newMAC = mac_address::fromString(value);
    if (!mac_address::isUnicast(newMAC))
    {
        log<level::ERR>("MACAddress is not valid.",
                        entry("MAC=%s", value.c_str()));
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("MACAddress"),
                              Argument::ARGUMENT_VALUE(value.c_str()));
    }

    // We don't need to update the system if the address is unchanged
    ether_addr oldMAC = mac_address::fromString(MacAddressIntf::mACAddress());
    if (!stdplus::raw::equal(newMAC, oldMAC))
    {
        // Update everything that depends on the MAC value
        for (const auto& [name, intf] : vlanInterfaces)
        {
            intf->MacAddressIntf::mACAddress(value);
        }
        MacAddressIntf::mACAddress(value);

        auto interface = interfaceName();

#ifdef HAVE_UBOOT_ENV
        auto envVar = interfaceToUbootEthAddr(interface.c_str());
        if (envVar)
        {
            execute("/sbin/fw_setenv", "fw_setenv", envVar->c_str(),
                    value.c_str());
        }
#endif // HAVE_UBOOT_ENV

        // TODO: would remove the call below and
        //      just restart systemd-netwokd
        //      through https://github.com/systemd/systemd/issues/6696
        execute("/sbin/ip", "ip", "link", "set", "dev", interface.c_str(),
                "down");
        manager.writeToConfigurationFile();
    }

    return value;
}

void EthernetInterface::deleteAll()
{
    if (dhcpIsEnabled(IP::Protocol::IPv4, true))
    {
        log<level::INFO>("DHCP enabled on the interface"),
            entry("INTERFACE=%s", interfaceName().c_str());
    }

    // clear all the ip on the interface
    addrs.clear();
    manager.writeToConfigurationFile();
}

} // namespace network
} // namespace phosphor
