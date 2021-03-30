#include "main.hpp"

#include "comm_module.hpp"
#include "command/guid.hpp"
#include "command_table.hpp"
#include "message.hpp"
#include "message_handler.hpp"
#include "socket_channel.hpp"
#include "sol_module.hpp"

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <ipmid/api.h>
#include <systemd/sd-daemon.h>
#include <systemd/sd-event.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <tuple>
#include <user_channel/channel_layer.hpp>

using namespace phosphor::logging;

// Tuple of Global Singletons
static auto io = std::make_shared<boost::asio::io_context>();
session::Manager manager;
command::Table table;
eventloop::EventLoop loop(io);
sol::Manager solManager(io);

std::tuple<session::Manager&, command::Table&, eventloop::EventLoop&,
           sol::Manager&>
    singletonPool(manager, table, loop, solManager);

sd_bus* bus = nullptr;

std::shared_ptr<sdbusplus::asio::connection> sdbusp;

/*
 * @brief Required by apphandler IPMI Provider Library
 */
sd_bus* ipmid_get_sd_bus_connection()
{
    return bus;
}

/*
 * @brief mechanism to get at sdbusplus object
 */
std::shared_ptr<sdbusplus::asio::connection> getSdBus()
{
    return sdbusp;
}

static EInterfaceIndex currentInterfaceIndex = interfaceUnknown;
static void setInterfaceIndex(const std::string& channel)
{
    try
    {
        currentInterfaceIndex =
            static_cast<EInterfaceIndex>(ipmi::getChannelByName(channel));
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Requested channel name is not a valid channel name",
                        entry("ERROR=%s", e.what()),
                        entry("CHANNEL=%s", channel.c_str()));
    }
}
EInterfaceIndex getInterfaceIndex(void)
{
    return currentInterfaceIndex;
}

int main(int argc, char* argv[])
{
    CLI::App app("KCS RMCP+ bridge");
    std::string channel;
    app.add_option("-c,--channel", channel, "channel name. e.g., eth0");
    uint16_t port = ipmi::rmcpp::defaultPort;
    app.add_option("-p,--port", port, "port number");
    bool verbose = false;
    app.add_option("-v,--verbose", verbose, "print more verbose output");
    CLI11_PARSE(app, argc, argv);

    // Connect to system bus
    auto rc = sd_bus_default_system(&bus);
    if (rc < 0)
    {
        log<level::ERR>("Failed to connect to system bus",
                        entry("ERROR=%s", strerror(-rc)));
        return rc;
    }
    sdbusp = std::make_shared<sdbusplus::asio::connection>(*io, bus);

    ipmi::ipmiChannelInit();
    if (channel.size())
    {
        setInterfaceIndex(channel);
    }

    std::get<session::Manager&>(singletonPool).managerInit(channel);
    // Register callback to update cache for a GUID change and cache the GUID
    command::registerGUIDChangeCallback();
    cache::guid = command::getSystemGUID();

    // Register the phosphor-net-ipmid session setup commands
    command::sessionSetupCommands();

    // Register the phosphor-net-ipmid SOL commands
    sol::command::registerCommands();

    auto& loop = std::get<eventloop::EventLoop&>(singletonPool);
    if (loop.setupSocket(sdbusp, channel))
    {
        return EXIT_FAILURE;
    }

    // Start Event Loop
    return loop.startEventLoop();
}
