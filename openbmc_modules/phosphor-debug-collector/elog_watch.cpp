#include "config.h"

#include "elog_watch.hpp"

#include "dump_internal.hpp"
#include "dump_serialize.hpp"
#include "errors_map.hpp"
#include "xyz/openbmc_project/Dump/Create/error.hpp"

#include <cereal/cereal.hpp>
#include <fstream>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>

// Register class version with Cereal
CEREAL_CLASS_VERSION(phosphor::dump::elog::Watch, CLASS_VERSION);

namespace phosphor
{
namespace dump
{
namespace elog
{

using namespace phosphor::logging;
constexpr auto LOG_PATH = "/xyz/openbmc_project/logging";
using Message = std::string;
using Attributes = std::variant<Message>;
using AttributeName = std::string;
using AttributeMap = std::map<AttributeName, Attributes>;
using PropertyName = std::string;
using PropertyMap = std::map<PropertyName, AttributeMap>;

Watch::Watch(sdbusplus::bus::bus& bus, IMgr& iMgr) :
    iMgr(iMgr),
    addMatch(bus,
             sdbusplus::bus::match::rules::interfacesAdded() +
                 sdbusplus::bus::match::rules::path_namespace(OBJ_LOGGING),
             std::bind(std::mem_fn(&Watch::addCallback), this,
                       std::placeholders::_1)),
    delMatch(bus,
             sdbusplus::bus::match::rules::interfacesRemoved() +
                 sdbusplus::bus::match::rules::path_namespace(OBJ_LOGGING),
             std::bind(std::mem_fn(&Watch::delCallback), this,
                       std::placeholders::_1))
{

    fs::path file(ELOG_ID_PERSIST_PATH);
    if (fs::exists(file))
    {
        if (!deserialize(ELOG_ID_PERSIST_PATH, elogList))
        {
            log<level::ERR>("Error occurred during error id deserialize");
        }
    }
}

void Watch::addCallback(sdbusplus::message::message& msg)
{
    using QuotaExceeded =
        sdbusplus::xyz::openbmc_project::Dump::Create::Error::QuotaExceeded;

    sdbusplus::message::object_path objectPath;
    PropertyMap propertyMap;
    try
    {
        msg.read(objectPath, propertyMap);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to parse elog add signal",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", msg.get_signature()));
        return;
    }

    std::size_t found = objectPath.str.find("entry");
    if (found == std::string::npos)
    {
        // Not a new error entry skip
        return;
    }

    auto eId = getEid(objectPath);

    auto search = elogList.find(eId);
    if (search != elogList.end())
    {
        // elog exists in the list, Skip the dump
        return;
    }

    auto iter = propertyMap.find("xyz.openbmc_project.Logging.Entry");
    if (iter == propertyMap.end())
    {
        return;
    }

    auto attr = iter->second.find("Message");
    if (attr == iter->second.end())
    {
        return;
    }

    auto& data = std::get<PropertyName>(attr->second);
    if (data.empty())
    {
        // No Message skip
        return;
    }

    EType errorType;
    for (const auto& [type, errorList] : errorMap)
    {
        auto error = std::find(errorList.begin(), errorList.end(), data);
        if (error != errorList.end())
        {
            errorType = type;
            break;
        }
    }

    // error not supported in the configuration
    if (errorType.empty())
    {
        return;
    }

    std::vector<std::string> fullPaths;
    fullPaths.push_back(objectPath);

    try
    {
        // Save the elog information. This is to avoid dump requests
        // in elog restore path.
        elogList.insert(eId);

        phosphor::dump::elog::serialize(elogList);

        auto item = std::find_if(
            TypeMap.begin(), TypeMap.end(),
            [errorType](const auto& err) { return (err.second == errorType); });
        if (item != TypeMap.end())
        {
            iMgr.IMgr::create((*item).first, fullPaths);
        }
    }
    catch (QuotaExceeded& e)
    {
        // No action now
    }
    return;
}

void Watch::delCallback(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path objectPath;
    try
    {
        msg.read(objectPath);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to parse elog del signal",
                        entry("ERROR=%s", e.what()),
                        entry("REPLY_SIG=%s", msg.get_signature()));
        return;
    }

    std::size_t found = objectPath.str.find("entry");
    if (found == std::string::npos)
    {
        // Not a error entry so skip
        return;
    }

    // Get elog id
    auto eId = getEid(objectPath);

    // Delete the elog entry from the list and serialize
    auto search = elogList.find(eId);
    if (search != elogList.end())
    {
        elogList.erase(search);
        phosphor::dump::elog::serialize(elogList);
    }
}

} // namespace elog
} // namespace dump
} // namespace phosphor
