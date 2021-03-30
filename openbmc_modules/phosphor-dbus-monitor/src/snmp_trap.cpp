#include "snmp_trap.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <snmp.hpp>
#include <snmp_notification.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>
namespace phosphor
{
namespace dbus
{
namespace monitoring
{
using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Logging::server;
using namespace phosphor::network::snmp;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

static constexpr auto entry = "xyz.openbmc_project.Logging.Entry";

void ErrorTrap::trap(sdbusplus::message::message& msg) const
{
    sdbusplus::message::object_path path;
    msg.read(path);
    PathInterfacesAdded intfs;
    msg.read(intfs);
    auto it = intfs.find(entry);
    if (it == intfs.end())
    {
        return;
    }
    auto& propMap = it->second;
    auto errorID = std::get<uint32_t>(propMap.at("Id"));
    auto timestamp = std::get<uint64_t>(propMap.at("Timestamp"));
    auto sev = std::get<std::string>(propMap.at("Severity"));
    auto isev = static_cast<uint8_t>(Entry::convertLevelFromString(sev));
    auto message = std::get<std::string>(propMap.at("Message"));
    try
    {
        sendTrap<OBMCErrorNotification>(errorID, timestamp, isev, message);
    }
    catch (const InternalFailure& e)
    {
        log<level::INFO>(
            "Failed to send SNMP trap",
            phosphor::logging::entry("ERROR_ID=%d", errorID),
            phosphor::logging::entry("TIMESTAMP=%llu", timestamp),
            phosphor::logging::entry("SEVERITY=%s", sev.c_str()),
            phosphor::logging::entry("MESSAGE=%s", message.c_str()));
    }
}
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
