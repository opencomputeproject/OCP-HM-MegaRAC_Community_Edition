#pragma once

#include "sol/sol_manager.hpp"

#include <systemd/sd-event.h>

#include <boost/asio/io_context.hpp>
#include <chrono>
#include <map>
#include <sdbusplus/asio/connection.hpp>
#include <string>

namespace ipmi
{
namespace rmcpp
{
constexpr uint16_t defaultPort = 623;
} // namespace rmcpp
} // namespace ipmi

namespace eventloop
{
using DbusObjectPath = std::string;
using DbusService = std::string;
using DbusInterface = std::string;
using ObjectTree =
    std::map<DbusObjectPath, std::map<DbusService, std::vector<DbusInterface>>>;
using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, double, std::string>;
// VLANs are a 12-bit value
constexpr uint16_t VLAN_VALUE_MASK = 0x0fff;
constexpr auto MAPPER_BUS_NAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_OBJ = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTF = "xyz.openbmc_project.ObjectMapper";
constexpr auto PATH_ROOT = "/xyz/openbmc_project/network";
constexpr auto INTF_VLAN = "xyz.openbmc_project.Network.VLAN";
constexpr auto INTF_ETHERNET = "xyz.openbmc_project.Network.EthernetInterface";
constexpr auto METHOD_GET = "Get";
constexpr auto PROP_INTF = "org.freedesktop.DBus.Properties";

class EventLoop
{
  public:
    explicit EventLoop(std::shared_ptr<boost::asio::io_context> io) : io(io)
    {
    }
    EventLoop() = delete;
    ~EventLoop() = default;
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    /** @brief Initialise the event loop and add the handler for incoming
     *         IPMI packets.
     *
     *  @return EXIT_SUCCESS on success and EXIT_FAILURE on failure.
     */
    int startEventLoop();

    /** @brief Set up the socket (if systemd has not already) and
     *         make sure that the bus name matches the specified channel
     */
    int setupSocket(std::shared_ptr<sdbusplus::asio::connection>& bus,
                    std::string iface,
                    uint16_t reqPort = ipmi::rmcpp::defaultPort);

  private:
    /** @brief async handler for incoming udp packets */
    void handleRmcpPacket();

    /** @brief register the async handler for incoming udp packets */
    void startRmcpReceive();

    /** @brief get vlanid  */
    int getVLANID(const std::string channel);

    /** @brief boost::asio io context to run with
     */
    std::shared_ptr<boost::asio::io_context> io;

    /** @brief boost::asio udp socket
     */
    std::shared_ptr<boost::asio::ip::udp::socket> udpSocket = nullptr;
};

} // namespace eventloop
