#include "config.h"

#include "selutility.hpp"

#include <chrono>
#include <filesystem>
#include <ipmid/api.hpp>
#include <ipmid/types.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <vector>
#include <xyz/openbmc_project/Common/error.hpp>

extern const ipmi::sensor::InvObjectIDMap invSensors;
using namespace phosphor::logging;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

namespace ipmi
{

namespace sel
{

namespace internal
{

GetSELEntryResponse
    prepareSELEntry(const std::string& objPath,
                    ipmi::sensor::InvObjectIDMap::const_iterator iter)
{
    GetSELEntryResponse record{};

    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    auto service = ipmi::getService(bus, logEntryIntf, objPath);

    // Read all the log entry properties.
    auto methodCall = bus.new_method_call(service.c_str(), objPath.c_str(),
                                          propIntf, "GetAll");
    methodCall.append(logEntryIntf);

    auto reply = bus.call(methodCall);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in reading logging property entries");
        elog<InternalFailure>();
    }

    std::map<PropertyName, PropertyType> entryData;
    reply.read(entryData);

    // Read Id from the log entry.
    static constexpr auto propId = "Id";
    auto iterId = entryData.find(propId);
    if (iterId == entryData.end())
    {
        log<level::ERR>("Error in reading Id of logging entry");
        elog<InternalFailure>();
    }

    record.recordID = static_cast<uint16_t>(std::get<uint32_t>(iterId->second));

    // Read Timestamp from the log entry.
    static constexpr auto propTimeStamp = "Timestamp";
    auto iterTimeStamp = entryData.find(propTimeStamp);
    if (iterTimeStamp == entryData.end())
    {
        log<level::ERR>("Error in reading Timestamp of logging entry");
        elog<InternalFailure>();
    }

    std::chrono::milliseconds chronoTimeStamp(
        std::get<uint64_t>(iterTimeStamp->second));
    record.timeStamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(chronoTimeStamp)
            .count());

    static constexpr auto systemEventRecord = 0x02;
    static constexpr auto generatorID = 0x2000;
    static constexpr auto eventMsgRevision = 0x04;

    record.recordType = systemEventRecord;
    record.generatorID = generatorID;
    record.eventMsgRevision = eventMsgRevision;

    record.sensorType = iter->second.sensorType;
    record.sensorNum = iter->second.sensorID;
    record.eventData1 = iter->second.eventOffset;

    // Read Resolved from the log entry.
    static constexpr auto propResolved = "Resolved";
    auto iterResolved = entryData.find(propResolved);
    if (iterResolved == entryData.end())
    {
        log<level::ERR>("Error in reading Resolved field of logging entry");
        elog<InternalFailure>();
    }

    static constexpr auto deassertEvent = 0x80;

    // Evaluate if the event is assertion or deassertion event
    if (std::get<bool>(iterResolved->second))
    {
        record.eventType = deassertEvent | iter->second.eventReadingType;
    }
    else
    {
        record.eventType = iter->second.eventReadingType;
    }

    return record;
}

} // namespace internal

GetSELEntryResponse convertLogEntrytoSEL(const std::string& objPath)
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    static constexpr auto assocIntf =
        "xyz.openbmc_project.Association.Definitions";
    static constexpr auto assocProp = "Associations";

    auto service = ipmi::getService(bus, assocIntf, objPath);

    // Read the Associations interface.
    auto methodCall =
        bus.new_method_call(service.c_str(), objPath.c_str(), propIntf, "Get");
    methodCall.append(assocIntf);
    methodCall.append(assocProp);

    auto reply = bus.call(methodCall);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in reading Associations interface");
        elog<InternalFailure>();
    }

    using AssociationList =
        std::vector<std::tuple<std::string, std::string, std::string>>;

    std::variant<AssociationList> list;
    reply.read(list);

    auto& assocs = std::get<AssociationList>(list);

    /*
     * Check if the log entry has any callout associations, if there is a
     * callout association try to match the inventory path to the corresponding
     * IPMI sensor.
     */
    for (const auto& item : assocs)
    {
        if (std::get<0>(item).compare(CALLOUT_FWD_ASSOCIATION) == 0)
        {
            auto iter = invSensors.find(std::get<2>(item));
            if (iter == invSensors.end())
            {
                iter = invSensors.find(BOARD_SENSOR);
                if (iter == invSensors.end())
                {
                    log<level::ERR>("Motherboard sensor not found");
                    elog<InternalFailure>();
                }
            }

            return internal::prepareSELEntry(objPath, iter);
        }
    }

    // If there are no callout associations link the log entry to system event
    // sensor
    auto iter = invSensors.find(SYSTEM_SENSOR);
    if (iter == invSensors.end())
    {
        log<level::ERR>("System event sensor not found");
        elog<InternalFailure>();
    }

    return internal::prepareSELEntry(objPath, iter);
}

std::chrono::seconds getEntryTimeStamp(const std::string& objPath)
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    auto service = ipmi::getService(bus, logEntryIntf, objPath);

    using namespace std::string_literals;
    static const auto propTimeStamp = "Timestamp"s;

    auto methodCall =
        bus.new_method_call(service.c_str(), objPath.c_str(), propIntf, "Get");
    methodCall.append(logEntryIntf);
    methodCall.append(propTimeStamp);

    auto reply = bus.call(methodCall);
    if (reply.is_method_error())
    {
        log<level::ERR>("Error in reading Timestamp from Entry interface");
        elog<InternalFailure>();
    }

    std::variant<uint64_t> timeStamp;
    reply.read(timeStamp);

    std::chrono::milliseconds chronoTimeStamp(std::get<uint64_t>(timeStamp));

    return std::chrono::duration_cast<std::chrono::seconds>(chronoTimeStamp);
}

void readLoggingObjectPaths(ObjectPaths& paths)
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    auto depth = 0;
    paths.clear();

    auto mapperCall = bus.new_method_call(mapperBusName, mapperObjPath,
                                          mapperIntf, "GetSubTreePaths");
    mapperCall.append(logBasePath);
    mapperCall.append(depth);
    mapperCall.append(ObjectPaths({logEntryIntf}));

    auto reply = bus.call(mapperCall);
    if (reply.is_method_error())
    {
        log<level::INFO>("Error in reading logging entry object paths");
    }
    else
    {
        reply.read(paths);

        std::sort(paths.begin(), paths.end(),
                  [](const std::string& a, const std::string& b) {
                      namespace fs = std::filesystem;
                      fs::path pathA(a);
                      fs::path pathB(b);
                      auto idA = std::stoul(pathA.filename().string());
                      auto idB = std::stoul(pathB.filename().string());

                      return idA < idB;
                  });
    }
}

} // namespace sel

} // namespace ipmi
