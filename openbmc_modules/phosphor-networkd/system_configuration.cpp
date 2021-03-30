#include "config.h"

#include "system_configuration.hpp"

#include "network_manager.hpp"
#include "routing_table.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{

// systemd service to kick start a target.
constexpr auto HOSTNAMED_SERVICE = "org.freedesktop.hostname1";
constexpr auto HOSTNAMED_SERVICE_PATH = "/org/freedesktop/hostname1";
constexpr auto HOSTNAMED_INTERFACE = "org.freedesktop.hostname1";
constexpr auto PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto METHOD_GET = "Get";
constexpr auto METHOD_SET = "SetStaticHostname";

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using InvalidArgumentMetadata = xyz::openbmc_project::Common::InvalidArgument;

using SystemConfigIntf =
    sdbusplus::xyz::openbmc_project::Network::server::SystemConfiguration;

SystemConfiguration::SystemConfiguration(sdbusplus::bus::bus& bus,
                                         const std::string& objPath,
                                         Manager& parent) :
    Iface(bus, objPath.c_str(), true),
    bus(bus), manager(parent)
{
    auto name = getHostNameFromSystem();
    route::Table routingTable;

    SystemConfigIntf::hostName(name);
    SystemConfigIntf::defaultGateway(routingTable.getDefaultGateway());
    SystemConfigIntf::defaultGateway6(routingTable.getDefaultGateway6());

    this->emit_object_added();
}

std::string SystemConfiguration::hostName(std::string name)
{
    if (SystemConfigIntf::hostName() == name)
    {
        return name;
    }
    auto method = bus.new_method_call(HOSTNAMED_SERVICE, HOSTNAMED_SERVICE_PATH,
                                      HOSTNAMED_INTERFACE, METHOD_SET);

    method.append(name, true);

    if (!bus.call(method))
    {
        log<level::ERR>("Failed to set the hostname");
        report<InternalFailure>();
        return SystemConfigIntf::hostName();
    }

    return SystemConfigIntf::hostName(name);
}

std::string SystemConfiguration::getHostNameFromSystem() const
{
    try
    {
        std::variant<std::string> name;
        auto method =
            bus.new_method_call(HOSTNAMED_SERVICE, HOSTNAMED_SERVICE_PATH,
                                PROPERTY_INTERFACE, METHOD_GET);

        method.append(HOSTNAMED_INTERFACE, "Hostname");

        auto reply = bus.call(method);

        reply.read(name);
        return std::get<std::string>(name);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>(
            "Failed to get the hostname from systemd-hostnamed service",
            entry("ERR=%s", ex.what()));
    }
    return "";
}

std::string SystemConfiguration::defaultGateway(std::string gateway)
{
    auto gw = SystemConfigIntf::defaultGateway();
    if (gw == gateway)
    {
        return gw;
    }

    if (!isValidIP(AF_INET, gateway))
    {
        log<level::ERR>("Not a valid v4 Gateway",
                        entry("GATEWAY=%s", gateway.c_str()));
        elog<InvalidArgument>(
            InvalidArgumentMetadata::ARGUMENT_NAME("GATEWAY"),
            InvalidArgumentMetadata::ARGUMENT_VALUE(gateway.c_str()));
    }
    gw = SystemConfigIntf::defaultGateway(gateway);
    manager.writeToConfigurationFile();
    return gw;
}

std::string SystemConfiguration::defaultGateway6(std::string gateway)
{
    auto gw = SystemConfigIntf::defaultGateway6();
    if (gw == gateway)
    {
        return gw;
    }

    if (!isValidIP(AF_INET6, gateway))
    {
        log<level::ERR>("Not a valid v6 Gateway",
                        entry("GATEWAY=%s", gateway.c_str()));
        elog<InvalidArgument>(
            InvalidArgumentMetadata::ARGUMENT_NAME("GATEWAY"),
            InvalidArgumentMetadata::ARGUMENT_VALUE(gateway.c_str()));
    }
    gw = SystemConfigIntf::defaultGateway6(gateway);
    manager.writeToConfigurationFile();
    return gw;
}

} // namespace network
} // namespace phosphor
