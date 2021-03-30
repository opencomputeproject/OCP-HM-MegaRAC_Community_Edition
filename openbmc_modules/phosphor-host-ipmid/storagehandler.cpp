#include "storagehandler.hpp"

#include "fruread.hpp"
#include "read_fru_data.hpp"
#include "selutility.hpp"
#include "sensorhandler.hpp"
#include "storageaddsel.hpp"

#include <arpa/inet.h>
#include <mapper.h>
#include <systemd/sd-bus.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <variant>
#include <xyz/openbmc_project/Common/error.hpp>

void register_netfn_storage_functions() __attribute__((constructor));

unsigned int g_sel_time = 0xFFFFFFFF;
namespace ipmi
{
namespace sensor
{
extern const IdInfoMap sensors;
} // namespace sensor
} // namespace ipmi

extern const FruMap frus;
constexpr uint8_t eventDataSize = 3;
namespace
{
constexpr auto TIME_INTERFACE = "xyz.openbmc_project.Time.EpochTime";
constexpr auto BMC_TIME_PATH = "/xyz/openbmc_project/time/bmc";
constexpr auto DBUS_PROPERTIES = "org.freedesktop.DBus.Properties";
constexpr auto PROPERTY_ELAPSED = "Elapsed";

} // namespace

namespace cache
{
/*
 * This cache contains the object paths of the logging entries sorted in the
 * order of the filename(numeric order). The cache is initialized by
 * invoking readLoggingObjectPaths with the cache as the parameter. The
 * cache is invoked in the execution of the Get SEL info and Delete SEL
 * entry command. The Get SEL Info command is typically invoked before the
 * Get SEL entry command, so the cache is utilized for responding to Get SEL
 * entry command. The cache is invalidated by clearing after Delete SEL
 * entry and Clear SEL command.
 */
ipmi::sel::ObjectPaths paths;

} // namespace cache

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using namespace phosphor::logging;
using namespace ipmi::fru;

/**
 * @enum Device access mode
 */
enum class AccessMode
{
    bytes, ///< Device is accessed by bytes
    words  ///< Device is accessed by words
};

/** @brief implements the get SEL Info command
 *  @returns IPMI completion code plus response data
 *   - selVersion - SEL revision
 *   - entries    - Number of log entries in SEL.
 *   - freeSpace  - Free Space in bytes.
 *   - addTimeStamp - Most recent addition timestamp
 *   - eraseTimeStamp - Most recent erase timestamp
 *   - operationSupport - Reserve & Delete SEL operations supported
 */

ipmi::RspType<uint8_t,  // SEL revision.
              uint16_t, // number of log entries in SEL.
              uint16_t, // free Space in bytes.
              uint32_t, // most recent addition timestamp
              uint32_t, // most recent erase timestamp.

              bool,    // SEL allocation info supported
              bool,    // reserve SEL supported
              bool,    // partial Add SEL Entry supported
              bool,    // delete SEL supported
              uint3_t, // reserved
              bool     // overflow flag
              >
    ipmiStorageGetSelInfo()
{
    uint16_t entries = 0;
    // Most recent addition timestamp.
    uint32_t addTimeStamp = ipmi::sel::invalidTimeStamp;

    try
    {
        ipmi::sel::readLoggingObjectPaths(cache::paths);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // No action if reading log objects have failed for this command.
        // readLoggingObjectPaths will throw exception if there are no log
        // entries. The command will be responded with number of SEL entries
        // as 0.
    }

    if (!cache::paths.empty())
    {
        entries = static_cast<uint16_t>(cache::paths.size());

        try
        {
            addTimeStamp = static_cast<uint32_t>(
                (ipmi::sel::getEntryTimeStamp(cache::paths.back()).count()));
        }
        catch (InternalFailure& e)
        {
        }
        catch (const std::runtime_error& e)
        {
            log<level::ERR>(e.what());
        }
    }

    constexpr uint8_t selVersion = ipmi::sel::selVersion;
    constexpr uint16_t freeSpace = 0xFFFF;
    constexpr uint32_t eraseTimeStamp = ipmi::sel::invalidTimeStamp;
    constexpr uint3_t reserved{0};

    return ipmi::responseSuccess(
        selVersion, entries, freeSpace, addTimeStamp, eraseTimeStamp,
        ipmi::sel::operationSupport::getSelAllocationInfo,
        ipmi::sel::operationSupport::reserveSel,
        ipmi::sel::operationSupport::partialAddSelEntry,
        ipmi::sel::operationSupport::deleteSel, reserved,
        ipmi::sel::operationSupport::overflow);
}

ipmi_ret_t getSELEntry(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                       ipmi_request_t request, ipmi_response_t response,
                       ipmi_data_len_t data_len, ipmi_context_t context)
{
    if (*data_len != sizeof(ipmi::sel::GetSELEntryRequest))
    {
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto requestData =
        reinterpret_cast<const ipmi::sel::GetSELEntryRequest*>(request);

    if (requestData->reservationID != 0)
    {
        if (!checkSELReservation(requestData->reservationID))
        {
            *data_len = 0;
            return IPMI_CC_INVALID_RESERVATION_ID;
        }
    }

    if (cache::paths.empty())
    {
        *data_len = 0;
        return IPMI_CC_SENSOR_INVALID;
    }

    ipmi::sel::ObjectPaths::const_iterator iter;

    // Check for the requested SEL Entry.
    if (requestData->selRecordID == ipmi::sel::firstEntry)
    {
        iter = cache::paths.begin();
    }
    else if (requestData->selRecordID == ipmi::sel::lastEntry)
    {
        iter = cache::paths.end();
    }
    else
    {
        std::string objPath = std::string(ipmi::sel::logBasePath) + "/" +
                              std::to_string(requestData->selRecordID);

        iter = std::find(cache::paths.begin(), cache::paths.end(), objPath);
        if (iter == cache::paths.end())
        {
            *data_len = 0;
            return IPMI_CC_SENSOR_INVALID;
        }
    }

    ipmi::sel::GetSELEntryResponse record{};

    // Convert the log entry into SEL record.
    try
    {
        record = ipmi::sel::convertLogEntrytoSEL(*iter);
    }
    catch (InternalFailure& e)
    {
        *data_len = 0;
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    catch (const std::runtime_error& e)
    {
        log<level::ERR>(e.what());
        *data_len = 0;
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    // Identify the next SEL record ID
    if (iter != cache::paths.end())
    {
        ++iter;
        if (iter == cache::paths.end())
        {
            record.nextRecordID = ipmi::sel::lastEntry;
        }
        else
        {
            namespace fs = std::filesystem;
            fs::path path(*iter);
            record.nextRecordID = static_cast<uint16_t>(
                std::stoul(std::string(path.filename().c_str())));
        }
    }
    else
    {
        record.nextRecordID = ipmi::sel::lastEntry;
    }

    if (requestData->readLength == ipmi::sel::entireRecord)
    {
        std::memcpy(response, &record, sizeof(record));
        *data_len = sizeof(record);
    }
    else
    {
        if (requestData->offset >= ipmi::sel::selRecordSize ||
            requestData->readLength > ipmi::sel::selRecordSize)
        {
            *data_len = 0;
            return IPMI_CC_INVALID_FIELD_REQUEST;
        }

        auto diff = ipmi::sel::selRecordSize - requestData->offset;
        auto readLength =
            std::min(diff, static_cast<int>(requestData->readLength));

        std::memcpy(response, &record.nextRecordID,
                    sizeof(record.nextRecordID));
        std::memcpy(static_cast<uint8_t*>(response) +
                        sizeof(record.nextRecordID),
                    &record.recordID + requestData->offset, readLength);
        *data_len = sizeof(record.nextRecordID) + readLength;
    }

    return IPMI_CC_OK;
}

/** @brief implements the delete SEL entry command
 * @request
 *   - reservationID; // reservation ID.
 *   - selRecordID;   // SEL record ID.
 *
 *  @returns ipmi completion code plus response data
 *   - Record ID of the deleted record
 */
ipmi::RspType<uint16_t // deleted record ID
              >
    deleteSELEntry(uint16_t reservationID, uint16_t selRecordID)
{

    namespace fs = std::filesystem;

    if (!checkSELReservation(reservationID))
    {
        return ipmi::responseInvalidReservationId();
    }

    // Per the IPMI spec, need to cancel the reservation when a SEL entry is
    // deleted
    cancelSELReservation();

    try
    {
        ipmi::sel::readLoggingObjectPaths(cache::paths);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // readLoggingObjectPaths will throw exception if there are no error
        // log entries.
        return ipmi::responseSensorInvalid();
    }

    if (cache::paths.empty())
    {
        return ipmi::responseSensorInvalid();
    }

    ipmi::sel::ObjectPaths::const_iterator iter;
    uint16_t delRecordID = 0;

    if (selRecordID == ipmi::sel::firstEntry)
    {
        iter = cache::paths.begin();
        fs::path path(*iter);
        delRecordID = static_cast<uint16_t>(
            std::stoul(std::string(path.filename().c_str())));
    }
    else if (selRecordID == ipmi::sel::lastEntry)
    {
        iter = cache::paths.end();
        fs::path path(*iter);
        delRecordID = static_cast<uint16_t>(
            std::stoul(std::string(path.filename().c_str())));
    }
    else
    {
        std::string objPath = std::string(ipmi::sel::logBasePath) + "/" +
                              std::to_string(selRecordID);

        iter = std::find(cache::paths.begin(), cache::paths.end(), objPath);
        if (iter == cache::paths.end())
        {
            return ipmi::responseSensorInvalid();
        }
        delRecordID = selRecordID;
    }

    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    std::string service;

    try
    {
        service = ipmi::getService(bus, ipmi::sel::logDeleteIntf, *iter);
    }
    catch (const std::runtime_error& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }

    auto methodCall = bus.new_method_call(service.c_str(), (*iter).c_str(),
                                          ipmi::sel::logDeleteIntf, "Delete");
    auto reply = bus.call(methodCall);
    if (reply.is_method_error())
    {
        return ipmi::responseUnspecifiedError();
    }

    // Invalidate the cache of dbus entry objects.
    cache::paths.clear();

    return ipmi::responseSuccess(delRecordID);
}

/** @brief implements the Clear SEL command
 * @request
 *   - reservationID   // Reservation ID.
 *   - clr             // char array { 'C'(0x43h), 'L'(0x4Ch), 'R'(0x52h) }
 *   - eraseOperation; // requested operation.
 *
 *  @returns ipmi completion code plus response data
 *   - erase status
 */

ipmi::RspType<uint8_t // erase status
              >
    clearSEL(uint16_t reservationID, const std::array<char, 3>& clr,
             uint8_t eraseOperation)
{
    static constexpr std::array<char, 3> clrOk = {'C', 'L', 'R'};
    if (clr != clrOk)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    if (!checkSELReservation(reservationID))
    {
        return ipmi::responseInvalidReservationId();
    }

    /*
     * Erasure status cannot be fetched from DBUS, so always return erasure
     * status as `erase completed`.
     */
    if (eraseOperation == ipmi::sel::getEraseStatus)
    {
        return ipmi::responseSuccess(
            static_cast<uint8_t>(ipmi::sel::eraseComplete));
    }

    // Per the IPMI spec, need to cancel any reservation when the SEL is cleared
    cancelSELReservation();

    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
    ipmi::sel::ObjectPaths objectPaths;
    auto depth = 0;

    auto mapperCall =
        bus.new_method_call(ipmi::sel::mapperBusName, ipmi::sel::mapperObjPath,
                            ipmi::sel::mapperIntf, "GetSubTreePaths");
    mapperCall.append(ipmi::sel::logBasePath);
    mapperCall.append(depth);
    mapperCall.append(ipmi::sel::ObjectPaths({ipmi::sel::logEntryIntf}));

    try
    {
        auto reply = bus.call(mapperCall);
        if (reply.is_method_error())
        {
            return ipmi::responseSuccess(
                static_cast<uint8_t>(ipmi::sel::eraseComplete));
        }

        reply.read(objectPaths);
        if (objectPaths.empty())
        {
            return ipmi::responseSuccess(
                static_cast<uint8_t>(ipmi::sel::eraseComplete));
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return ipmi::responseSuccess(
            static_cast<uint8_t>(ipmi::sel::eraseComplete));
    }

    std::string service;

    try
    {
        service = ipmi::getService(bus, ipmi::sel::logDeleteIntf,
                                   objectPaths.front());
    }
    catch (const std::runtime_error& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }

    for (const auto& iter : objectPaths)
    {
        auto methodCall = bus.new_method_call(
            service.c_str(), iter.c_str(), ipmi::sel::logDeleteIntf, "Delete");

        auto reply = bus.call(methodCall);
        if (reply.is_method_error())
        {
            return ipmi::responseUnspecifiedError();
        }
    }

    // Invalidate the cache of dbus entry objects.
    cache::paths.clear();
    return ipmi::responseSuccess(
        static_cast<uint8_t>(ipmi::sel::eraseComplete));
}

/** @brief implements the get SEL time command
 *  @returns IPMI completion code plus response data
 *   -current time
 */
ipmi::RspType<uint32_t> // current time
    ipmiStorageGetSelTime()
{
    using namespace std::chrono;
    uint64_t bmc_time_usec = 0;
    std::stringstream bmcTime;

    try
    {
        sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
        auto service = ipmi::getService(bus, TIME_INTERFACE, BMC_TIME_PATH);
        std::variant<uint64_t> value;

        // Get bmc time
        auto method = bus.new_method_call(service.c_str(), BMC_TIME_PATH,
                                          DBUS_PROPERTIES, "Get");

        method.append(TIME_INTERFACE, PROPERTY_ELAPSED);
        auto reply = bus.call(method);
        if (reply.is_method_error())
        {
            log<level::ERR>("Error getting time",
                            entry("SERVICE=%s", service.c_str()),
                            entry("PATH=%s", BMC_TIME_PATH));
            return ipmi::responseUnspecifiedError();
        }
        reply.read(value);
        bmc_time_usec = std::get<uint64_t>(value);
    }
    catch (InternalFailure& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }

    bmcTime << "BMC time:"
            << duration_cast<seconds>(microseconds(bmc_time_usec)).count();
    log<level::DEBUG>(bmcTime.str().c_str());

    // Time is really long int but IPMI wants just uint32. This works okay until
    // the number of seconds since 1970 overflows uint32 size.. Still a whole
    // lot of time here to even think about that.
    return ipmi::responseSuccess(
        duration_cast<seconds>(microseconds(bmc_time_usec)).count());
}

/** @brief implements the set SEL time command
 *  @param selDeviceTime - epoch time
 *        -local time as the number of seconds from 00:00:00, January 1, 1970
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiStorageSetSelTime(uint32_t selDeviceTime)
{
    using namespace std::chrono;
    microseconds usec{seconds(selDeviceTime)};

    try
    {
        sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};
        auto service = ipmi::getService(bus, TIME_INTERFACE, BMC_TIME_PATH);
        std::variant<uint64_t> value{(uint64_t)usec.count()};

        // Set bmc time
        auto method = bus.new_method_call(service.c_str(), BMC_TIME_PATH,
                                          DBUS_PROPERTIES, "Set");

        method.append(TIME_INTERFACE, PROPERTY_ELAPSED, value);
        auto reply = bus.call(method);
        if (reply.is_method_error())
        {
            log<level::ERR>("Error setting time",
                            entry("SERVICE=%s", service.c_str()),
                            entry("PATH=%s", BMC_TIME_PATH));
            return ipmi::responseUnspecifiedError();
        }
    }
    catch (InternalFailure& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

/** @brief implements the reserve SEL command
 *  @returns IPMI completion code plus response data
 *   - SEL reservation ID.
 */
ipmi::RspType<uint16_t> ipmiStorageReserveSel()
{
    return ipmi::responseSuccess(reserveSel());
}

/** @brief implements the Add SEL entry command
 * @request
 *
 *   - recordID      ID used for SEL Record access
 *   - recordType    Record Type
 *   - timeStamp     Time when event was logged. LS byte first
 *   - generatorID   software ID if event was generated from
 *                   system software
 *   - evmRev        event message format version
 *   - sensorType    sensor type code for service that generated
 *                   the event
 *   - sensorNumber  number of sensors that generated the event
 *   - eventDir     event dir
 *   - eventData    event data field contents
 *
 *  @returns ipmi completion code plus response data
 *   - RecordID of the Added SEL entry
 */
ipmi::RspType<uint16_t // recordID of the Added SEL entry
              >
    ipmiStorageAddSEL(uint16_t recordID, uint8_t recordType, uint32_t timeStamp,
                      uint16_t generatorID, uint8_t evmRev, uint8_t sensorType,
                      uint8_t sensorNumber, uint8_t eventDir,
                      std::array<uint8_t, eventDataSize> eventData)
{
    // Per the IPMI spec, need to cancel the reservation when a SEL entry is
    // added
    cancelSELReservation();
    // Hostboot sends SEL with OEM record type 0xDE to indicate that there is
    // a maintenance procedure associated with eSEL record.
    static constexpr auto procedureType = 0xDE;
    if (recordType == procedureType)
    {
        // In the OEM record type 0xDE, byte 11 in the SEL record indicate the
        // procedure number.
        createProcedureLogEntry(sensorType);
    }

    return ipmi::responseSuccess(recordID);
}

bool isFruPresent(const std::string& fruPath)
{
    using namespace ipmi::fru;

    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    auto propValue =
        ipmi::getDbusProperty(bus, invMgrInterface, invObjPath + fruPath,
                              invItemInterface, itemPresentProp);

    return std::get<bool>(propValue);
}

/** @brief implements the get FRU Inventory Area Info command
 *
 *  @returns IPMI completion code plus response data
 *   - FRU Inventory area size in bytes,
 *   - access bit
 **/
ipmi::RspType<uint16_t, // FRU Inventory area size in bytes,
              uint8_t   // access size (bytes / words)
              >
    ipmiStorageGetFruInvAreaInfo(uint8_t fruID)
{

    auto iter = frus.find(fruID);
    if (iter == frus.end())
    {
        return ipmi::responseSensorInvalid();
    }

    auto path = iter->second[0].path;
    if (!isFruPresent(path))
    {
        return ipmi::responseSensorInvalid();
    }

    try
    {
        return ipmi::responseSuccess(
            static_cast<uint16_t>(getFruAreaData(fruID).size()),
            static_cast<uint8_t>(AccessMode::bytes));
    }
    catch (const InternalFailure& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
}

/**@brief implements the Read FRU Data command
 * @param fruDeviceId - FRU device ID. FFh = reserved
 * @param offset      - FRU inventory offset to read
 * @param readCount   - count to read
 *
 * @return IPMI completion code plus response data
 * - returnCount - response data count.
 * - data        -  response data
 */
ipmi::RspType<uint8_t,              // count returned
              std::vector<uint8_t>> // FRU data
    ipmiStorageReadFruData(uint8_t fruDeviceId, uint16_t offset,
                           uint8_t readCount)
{
    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    auto iter = frus.find(fruDeviceId);
    if (iter == frus.end())
    {
        return ipmi::responseSensorInvalid();
    }

    try
    {
        const auto& fruArea = getFruAreaData(fruDeviceId);
        auto size = fruArea.size();

        if (offset >= size)
        {
            return ipmi::responseParmOutOfRange();
        }

        // Write the count of response data.
        uint8_t returnCount;
        if ((offset + readCount) <= size)
        {
            returnCount = readCount;
        }
        else
        {
            returnCount = size - offset;
        }

        std::vector<uint8_t> fruData((fruArea.begin() + offset),
                                     (fruArea.begin() + offset + returnCount));

        return ipmi::responseSuccess(returnCount, fruData);
    }
    catch (const InternalFailure& e)
    {
        log<level::ERR>(e.what());
        return ipmi::responseUnspecifiedError();
    }
}

ipmi::RspType<uint8_t,  // SDR version
              uint16_t, // record count LS first
              uint16_t, // free space in bytes, LS first
              uint32_t, // addition timestamp LS first
              uint32_t, // deletion timestamp LS first
              uint8_t>  // operation Support
    ipmiGetRepositoryInfo()
{

    constexpr uint8_t sdrVersion = 0x51;
    constexpr uint16_t freeSpace = 0xFFFF;
    constexpr uint32_t additionTimestamp = 0x0;
    constexpr uint32_t deletionTimestamp = 0x0;
    constexpr uint8_t operationSupport = 0;

    uint16_t records = frus.size() + ipmi::sensor::sensors.size();

    return ipmi::responseSuccess(sdrVersion, records, freeSpace,
                                 additionTimestamp, deletionTimestamp,
                                 operationSupport);
}

void register_netfn_storage_functions()
{
    // <Get SEL Info>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelInfo, ipmi::Privilege::User,
                          ipmiStorageGetSelInfo);

    // <Get SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelTime, ipmi::Privilege::User,
                          ipmiStorageGetSelTime);

    // <Set SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdSetSelTime,
                          ipmi::Privilege::Operator, ipmiStorageSetSelTime);

    // <Reserve SEL>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdReserveSel, ipmi::Privilege::User,
                          ipmiStorageReserveSel);
    // <Get SEL Entry>
    ipmi_register_callback(NETFUN_STORAGE, IPMI_CMD_GET_SEL_ENTRY, NULL,
                           getSELEntry, PRIVILEGE_USER);

    // <Delete SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdDeleteSelEntry,
                          ipmi::Privilege::Operator, deleteSELEntry);

    // <Add SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdAddSelEntry,
                          ipmi::Privilege::Operator, ipmiStorageAddSEL);

    // <Clear SEL>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdClearSel, ipmi::Privilege::Operator,
                          clearSEL);

    // <Get FRU Inventory Area Info>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetFruInventoryAreaInfo,
                          ipmi::Privilege::User, ipmiStorageGetFruInvAreaInfo);

    // <READ FRU Data>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdReadFruData,
                          ipmi::Privilege::Operator, ipmiStorageReadFruData);

    // <Get Repository Info>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSdrRepositoryInfo,
                          ipmi::Privilege::User, ipmiGetRepositoryInfo);

    // <Reserve SDR Repository>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdReserveSdrRepository,
                          ipmi::Privilege::User, ipmiSensorReserveSdr);

    // <Get SDR>
    ipmi_register_callback(NETFUN_STORAGE, IPMI_CMD_GET_SDR, nullptr,
                           ipmi_sen_get_sdr, PRIVILEGE_USER);

    ipmi::fru::registerCallbackHandler();
    return;
}
