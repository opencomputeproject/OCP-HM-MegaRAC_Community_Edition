/* Copyright 2017 - 2019 Intel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include <getopt.h>
#include <linux/ipmi_bmc.h>

#include <CLI/CLI.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

using namespace phosphor::logging;

namespace
{
namespace io_control
{
struct ClearSmsAttention
{
    // Get the name of the IO control command.
    int name() const
    {
        return static_cast<int>(IPMI_BMC_IOCTL_CLEAR_SMS_ATN);
    }

    // Get the address of the command data.
    boost::asio::detail::ioctl_arg_type* data()
    {
        return nullptr;
    }
};

struct SetSmsAttention
{
    // Get the name of the IO control command.
    int name() const
    {
        return static_cast<int>(IPMI_BMC_IOCTL_SET_SMS_ATN);
    }

    // Get the address of the command data.
    boost::asio::detail::ioctl_arg_type* data()
    {
        return nullptr;
    }
};

struct ForceAbort
{
    // Get the name of the IO control command.
    int name() const
    {
        return static_cast<int>(IPMI_BMC_IOCTL_FORCE_ABORT);
    }

    // Get the address of the command data.
    boost::asio::detail::ioctl_arg_type* data()
    {
        return nullptr;
    }
};
} // namespace io_control

class SmsChannel
{
  public:
    static constexpr size_t kcsMessageSize = 4096;
    static constexpr uint8_t netFnShift = 2;
    static constexpr uint8_t lunMask = (1 << netFnShift) - 1;

    SmsChannel(std::shared_ptr<boost::asio::io_context>& io,
               std::shared_ptr<sdbusplus::asio::connection>& bus,
               const std::string& channel, bool verbose) :
        io(io),
        bus(bus), verbose(verbose)
    {
        static constexpr const char devBase[] = "/dev/";
        std::string devName = devBase + channel;
        // open device
        int fd = open(devName.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0)
        {
            log<level::ERR>("Couldn't open SMS channel O_RDWR",
                            entry("FILENAME=%s", devName.c_str()),
                            entry("ERROR=%s", strerror(errno)));
            return;
        }
        else
        {
            dev = std::make_unique<boost::asio::posix::stream_descriptor>(*io,
                                                                          fd);
        }

        async_read();

        // register interfaces...
        server = std::make_shared<sdbusplus::asio::object_server>(bus);

        static constexpr const char pathBase[] =
            "/xyz/openbmc_project/Ipmi/Channel/";
        std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
            server->add_interface(pathBase + channel,
                                  "xyz.openbmc_project.Ipmi.Channel.SMS");
        iface->register_method("setAttention",
                               [this]() { return setAttention(); });
        iface->register_method("clearAttention",
                               [this]() { return clearAttention(); });
        iface->register_method("forceAbort", [this]() { return forceAbort(); });
        iface->initialize();
    }

    bool initOK() const
    {
        return !!dev;
    }

    void channelAbort(const char* msg, const boost::system::error_code& ec)
    {
        log<level::ERR>(msg, entry("ERROR=%s", ec.message().c_str()));
        // bail; maybe a restart from systemd can clear the error
        io->stop();
    }

    void async_read()
    {
        boost::asio::async_read(
            *dev, boost::asio::buffer(xferBuffer, xferBuffer.size()),
            boost::asio::transfer_at_least(2),
            [this](const boost::system::error_code& ec, size_t rlen) {
                processMessage(ec, rlen);
            });
    }

    void processMessage(const boost::system::error_code& ecRd, size_t rlen)
    {
        if (ecRd || rlen < 2)
        {
            channelAbort("Failed to read req msg", ecRd);
            return;
        }

        async_read();

        // trim raw to be only bytes returned from read
        // separate netfn/lun/cmd from payload
        auto rawIter = xferBuffer.cbegin();
        auto rawEnd = rawIter + rlen;
        uint8_t netfn = *rawIter >> netFnShift;
        uint8_t lun = *rawIter++ & lunMask;
        uint8_t cmd = *rawIter++;
        if (verbose)
        {
            log<level::INFO>("Read req msg", entry("NETFN=0x%02x", netfn),
                             entry("LUN=0x%02x", lun),
                             entry("CMD=0x%02x", cmd));
        }
        // copy out payload
        std::vector<uint8_t> data(rawIter, rawEnd);
        // non-session bridges still need to pass an empty options map
        std::map<std::string, std::variant<int>> options;
        // the response is a tuple because dbus can only return a single value
        using IpmiDbusRspType = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,
                                           std::vector<uint8_t>>;
        static constexpr const char ipmiQueueService[] =
            "xyz.openbmc_project.Ipmi.Host";
        static constexpr const char ipmiQueuePath[] =
            "/xyz/openbmc_project/Ipmi";
        static constexpr const char ipmiQueueIntf[] =
            "xyz.openbmc_project.Ipmi.Server";
        static constexpr const char ipmiQueueMethod[] = "execute";
        bus->async_method_call(
            [this, netfnCap{netfn}, lunCap{lun},
             cmdCap{cmd}](const boost::system::error_code& ec,
                          const IpmiDbusRspType& response) {
                std::vector<uint8_t> rsp;
                const auto& [netfn, lun, cmd, cc, payload] = response;
                if (ec)
                {
                    log<level::ERR>(
                        "kcs<->ipmid bus error:", entry("NETFN=0x%02x", netfn),
                        entry("LUN=0x%02x", lun), entry("CMD=0x%02x", cmd),
                        entry("ERROR=%s", ec.message().c_str()));
                    // send unspecified error for a D-Bus error
                    constexpr uint8_t ccResponseNotAvailable = 0xce;
                    rsp.resize(sizeof(netfn) + sizeof(cmd) + sizeof(cc));
                    rsp[0] =
                        ((netfnCap + 1) << netFnShift) | (lunCap & lunMask);
                    rsp[1] = cmdCap;
                    rsp[2] = ccResponseNotAvailable;
                }
                else
                {
                    rsp.resize(sizeof(netfn) + sizeof(cmd) + sizeof(cc) +
                               payload.size());

                    // write the response
                    auto rspIter = rsp.begin();
                    *rspIter++ = (netfn << netFnShift) | (lun & lunMask);
                    *rspIter++ = cmd;
                    *rspIter++ = cc;
                    if (payload.size())
                    {
                        std::copy(payload.cbegin(), payload.cend(), rspIter);
                    }
                }
                if (verbose)
                {
                    log<level::INFO>(
                        "Send rsp msg", entry("NETFN=0x%02x", netfn),
                        entry("LUN=0x%02x", lun), entry("CMD=0x%02x", cmd),
                        entry("CC=0x%02x", cc));
                }
                boost::system::error_code ecWr;
                size_t wlen =
                    boost::asio::write(*dev, boost::asio::buffer(rsp), ecWr);
                if (ecWr || wlen != rsp.size())
                {
                    log<level::ERR>(
                        "Failed to send rsp msg", entry("SIZE=%d", wlen),
                        entry("EXPECT=%d", rsp.size()),
                        entry("ERROR=%s", ecWr.message().c_str()),
                        entry("NETFN=0x%02x", netfn), entry("LUN=0x%02x", lun),
                        entry("CMD=0x%02x", cmd), entry("CC=0x%02x", cc));
                }
            },
            ipmiQueueService, ipmiQueuePath, ipmiQueueIntf, ipmiQueueMethod,
            netfn, lun, cmd, data, options);
    }

    int64_t setAttention()
    {
        if (verbose)
        {
            log<level::INFO>("Sending SET_SMS_ATTENTION");
        }
        io_control::SetSmsAttention command;
        boost::system::error_code ec;
        dev->io_control(command, ec);
        if (ec)
        {
            log<level::ERR>("Couldn't SET_SMS_ATTENTION",
                            entry("ERROR=%s", ec.message().c_str()));
            return ec.value();
        }
        return 0;
    }

    int64_t clearAttention()
    {
        if (verbose)
        {
            log<level::INFO>("Sending CLEAR_SMS_ATTENTION");
        }
        io_control::ClearSmsAttention command;
        boost::system::error_code ec;
        dev->io_control(command, ec);
        if (ec)
        {
            log<level::ERR>("Couldn't CLEAR_SMS_ATTENTION",
                            entry("ERROR=%s", ec.message().c_str()));
            return ec.value();
        }
        return 0;
    }

    int64_t forceAbort()
    {
        if (verbose)
        {
            log<level::INFO>("Sending FORCE_ABORT");
        }
        io_control::ForceAbort command;
        boost::system::error_code ec;
        dev->io_control(command, ec);
        if (ec)
        {
            log<level::ERR>("Couldn't FORCE_ABORT",
                            entry("ERROR=%s", ec.message().c_str()));
            return ec.value();
        }
        return 0;
    }

  protected:
    std::array<uint8_t, kcsMessageSize> xferBuffer;
    std::shared_ptr<boost::asio::io_context> io;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> server;
    std::unique_ptr<boost::asio::posix::stream_descriptor> dev = nullptr;
    bool verbose;
};

} // namespace

int main(int argc, char* argv[])
{
    CLI::App app("KCS IPMI bridge");
    std::string channel;
    app.add_option("-c,--channel", channel, "channel name. e.g., ipmi-kcs3")
        ->required();
    bool verbose = false;
    app.add_option("-v,--verbose", verbose, "print more verbose output");
    CLI11_PARSE(app, argc, argv);

    // Connect to system bus
    auto io = std::make_shared<boost::asio::io_context>();
    sd_bus* dbus;
    sd_bus_default_system(&dbus);
    auto bus = std::make_shared<sdbusplus::asio::connection>(*io, dbus);

    // Create the channel, listening on D-Bus and on the SMS device
    SmsChannel smsChannel(io, bus, channel, verbose);

    if (!smsChannel.initOK())
    {
        return EXIT_FAILURE;
    }

    static constexpr const char busBase[] = "xyz.openbmc_project.Ipmi.Channel.";
    std::string busName(busBase + channel);
    bus->request_name(busName.c_str());

    io->run();

    return 0;
}
