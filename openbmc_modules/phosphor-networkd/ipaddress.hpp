#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>
#include <xyz/openbmc_project/Network/IP/server.hpp>
#include <xyz/openbmc_project/Object/Delete/server.hpp>

namespace phosphor
{
namespace network
{

using IPIfaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Network::server::IP,
    sdbusplus::xyz::openbmc_project::Object::server::Delete>;

using IP = sdbusplus::xyz::openbmc_project::Network::server::IP;

class EthernetInterface;

/** @class IPAddress
 *  @brief OpenBMC IPAddress implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Network.IPProtocol
 *  xyz.openbmc_project.Network.IP Dbus interfaces.
 */
class IPAddress : public IPIfaces
{
  public:
    IPAddress() = delete;
    IPAddress(const IPAddress&) = delete;
    IPAddress& operator=(const IPAddress&) = delete;
    IPAddress(IPAddress&&) = delete;
    IPAddress& operator=(IPAddress&&) = delete;
    virtual ~IPAddress() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] parent - Parent object.
     *  @param[in] type - ipaddress type(v4/v6).
     *  @param[in] ipAddress - ipadress.
     *  @param[in] origin - origin of ipaddress(dhcp/static/SLAAC/LinkLocal).
     *  @param[in] prefixLength - Length of prefix.
     *  @param[in] gateway - gateway address.
     */
    IPAddress(sdbusplus::bus::bus& bus, const char* objPath,
              EthernetInterface& parent, IP::Protocol type,
              const std::string& ipAddress, IP::AddressOrigin origin,
              uint8_t prefixLength, const std::string& gateway);

    std::string address(std::string ipAddress) override;
    uint8_t prefixLength(uint8_t) override;
    std::string gateway(std::string gateway) override;
    IP::Protocol type(IP::Protocol type) override;
    IP::AddressOrigin origin(IP::AddressOrigin origin) override;

    /** @brief Delete this d-bus object.
     */
    void delete_() override;

    using IP::address;
    using IP::gateway;
    using IP::origin;
    using IP::prefixLength;
    using IP::type;

  private:
    /** @brief Parent Object. */
    EthernetInterface& parent;
};

} // namespace network
} // namespace phosphor
