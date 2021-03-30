#pragma once

#include "xyz/openbmc_project/Network/Client/server.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>

namespace phosphor
{
namespace rsyslog_config
{

using namespace phosphor::logging;
using NetworkClient = sdbusplus::xyz::openbmc_project::Network::server::Client;
using Iface = sdbusplus::server::object::object<NetworkClient>;
namespace sdbusRule = sdbusplus::bus::match::rules;

/** @class Server
 *  @brief Configuration for rsyslog server
 *  @details A concrete implementation of the
 *  xyz.openbmc_project.Network.Client API, in order to
 *  provide remote rsyslog server's address and port.
 */
class Server : public Iface
{
  public:
    Server() = delete;
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;
    virtual ~Server() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] filePath - rsyslog remote logging config file
     */
    Server(sdbusplus::bus::bus& bus, const std::string& path,
           const char* filePath) :
        Iface(bus, path.c_str(), true),
        configFilePath(filePath),
        hostnameChange(
            bus,
            sdbusRule::propertiesChanged("/org/freedesktop/hostname1",
                                         "org.freedesktop.hostname1"),
            std::bind(std::mem_fn(&Server::hostnameChanged), this,
                      std::placeholders::_1))
    {
        try
        {
            restore(configFilePath.c_str());
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(e.what());
        }

        emit_object_added();
    }

    using NetworkClient::address;
    using NetworkClient::port;

    /** @brief Override that updates rsyslog config file as well
     *  @param[in] value - remote server address
     *  @returns value of changed address
     */
    virtual std::string address(std::string value) override;

    /** @brief Override that updates rsyslog config file as well
     *  @param[in] value - remote server port
     *  @returns value of changed port
     */
    virtual uint16_t port(uint16_t value) override;

    /** @brief Restart rsyslog's systemd unit
     */
    virtual void restart();

  private:
    /** @brief Update remote server address and port in
     *         rsyslog config file.
     *  @param[in] serverAddress - remote server address
     *  @param[in] serverPort - remote server port
     *  @param[in] filePath - rsyslog config file path
     */
    void writeConfig(const std::string& serverAddress, uint16_t serverPort,
                     const char* filePath);

    /** @brief Checks if input IP address is valid (uses getaddrinfo)
     *  @param[in] address - server address
     *  @returns true if valid, false otherwise
     */
    bool addressValid(const std::string& address);

    /** @brief Populate existing config into D-Bus properties
     *  @param[in] filePath - rsyslog config file path
     */
    void restore(const char* filePath);

    std::string configFilePath{};

    /** @brief React to hostname change
     *  @param[in] msg - sdbusplus message
     */
    void hostnameChanged(sdbusplus::message::message& msg)
    {
        restart();
    }

    sdbusplus::bus::match_t hostnameChange;
};

} // namespace rsyslog_config
} // namespace phosphor
