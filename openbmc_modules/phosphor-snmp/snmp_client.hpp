#pragma once
#include <experimental/filesystem>

#include "xyz/openbmc_project/Network/Client/server.hpp"
#include "xyz/openbmc_project/Object/Delete/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>

#include <string>

namespace phosphor
{
namespace network
{
namespace snmp
{

class ConfManager;

using Ifaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Network::server::Client,
    sdbusplus::xyz::openbmc_project::Object::server::Delete>;

using Id = size_t;

/** @class Client
 *  @brief represents the snmp client configuration
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Network.Client Dbus interface.
 */
class Client : public Ifaces
{
  public:
    Client() = delete;
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) = delete;
    Client &operator=(Client &&) = delete;
    virtual ~Client() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] parent - Parent D-bus Object.
     *  @param[in] address - IPaddress/Hostname.
     *  @param[in] port - network port.
     */
    Client(sdbusplus::bus::bus &bus, const char *objPath, ConfManager &parent,
           const std::string &address, uint16_t port);

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] parent - Parent D-bus Object.
     */
    Client(sdbusplus::bus::bus &bus, const char *objPath, ConfManager &parent) :
        Ifaces(bus, objPath, true),
        id(std::stol(std::experimental::filesystem::path(objPath).filename())),
        parent(parent)
    {
    }

    /** @brief Update the address of the object.
     *
     *  @param[in] value - IP address
     *
     *  @return On success the updated IP address
     */
    std::string address(std::string value) override;

    /** @brief Update the port
     *
     *  @param[in] value - port number
     *
     *  @return On success the updated port number
     */
    uint16_t port(uint16_t value) override;

    using sdbusplus::xyz::openbmc_project::Network::server::Client::address;

    using sdbusplus::xyz::openbmc_project::Network::server::Client::port;

    /** @brief Delete this d-bus object.
     */
    void delete_() override;

  private:
    /** Client ID. */
    Id id;
    /** @brief Parent D-Bus Object. */
    ConfManager &parent;
};

} // namespace snmp
} // namespace network
} // namespace phosphor
