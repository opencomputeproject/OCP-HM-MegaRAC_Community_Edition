#include "fru-fault-monitor.hpp"

#include "elog-errors.hpp"
#include "xyz/openbmc_project/Led/Fru/Monitor/error.hpp"
#include "xyz/openbmc_project/Led/Mapper/error.hpp"

#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>

namespace phosphor
{
namespace led
{
namespace fru
{
namespace fault
{
namespace monitor
{

using namespace phosphor::logging;

constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_OBJ_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto OBJMGR_IFACE = "org.freedesktop.DBus.ObjectManager";
constexpr auto LED_GROUPS = "/xyz/openbmc_project/led/groups/";
constexpr auto LOG_PATH = "/xyz/openbmc_project/logging";
constexpr auto LOG_IFACE = "xyz.openbmc_project.Logging.Entry";

using AssociationList =
    std::vector<std::tuple<std::string, std::string, std::string>>;
using Attributes = std::variant<bool, AssociationList>;
using PropertyName = std::string;
using PropertyMap = std::map<PropertyName, Attributes>;
using InterfaceName = std::string;
using InterfaceMap = std::map<InterfaceName, PropertyMap>;

using Service = std::string;
using Path = std::string;
using Interface = std::string;
using Interfaces = std::vector<Interface>;
using MapperResponseType = std::map<Path, std::map<Service, Interfaces>>;

using MethodErr =
    sdbusplus::xyz::openbmc_project::Led::Mapper::Error::MethodError;
using ObjectNotFoundErr =
    sdbusplus::xyz::openbmc_project::Led::Mapper::Error::ObjectNotFoundError;
using InventoryPathErr = sdbusplus::xyz::openbmc_project::Led::Fru::Monitor::
    Error::InventoryPathError;

std::string getService(sdbusplus::bus::bus& bus, const std::string& path)
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                      MAPPER_IFACE, "GetObject");
    mapper.append(path.c_str(), std::vector<std::string>({OBJMGR_IFACE}));
    auto mapperResponseMsg = bus.call(mapper);
    if (mapperResponseMsg.is_method_error())
    {
        using namespace xyz::openbmc_project::Led::Mapper;
        elog<MethodErr>(MethodError::METHOD_NAME("GetObject"),
                        MethodError::PATH(path.c_str()),
                        MethodError::INTERFACE(OBJMGR_IFACE));
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    try
    {
        mapperResponseMsg.read(mapperResponse);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>(
            "Failed to parse getService mapper response",
            entry("ERROR=%s", e.what()),
            entry("REPLY_SIG=%s", mapperResponseMsg.get_signature()));
        using namespace xyz::openbmc_project::Led::Mapper;
        elog<ObjectNotFoundErr>(ObjectNotFoundError::METHOD_NAME("GetObject"),
                                ObjectNotFoundError::PATH(path.c_str()),
                                ObjectNotFoundError::INTERFACE(OBJMGR_IFACE));
    }
    if (mapperResponse.empty())
    {
        using namespace xyz::openbmc_project::Led::Mapper;
        elog<ObjectNotFoundErr>(ObjectNotFoundError::METHOD_NAME("GetObject"),
                                ObjectNotFoundError::PATH(path.c_str()),
                                ObjectNotFoundError::INTERFACE(OBJMGR_IFACE));
    }

    return mapperResponse.cbegin()->first;
}

void action(sdbusplus::bus::bus& bus, const std::string& path, bool assert)
{
    std::string service;
    try
    {
        std::string groups{LED_GROUPS};
        groups.pop_back();
        service = getService(bus, groups);
    }
    catch (MethodErr& e)
    {
        commit<MethodErr>();
        return;
    }
    catch (ObjectNotFoundErr& e)
    {
        commit<ObjectNotFoundErr>();
        return;
    }

    auto pos = path.rfind("/");
    if (pos == std::string::npos)
    {
        using namespace xyz::openbmc_project::Led::Fru::Monitor;
        report<InventoryPathErr>(InventoryPathError::PATH(path.c_str()));
        return;
    }
    auto unit = path.substr(pos + 1);

    std::string ledPath = LED_GROUPS + unit + '_' + LED_FAULT;

    auto method = bus.new_method_call(service.c_str(), ledPath.c_str(),
                                      "org.freedesktop.DBus.Properties", "Set");
    method.append("xyz.openbmc_project.Led.Group");
    method.append("Asserted");

    method.append(std::variant<bool>(assert));

    try
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // Log an info message, system may not have all the LED Groups defined
        log<level::INFO>("Failed to Assert LED Group",
                         entry("ERROR=%s", e.what()));
    }

    return;
}

void Add::created(sdbusplus::message::message& msg)
{
    auto bus = msg.get_bus();

    sdbusplus::message::object_path objectPath;
    InterfaceMap interfaces;
    try
    {
        msg.read(objectPath, interfaces);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to parse created message",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", msg.get_signature()));
        return;
    }

    std::size_t found = objectPath.str.find(ELOG_ENTRY);
    if (found == std::string::npos)
    {
        // Not a new error entry skip
        return;
    }
    auto iter = interfaces.find("xyz.openbmc_project.Association.Definitions");
    if (iter == interfaces.end())
    {
        return;
    }

    // Nothing else shows when a specific error log
    // has been created. Do it here.
    std::string message{objectPath.str + " created"};
    log<level::INFO>(message.c_str());

    auto attr = iter->second.find("Associations");
    if (attr == iter->second.end())
    {
        return;
    }

    auto& assocs = std::get<AssociationList>(attr->second);
    if (assocs.empty())
    {
        // No associations skip
        return;
    }

    for (const auto& item : assocs)
    {
        if (std::get<1>(item).compare(CALLOUT_REV_ASSOCIATION) == 0)
        {
            removeWatches.emplace_back(
                std::make_unique<Remove>(bus, std::get<2>(item)));
            action(bus, std::get<2>(item), true);
        }
    }

    return;
}

void getLoggingSubTree(sdbusplus::bus::bus& bus, MapperResponseType& subtree)
{
    auto depth = 0;
    auto mapperCall = bus.new_method_call(MAPPER_BUSNAME, MAPPER_OBJ_PATH,
                                          MAPPER_IFACE, "GetSubTree");
    mapperCall.append("/");
    mapperCall.append(depth);
    mapperCall.append(std::vector<Interface>({LOG_IFACE}));

    try
    {
        auto mapperResponseMsg = bus.call(mapperCall);
        if (mapperResponseMsg.is_method_error())
        {
            using namespace xyz::openbmc_project::Led::Mapper;
            report<MethodErr>(MethodError::METHOD_NAME("GetSubTree"),
                              MethodError::PATH(MAPPER_OBJ_PATH),
                              MethodError::INTERFACE(OBJMGR_IFACE));
            return;
        }

        try
        {
            mapperResponseMsg.read(subtree);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>(
                "Failed to parse existing callouts subtree message",
                entry("ERROR=%s", e.what()),
                entry("REPLY_SIG=%s", mapperResponseMsg.get_signature()));
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // Just means no log entries at the moment
    }
}

void Add::processExistingCallouts(sdbusplus::bus::bus& bus)
{
    MapperResponseType mapperResponse;

    getLoggingSubTree(bus, mapperResponse);
    if (mapperResponse.empty())
    {
        // No errors to process.
        return;
    }

    for (const auto& elem : mapperResponse)
    {
        auto method = bus.new_method_call(
            elem.second.begin()->first.c_str(), elem.first.c_str(),
            "org.freedesktop.DBus.Properties", "Get");
        method.append("xyz.openbmc_project.Association.Definitions");
        method.append("Associations");
        auto reply = bus.call(method);
        if (reply.is_method_error())
        {
            // do not stop, continue with next elog
            log<level::ERR>("Error in getting associations");
            continue;
        }

        std::variant<AssociationList> assoc;
        try
        {
            reply.read(assoc);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>(
                "Failed to parse existing callouts associations message",
                entry("ERROR=%s", e.what()),
                entry("REPLY_SIG=%s", reply.get_signature()));
            continue;
        }
        auto& assocs = std::get<AssociationList>(assoc);
        if (assocs.empty())
        {
            // no associations, skip
            continue;
        }

        for (const auto& item : assocs)
        {
            if (std::get<1>(item).compare(CALLOUT_REV_ASSOCIATION) == 0)
            {
                removeWatches.emplace_back(
                    std::make_unique<Remove>(bus, std::get<2>(item)));
                action(bus, std::get<2>(item), true);
            }
        }
    }
}

void Remove::removed(sdbusplus::message::message& msg)
{
    auto bus = msg.get_bus();

    action(bus, inventoryPath, false);
    return;
}

} // namespace monitor
} // namespace fault
} // namespace fru
} // namespace led
} // namespace phosphor
