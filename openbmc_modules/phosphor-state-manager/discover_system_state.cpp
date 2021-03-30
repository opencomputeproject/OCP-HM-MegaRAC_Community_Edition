#include "config.h"

#include "host_state_manager.hpp"
#include "settings.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp"

#include <getopt.h>
#include <systemd/sd-bus.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>

#include <iostream>
#include <map>
#include <string>

namespace phosphor
{
namespace state
{
namespace manager
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;
using sdbusplus::exception::SdBusError;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";

constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

std::string getService(sdbusplus::bus::bus& bus, std::string path,
                       std::string interface)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));

    std::map<std::string, std::vector<std::string>> mapperResponse;
    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            log<level::ERR>("Error reading mapper response",
                            entry("PATH=%s", path.c_str()),
                            entry("INTERFACE=%s", interface.c_str()));
            throw std::runtime_error("Error reading mapper response");
        }
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in mapper call", entry("ERROR=%s", e.what()),
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
        throw;
    }

    return mapperResponse.begin()->first;
}

std::string getProperty(sdbusplus::bus::bus& bus, std::string path,
                        std::string interface, std::string propertyName)
{
    std::variant<std::string> property;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Get");

    method.append(interface, propertyName);

    try
    {
        auto reply = bus.call(method);
        reply.read(property);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in property Get", entry("ERROR=%s", e.what()),
                        entry("PROPERTY=%s", propertyName.c_str()));
        throw;
    }

    if (std::get<std::string>(property).empty())
    {
        log<level::ERR>("Error reading property response",
                        entry("PROPERTY=%s", propertyName.c_str()));
        throw std::runtime_error("Error reading property response");
    }

    return std::get<std::string>(property);
}

void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& interface, const std::string& property,
                 const std::string& value)
{
    std::variant<std::string> variantValue = value;
    std::string service = getService(bus, path, interface);

    auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                      PROPERTY_INTERFACE, "Set");

    method.append(interface, property, variantValue);
    bus.call_noreply(method);

    return;
}

} // namespace manager
} // namespace state
} // namespace phosphor

int main(int argc, char** argv)
{
    using namespace phosphor::logging;

    std::string hostPath = "/xyz/openbmc_project/state/host0";
    int arg;
    int optIndex = 0;

    static struct option longOpts[] = {{"host", required_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

    while ((arg = getopt_long(argc, argv, "h:", longOpts, &optIndex)) != -1)
    {
        switch (arg)
        {
            case 'h':
                hostPath =
                    std::string("/xyz/openbmc_project/state/host") + optarg;
                break;
            default:
                break;
        }
    }

    auto bus = sdbusplus::bus::new_default();

    using namespace settings;
    Objects settings(bus);

    using namespace phosphor::state::manager;
    namespace server = sdbusplus::xyz::openbmc_project::State::server;

    // This application is only run if chassis power is off

    auto method = bus.new_method_call(
        settings.service(settings.powerRestorePolicy, powerRestoreIntf).c_str(),
        settings.powerRestorePolicy.c_str(), "org.freedesktop.DBus.Properties",
        "Get");
    method.append(powerRestoreIntf, "PowerRestorePolicy");

    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error in PowerRestorePolicy Get",
                        entry("ERROR=%s", e.what()));
        elog<InternalFailure>();
    }

    auto powerPolicy = std::get<std::string>(result);

    log<level::INFO>("Host power is off, checking power policy",
                     entry("POWER_POLICY=%s", powerPolicy.c_str()));

    if (RestorePolicy::Policy::AlwaysOn ==
        RestorePolicy::convertPolicyFromString(powerPolicy))
    {
        log<level::INFO>("power_policy=ALWAYS_POWER_ON, powering host on");
        setProperty(bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                    convertForMessage(server::Host::Transition::On));
    }
    else if (RestorePolicy::Policy::Restore ==
             RestorePolicy::convertPolicyFromString(powerPolicy))
    {
        log<level::INFO>("power_policy=RESTORE, restoring last state");

        // Read last requested state and re-request it to execute it
        auto hostReqState =
            getProperty(bus, hostPath, HOST_BUSNAME, "RequestedHostTransition");
        setProperty(bus, hostPath, HOST_BUSNAME, "RequestedHostTransition",
                    hostReqState);
    }

    return 0;
}
