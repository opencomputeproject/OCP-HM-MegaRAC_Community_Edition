#include "sd_event_loop.hpp"

#include "main.hpp"
#include "message_handler.hpp"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <systemd/sd-daemon.h>

#include <boost/asio/io_context.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <user_channel/channel_layer.hpp>

namespace eventloop
{
using namespace phosphor::logging;

void EventLoop::handleRmcpPacket()
{
    try
    {
        auto channelPtr = std::make_shared<udpsocket::Channel>(udpSocket);

        // Initialize the Message Handler with the socket channel
        auto msgHandler = std::make_shared<message::Handler>(channelPtr, io);

        msgHandler->processIncoming();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Executing the IPMI message failed",
                        entry("EXCEPTION=%s", e.what()));
    }
}

void EventLoop::startRmcpReceive()
{
    udpSocket->async_wait(boost::asio::socket_base::wait_read,
                          [this](const boost::system::error_code& ec) {
                              if (!ec)
                              {
                                  io->post([this]() { startRmcpReceive(); });
                                  handleRmcpPacket();
                              }
                          });
}

int EventLoop::getVLANID(const std::string channel)
{
    int vlanid = 0;
    if (channel.empty())
    {
        return 0;
    }

    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    // Enumerate all VLAN + ETHERNET interfaces
    auto req = bus.new_method_call(MAPPER_BUS_NAME, MAPPER_OBJ, MAPPER_INTF,
                                   "GetSubTree");
    req.append(PATH_ROOT, 0,
               std::vector<std::string>{INTF_VLAN, INTF_ETHERNET});
    ObjectTree objs;
    try
    {
        auto reply = bus.call(req);
        reply.read(objs);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("getVLANID: failed to execute/read GetSubTree");
        return 0;
    }

    std::string ifService, logicalPath;
    for (const auto& [path, impls] : objs)
    {
        if (path.find(channel) == path.npos)
        {
            continue;
        }
        for (const auto& [service, intfs] : impls)
        {
            bool vlan = false;
            bool ethernet = false;
            for (const auto& intf : intfs)
            {
                if (intf == INTF_VLAN)
                {
                    vlan = true;
                }
                else if (intf == INTF_ETHERNET)
                {
                    ethernet = true;
                }
            }
            if (ifService.empty() && (vlan || ethernet))
            {
                ifService = service;
            }
            if (logicalPath.empty() && vlan)
            {
                logicalPath = path;
            }
        }
    }

    // VLAN devices will always have a separate logical object
    if (logicalPath.empty())
    {
        return 0;
    }

    Value value;
    auto method = bus.new_method_call(ifService.c_str(), logicalPath.c_str(),
                                      PROP_INTF, METHOD_GET);
    method.append(INTF_VLAN, "Id");
    try
    {
        auto method_reply = bus.call(method);
        method_reply.read(value);
    }
    catch (std::exception& e)
    {
        log<level::ERR>("getVLANID: failed to execute/read VLAN Id");
        return 0;
    }

    vlanid = std::get<uint32_t>(value);
    if ((vlanid & VLAN_VALUE_MASK) != vlanid)
    {
        log<level::ERR>("networkd returned an invalid vlan",
                        entry("VLAN=%", vlanid));
        return 0;
    }

    return vlanid;
}

int EventLoop::setupSocket(std::shared_ptr<sdbusplus::asio::connection>& bus,
                           std::string channel, uint16_t reqPort)
{
    std::string iface = channel;
    static constexpr const char* unboundIface = "rmcpp";
    if (channel == "")
    {
        iface = channel = unboundIface;
    }
    else
    {
        // If VLANID of this channel is set, bind the socket to this
        // VLAN logic device
        auto vlanid = getVLANID(channel);
        if (vlanid)
        {
            iface = iface + "." + std::to_string(vlanid);
            log<level::DEBUG>("This channel has VLAN id",
                              entry("VLANID=%d", vlanid));
        }
    }
    // Create our own socket if SysD did not supply one.
    int listensFdCount = sd_listen_fds(0);
    if (listensFdCount > 1)
    {
        log<level::ERR>("Too many file descriptors received");
        return EXIT_FAILURE;
    }
    if (listensFdCount == 1)
    {
        int openFd = SD_LISTEN_FDS_START;
        if (!sd_is_socket(openFd, AF_UNSPEC, SOCK_DGRAM, -1))
        {
            log<level::ERR>("Failed to set up systemd-passed socket");
            return EXIT_FAILURE;
        }
        udpSocket = std::make_shared<boost::asio::ip::udp::socket>(
            *io, boost::asio::ip::udp::v6(), openFd);
    }
    else
    {
        // asio does not natively offer a way to bind to an interface
        // so it must be done in steps
        boost::asio::ip::udp::endpoint ep(boost::asio::ip::udp::v6(), reqPort);
        udpSocket = std::make_shared<boost::asio::ip::udp::socket>(*io);
        udpSocket->open(ep.protocol());
        // bind
        udpSocket->set_option(
            boost::asio::ip::udp::socket::reuse_address(true));
        udpSocket->bind(ep);
    }
    // SO_BINDTODEVICE
    char nameout[IFNAMSIZ];
    unsigned int lenout = sizeof(nameout);
    if ((::getsockopt(udpSocket->native_handle(), SOL_SOCKET, SO_BINDTODEVICE,
                      nameout, &lenout) == -1))
    {
        log<level::ERR>("Failed to read bound device",
                        entry("ERROR=%s", strerror(errno)));
    }
    if (iface != nameout && iface != unboundIface)
    {
        // SO_BINDTODEVICE
        if ((::setsockopt(udpSocket->native_handle(), SOL_SOCKET,
                          SO_BINDTODEVICE, iface.c_str(),
                          iface.size() + 1) == -1))
        {
            log<level::ERR>("Failed to bind to requested interface",
                            entry("ERROR=%s", strerror(errno)));
            return EXIT_FAILURE;
        }
        log<level::INFO>("Bind to interfae",
                         entry("INTERFACE=%s", iface.c_str()));
    }
    // cannot be constexpr because it gets passed by address
    const int option_enabled = 1;
    // common socket stuff; set options to get packet info (DST addr)
    ::setsockopt(udpSocket->native_handle(), IPPROTO_IP, IP_PKTINFO,
                 &option_enabled, sizeof(option_enabled));
    ::setsockopt(udpSocket->native_handle(), IPPROTO_IPV6, IPV6_RECVPKTINFO,
                 &option_enabled, sizeof(option_enabled));

    // set the dbus name
    std::string busName = "xyz.openbmc_project.Ipmi.Channel." + channel;
    try
    {
        bus->request_name(busName.c_str());
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to acquire D-Bus name",
                        entry("NAME=%s", busName.c_str()),
                        entry("ERROR=%s", e.what()));
        return EXIT_FAILURE;
    }
    return 0;
}

int EventLoop::startEventLoop()
{
    // set up boost::asio signal handling
    boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
    signals.async_wait(
        [this](const boost::system::error_code& error, int signalNumber) {
            udpSocket->cancel();
            udpSocket->close();
            io->stop();
        });

    startRmcpReceive();

    io->run();

    return EXIT_SUCCESS;
}

} // namespace eventloop
