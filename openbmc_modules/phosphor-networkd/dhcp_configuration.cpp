#include "config.h"

#include "dhcp_configuration.hpp"

#include "network_manager.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{
namespace dhcp
{

using namespace phosphor::network;
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
bool Configuration::sendHostNameEnabled(bool value)
{
    if (value == sendHostNameEnabled())
    {
        return value;
    }

    auto name = ConfigIntf::sendHostNameEnabled(value);
    manager.writeToConfigurationFile();

    return name;
}

bool Configuration::hostNameEnabled(bool value)
{
    if (value == hostNameEnabled())
    {
        return value;
    }

    auto name = ConfigIntf::hostNameEnabled(value);
    manager.writeToConfigurationFile();
    manager.restartSystemdUnit(phosphor::network::networkdService);

    return name;
}

bool Configuration::nTPEnabled(bool value)
{
    if (value == nTPEnabled())
    {
        return value;
    }

    auto ntp = ConfigIntf::nTPEnabled(value);
    manager.writeToConfigurationFile();
    manager.restartSystemdUnit(phosphor::network::networkdService);
    manager.restartSystemdUnit(phosphor::network::timeSynchdService);

    return ntp;
}

bool Configuration::dNSEnabled(bool value)
{
    if (value == dNSEnabled())
    {
        return value;
    }

    auto dns = ConfigIntf::dNSEnabled(value);
    manager.writeToConfigurationFile();
    manager.restartSystemdUnit(phosphor::network::networkdService);

    return dns;
}

bool Configuration::getDHCPPropFromConf(const std::string& prop)
{
    fs::path confPath = manager.getConfDir();
    auto interfaceStrList = getInterfaces();
    // get the first interface name, we need it to know config file name.
    auto interface = *interfaceStrList.begin();
    auto fileName = systemd::config::networkFilePrefix + interface +
                    systemd::config::networkFileSuffix;

    confPath /= fileName;
    // systemd default behaviour is all DHCP fields should be enabled by
    // default.
    auto propValue = true;
    config::Parser parser(confPath);

    auto rc = config::ReturnCode::SUCCESS;
    config::ValueList values{};
    std::tie(rc, values) = parser.getValues("DHCP", prop);

    if (rc != config::ReturnCode::SUCCESS)
    {
        log<level::DEBUG>("Unable to get the value from section DHCP",
                          entry("PROP=%s", prop.c_str()), entry("RC=%d", rc));
        return propValue;
    }

    if (values[0] == "false")
    {
        propValue = false;
    }
    return propValue;
}
} // namespace dhcp
} // namespace network
} // namespace phosphor
