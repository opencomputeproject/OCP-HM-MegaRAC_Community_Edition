#pragma once

#include "snmp_client.hpp"

#include <xyz/openbmc_project/Network/Client/Create/server.hpp>
#include <sdbusplus/bus.hpp>

#include <experimental/filesystem>
#include <string>

namespace phosphor
{
namespace network
{
namespace snmp
{

using ClientList = std::map<Id, std::unique_ptr<Client>>;
namespace fs = std::experimental::filesystem;

namespace details
{

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Network::Client::server::Create>;

} // namespace details

class TestSNMPConfManager;
/** @class Manager
 *  @brief OpenBMC SNMP config  implementation.
 */
class ConfManager : public details::CreateIface
{
  public:
    ConfManager() = delete;
    ConfManager(const ConfManager&) = delete;
    ConfManager& operator=(const ConfManager&) = delete;
    ConfManager(ConfManager&&) = delete;
    ConfManager& operator=(ConfManager&&) = delete;
    virtual ~ConfManager() = default;

    /** @brief Constructor to put object onto bus at a D-Bus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     */
    ConfManager(sdbusplus::bus::bus& bus, const char* objPath);

    /** @brief Function to create snmp manager details D-Bus object.
     *  @param[in] address- IP address/Hostname.
     *  @param[in] port - network port.
     *  @returns D-Bus object path
     */
    std::string client(std::string address, uint16_t port) override;

    /* @brief delete the D-Bus object of the given ID.
     * @param[in] id - client identifier.
     */
    void deleteSNMPClient(Id id);

    /** @brief Construct manager/client D-Bus objects from their persisted
     *         representations.
     */
    void restoreClients();

    /** @brief Check if client is already configured or not.
     *
     *  @param[in] address - SNMP manager address.
     *  @param[in] port -    SNMP manager port.
     *
     *  @return throw exception if client is already configured.
     */
    void checkClientConfigured(const std::string& address, uint16_t port);

    /** @brief location of the persisted D-Bus object.*/
    fs::path dbusPersistentLocation;

  private:
    /** @brief sdbusplus DBus bus object. */
    sdbusplus::bus::bus& bus;

    /** @brief Path of Object. */
    std::string objectPath;

    /** @brief map of SNMP Client dbus objects and their ID */
    ClientList clients;

    /** @brief Id of the last SNMP manager entry */
    Id lastClientId = 0;

    friend class TestSNMPConfManager;
};

} // namespace snmp
} // namespace network
} // namespace phosphor
