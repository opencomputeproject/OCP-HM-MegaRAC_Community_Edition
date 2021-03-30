/*
// Copyright (c) 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "storagecommands.hpp"

#include "commandutils.hpp"
#include "ipmi_to_redfish_hooks.hpp"
#include "sdrutils.hpp"
#include "types.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process.hpp>
#include <ipmid/api.hpp>
#include <ipmid/message.hpp>
#include <phosphor-ipmi-host/selutility.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/timer.hpp>

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string_view>

static constexpr bool DEBUG = false;

namespace intel_oem::ipmi::sel
{
static const std::filesystem::path selLogDir = "/var/log";
static const std::string selLogFilename = "ipmi_sel";

static int getFileTimestamp(const std::filesystem::path& file)
{
    struct stat st;

    if (stat(file.c_str(), &st) >= 0)
    {
        return st.st_mtime;
    }
    return ::ipmi::sel::invalidTimeStamp;
}

namespace erase_time
{
static constexpr const char* selEraseTimestamp = "/var/lib/ipmi/sel_erase_time";

void save()
{
    // open the file, creating it if necessary
    int fd = open(selEraseTimestamp, O_WRONLY | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0)
    {
        std::cerr << "Failed to open file\n";
        return;
    }

    // update the file timestamp to the current time
    if (futimens(fd, NULL) < 0)
    {
        std::cerr << "Failed to update timestamp: "
                  << std::string(strerror(errno));
    }
    close(fd);
}

int get()
{
    return getFileTimestamp(selEraseTimestamp);
}
} // namespace erase_time
} // namespace intel_oem::ipmi::sel

namespace ipmi
{

namespace storage
{

constexpr static const size_t maxMessageSize = 64;
constexpr static const size_t maxFruSdrNameSize = 16;
using ObjectType = boost::container::flat_map<
    std::string, boost::container::flat_map<std::string, DbusVariant>>;
using ManagedObjectType =
    boost::container::flat_map<sdbusplus::message::object_path, ObjectType>;
using ManagedEntry = std::pair<sdbusplus::message::object_path, ObjectType>;

constexpr static const char* fruDeviceServiceName =
    "xyz.openbmc_project.FruDevice";
constexpr static const char* entityManagerServiceName =
    "xyz.openbmc_project.EntityManager";
constexpr static const size_t writeTimeoutSeconds = 10;
constexpr static const char* chassisTypeRackMount = "23";

// event direction is bit[7] of eventType where 1b = Deassertion event
constexpr static const uint8_t deassertionEvent = 0x80;

static std::vector<uint8_t> fruCache;
static uint8_t cacheBus = 0xFF;
static uint8_t cacheAddr = 0XFF;
static uint8_t lastDevId = 0xFF;

static uint8_t writeBus = 0xFF;
static uint8_t writeAddr = 0XFF;

std::unique_ptr<phosphor::Timer> writeTimer = nullptr;
static std::vector<sdbusplus::bus::match::match> fruMatches;

ManagedObjectType frus;

// we unfortunately have to build a map of hashes in case there is a
// collision to verify our dev-id
boost::container::flat_map<uint8_t, std::pair<uint8_t, uint8_t>> deviceHashes;

void registerStorageFunctions() __attribute__((constructor));

bool writeFru()
{
    if (writeBus == 0xFF && writeAddr == 0xFF)
    {
        return true;
    }
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    sdbusplus::message::message writeFru = dbus->new_method_call(
        fruDeviceServiceName, "/xyz/openbmc_project/FruDevice",
        "xyz.openbmc_project.FruDeviceManager", "WriteFru");
    writeFru.append(writeBus, writeAddr, fruCache);
    try
    {
        sdbusplus::message::message writeFruResp = dbus->call(writeFru);
    }
    catch (sdbusplus::exception_t&)
    {
        // todo: log sel?
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "error writing fru");
        return false;
    }
    writeBus = 0xFF;
    writeAddr = 0xFF;
    return true;
}

void createTimers()
{
    writeTimer = std::make_unique<phosphor::Timer>(writeFru);
}

void recalculateHashes()
{

    deviceHashes.clear();
    // hash the object paths to create unique device id's. increment on
    // collision
    std::hash<std::string> hasher;
    for (const auto& fru : frus)
    {
        auto fruIface = fru.second.find("xyz.openbmc_project.FruDevice");
        if (fruIface == fru.second.end())
        {
            continue;
        }

        auto busFind = fruIface->second.find("BUS");
        auto addrFind = fruIface->second.find("ADDRESS");
        if (busFind == fruIface->second.end() ||
            addrFind == fruIface->second.end())
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "fru device missing Bus or Address",
                phosphor::logging::entry("FRU=%s", fru.first.str.c_str()));
            continue;
        }

        uint8_t fruBus = std::get<uint32_t>(busFind->second);
        uint8_t fruAddr = std::get<uint32_t>(addrFind->second);
        auto chassisFind = fruIface->second.find("CHASSIS_TYPE");
        std::string chassisType;
        if (chassisFind != fruIface->second.end())
        {
            chassisType = std::get<std::string>(chassisFind->second);
        }

        uint8_t fruHash = 0;
        if (chassisType.compare(chassisTypeRackMount) != 0)
        {
            fruHash = hasher(fru.first.str);
            // can't be 0xFF based on spec, and 0 is reserved for baseboard
            if (fruHash == 0 || fruHash == 0xFF)
            {
                fruHash = 1;
            }
        }
        std::pair<uint8_t, uint8_t> newDev(fruBus, fruAddr);

        bool emplacePassed = false;
        while (!emplacePassed)
        {
            auto resp = deviceHashes.emplace(fruHash, newDev);
            emplacePassed = resp.second;
            if (!emplacePassed)
            {
                fruHash++;
                // can't be 0xFF based on spec, and 0 is reserved for
                // baseboard
                if (fruHash == 0XFF)
                {
                    fruHash = 0x1;
                }
            }
        }
    }
}

void replaceCacheFru(const std::shared_ptr<sdbusplus::asio::connection>& bus,
                     boost::asio::yield_context& yield,
                     const std::optional<std::string>& path = std::nullopt)
{
    boost::system::error_code ec;

    frus = bus->yield_method_call<ManagedObjectType>(
        yield, ec, fruDeviceServiceName, "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "GetMangagedObjects for getSensorMap failed",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));

        return;
    }
    recalculateHashes();
}

ipmi::Cc getFru(ipmi::Context::ptr ctx, uint8_t devId)
{
    if (lastDevId == devId && devId != 0xFF)
    {
        return ipmi::ccSuccess;
    }

    auto deviceFind = deviceHashes.find(devId);
    if (deviceFind == deviceHashes.end())
    {
        return IPMI_CC_SENSOR_INVALID;
    }

    fruCache.clear();

    cacheBus = deviceFind->second.first;
    cacheAddr = deviceFind->second.second;

    boost::system::error_code ec;

    fruCache = ctx->bus->yield_method_call<std::vector<uint8_t>>(
        ctx->yield, ec, fruDeviceServiceName, "/xyz/openbmc_project/FruDevice",
        "xyz.openbmc_project.FruDeviceManager", "GetRawFru", cacheBus,
        cacheAddr);
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Couldn't get raw fru",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));

        cacheBus = 0xFF;
        cacheAddr = 0xFF;
        return ipmi::ccResponseError;
    }

    lastDevId = devId;
    return ipmi::ccSuccess;
}

void writeFruIfRunning()
{
    if (!writeTimer->isRunning())
    {
        return;
    }
    writeTimer->stop();
    writeFru();
}

void startMatch(void)
{
    if (fruMatches.size())
    {
        return;
    }

    fruMatches.reserve(2);

    auto bus = getSdBus();
    fruMatches.emplace_back(*bus,
                            "type='signal',arg0path='/xyz/openbmc_project/"
                            "FruDevice/',member='InterfacesAdded'",
                            [](sdbusplus::message::message& message) {
                                sdbusplus::message::object_path path;
                                ObjectType object;
                                try
                                {
                                    message.read(path, object);
                                }
                                catch (sdbusplus::exception_t&)
                                {
                                    return;
                                }
                                auto findType = object.find(
                                    "xyz.openbmc_project.FruDevice");
                                if (findType == object.end())
                                {
                                    return;
                                }
                                writeFruIfRunning();
                                frus[path] = object;
                                recalculateHashes();
                                lastDevId = 0xFF;
                            });

    fruMatches.emplace_back(*bus,
                            "type='signal',arg0path='/xyz/openbmc_project/"
                            "FruDevice/',member='InterfacesRemoved'",
                            [](sdbusplus::message::message& message) {
                                sdbusplus::message::object_path path;
                                std::set<std::string> interfaces;
                                try
                                {
                                    message.read(path, interfaces);
                                }
                                catch (sdbusplus::exception_t&)
                                {
                                    return;
                                }
                                auto findType = interfaces.find(
                                    "xyz.openbmc_project.FruDevice");
                                if (findType == interfaces.end())
                                {
                                    return;
                                }
                                writeFruIfRunning();
                                frus.erase(path);
                                recalculateHashes();
                                lastDevId = 0xFF;
                            });

    // call once to populate
    boost::asio::spawn(*getIoContext(), [](boost::asio::yield_context yield) {
        replaceCacheFru(getSdBus(), yield);
    });
}

/** @brief implements the read FRU data command
 *  @param fruDeviceId        - FRU Device ID
 *  @param fruInventoryOffset - FRU Inventory Offset to write
 *  @param countToRead        - Count to read
 *
 *  @returns ipmi completion code plus response data
 *   - countWritten  - Count written
 */
ipmi::RspType<uint8_t,             // Count
              std::vector<uint8_t> // Requested data
              >
    ipmiStorageReadFruData(ipmi::Context::ptr ctx, uint8_t fruDeviceId,
                           uint16_t fruInventoryOffset, uint8_t countToRead)
{
    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    ipmi::Cc status = getFru(ctx, fruDeviceId);

    if (status != ipmi::ccSuccess)
    {
        return ipmi::response(status);
    }

    size_t fromFruByteLen = 0;
    if (countToRead + fruInventoryOffset < fruCache.size())
    {
        fromFruByteLen = countToRead;
    }
    else if (fruCache.size() > fruInventoryOffset)
    {
        fromFruByteLen = fruCache.size() - fruInventoryOffset;
    }
    else
    {
        return ipmi::responseReqDataLenExceeded();
    }

    std::vector<uint8_t> requestedData;

    requestedData.insert(
        requestedData.begin(), fruCache.begin() + fruInventoryOffset,
        fruCache.begin() + fruInventoryOffset + fromFruByteLen);

    return ipmi::responseSuccess(static_cast<uint8_t>(requestedData.size()),
                                 requestedData);
}

/** @brief implements the write FRU data command
 *  @param fruDeviceId        - FRU Device ID
 *  @param fruInventoryOffset - FRU Inventory Offset to write
 *  @param dataToWrite        - Data to write
 *
 *  @returns ipmi completion code plus response data
 *   - countWritten  - Count written
 */
ipmi::RspType<uint8_t>
    ipmiStorageWriteFruData(ipmi::Context::ptr ctx, uint8_t fruDeviceId,
                            uint16_t fruInventoryOffset,
                            std::vector<uint8_t>& dataToWrite)
{
    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    size_t writeLen = dataToWrite.size();

    ipmi::Cc status = getFru(ctx, fruDeviceId);
    if (status != ipmi::ccSuccess)
    {
        return ipmi::response(status);
    }
    int lastWriteAddr = fruInventoryOffset + writeLen;
    if (fruCache.size() < lastWriteAddr)
    {
        fruCache.resize(fruInventoryOffset + writeLen);
    }

    std::copy(dataToWrite.begin(), dataToWrite.begin() + writeLen,
              fruCache.begin() + fruInventoryOffset);

    bool atEnd = false;

    if (fruCache.size() >= sizeof(FRUHeader))
    {
        FRUHeader* header = reinterpret_cast<FRUHeader*>(fruCache.data());

        int areaLength = 0;
        int lastRecordStart = std::max(
            {header->internalOffset, header->chassisOffset, header->boardOffset,
             header->productOffset, header->multiRecordOffset});
        lastRecordStart *= 8; // header starts in are multiples of 8 bytes

        if (header->multiRecordOffset)
        {
            // This FRU has a MultiRecord Area
            uint8_t endOfList = 0;
            // Walk the MultiRecord headers until the last record
            while (!endOfList)
            {
                // The MSB in the second byte of the MultiRecord header signals
                // "End of list"
                endOfList = fruCache[lastRecordStart + 1] & 0x80;
                // Third byte in the MultiRecord header is the length
                areaLength = fruCache[lastRecordStart + 2];
                // This length is in bytes (not 8 bytes like other headers)
                areaLength += 5; // The length omits the 5 byte header
                if (!endOfList)
                {
                    // Next MultiRecord header
                    lastRecordStart += areaLength;
                }
            }
        }
        else
        {
            // This FRU does not have a MultiRecord Area
            // Get the length of the area in multiples of 8 bytes
            if (lastWriteAddr > (lastRecordStart + 1))
            {
                // second byte in record area is the length
                areaLength = fruCache[lastRecordStart + 1];
                areaLength *= 8; // it is in multiples of 8 bytes
            }
        }
        if (lastWriteAddr >= (areaLength + lastRecordStart))
        {
            atEnd = true;
        }
    }
    uint8_t countWritten = 0;

    writeBus = cacheBus;
    writeAddr = cacheAddr;
    if (atEnd)
    {
        // cancel timer, we're at the end so might as well send it
        writeTimer->stop();
        if (!writeFru())
        {
            return ipmi::responseInvalidFieldRequest();
        }
        countWritten = std::min(fruCache.size(), static_cast<size_t>(0xFF));
    }
    else
    {
        // start a timer, if no further data is sent  to check to see if it is
        // valid
        writeTimer->start(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::seconds(writeTimeoutSeconds)));
        countWritten = 0;
    }

    return ipmi::responseSuccess(countWritten);
}

/** @brief implements the get FRU inventory area info command
 *  @param fruDeviceId  - FRU Device ID
 *
 *  @returns IPMI completion code plus response data
 *   - inventorySize - Number of possible allocation units
 *   - accessType    - Allocation unit size in bytes.
 */
ipmi::RspType<uint16_t, // inventorySize
              uint8_t>  // accessType
    ipmiStorageGetFruInvAreaInfo(ipmi::Context::ptr ctx, uint8_t fruDeviceId)
{
    if (fruDeviceId == 0xFF)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    getFru(ctx, fruDeviceId);

    constexpr uint8_t accessType =
        static_cast<uint8_t>(GetFRUAreaAccessType::byte);

    return ipmi::responseSuccess(fruCache.size(), accessType);
}

ipmi_ret_t getFruSdrCount(ipmi::Context::ptr ctx, size_t& count)
{
    count = deviceHashes.size();
    return IPMI_CC_OK;
}

ipmi_ret_t getFruSdrs(ipmi::Context::ptr ctx, size_t index,
                      get_sdr::SensorDataFruRecord& resp)
{
    if (deviceHashes.size() < index)
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }
    auto device = deviceHashes.begin() + index;
    uint8_t& bus = device->second.first;
    uint8_t& address = device->second.second;

    boost::container::flat_map<std::string, DbusVariant>* fruData = nullptr;
    auto fru =
        std::find_if(frus.begin(), frus.end(),
                     [bus, address, &fruData](ManagedEntry& entry) {
                         auto findFruDevice =
                             entry.second.find("xyz.openbmc_project.FruDevice");
                         if (findFruDevice == entry.second.end())
                         {
                             return false;
                         }
                         fruData = &(findFruDevice->second);
                         auto findBus = findFruDevice->second.find("BUS");
                         auto findAddress =
                             findFruDevice->second.find("ADDRESS");
                         if (findBus == findFruDevice->second.end() ||
                             findAddress == findFruDevice->second.end())
                         {
                             return false;
                         }
                         if (std::get<uint32_t>(findBus->second) != bus)
                         {
                             return false;
                         }
                         if (std::get<uint32_t>(findAddress->second) != address)
                         {
                             return false;
                         }
                         return true;
                     });
    if (fru == frus.end())
    {
        return IPMI_CC_RESPONSE_ERROR;
    }

#ifdef USING_ENTITY_MANAGER_DECORATORS

    boost::container::flat_map<std::string, DbusVariant>* entityData = nullptr;

    // todo: this should really use caching, this is a very inefficient lookup
    boost::system::error_code ec;
    ManagedObjectType entities = ctx->bus->yield_method_call<ManagedObjectType>(
        ctx->yield, ec, entityManagerServiceName, "/",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");

    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "GetMangagedObjects for getSensorMap failed",
            phosphor::logging::entry("ERROR=%s", ec.message().c_str()));

        return ipmi::ccResponseError;
    }

    auto entity = std::find_if(
        entities.begin(), entities.end(),
        [bus, address, &entityData](ManagedEntry& entry) {
            auto findFruDevice = entry.second.find(
                "xyz.openbmc_project.Inventory.Decorator.FruDevice");
            if (findFruDevice == entry.second.end())
            {
                return false;
            }

            // Integer fields added via Entity-Manager json are uint64_ts by
            // default.
            auto findBus = findFruDevice->second.find("Bus");
            auto findAddress = findFruDevice->second.find("Address");

            if (findBus == findFruDevice->second.end() ||
                findAddress == findFruDevice->second.end())
            {
                return false;
            }
            if ((std::get<uint64_t>(findBus->second) != bus) ||
                (std::get<uint64_t>(findAddress->second) != address))
            {
                return false;
            }

            // At this point we found the device entry and should return
            // true.
            auto findIpmiDevice = entry.second.find(
                "xyz.openbmc_project.Inventory.Decorator.Ipmi");
            if (findIpmiDevice != entry.second.end())
            {
                entityData = &(findIpmiDevice->second);
            }

            return true;
        });

    if (entity == entities.end())
    {
        if constexpr (DEBUG)
        {
            std::fprintf(stderr, "Ipmi or FruDevice Decorator interface "
                                 "not found for Fru\n");
        }
    }

#endif

    std::string name;
    auto findProductName = fruData->find("BOARD_PRODUCT_NAME");
    auto findBoardName = fruData->find("PRODUCT_PRODUCT_NAME");
    if (findProductName != fruData->end())
    {
        name = std::get<std::string>(findProductName->second);
    }
    else if (findBoardName != fruData->end())
    {
        name = std::get<std::string>(findBoardName->second);
    }
    else
    {
        name = "UNKNOWN";
    }
    if (name.size() > maxFruSdrNameSize)
    {
        name = name.substr(0, maxFruSdrNameSize);
    }
    size_t sizeDiff = maxFruSdrNameSize - name.size();

    resp.header.record_id_lsb = 0x0; // calling code is to implement these
    resp.header.record_id_msb = 0x0;
    resp.header.sdr_version = ipmiSdrVersion;
    resp.header.record_type = get_sdr::SENSOR_DATA_FRU_RECORD;
    resp.header.record_length = sizeof(resp.body) + sizeof(resp.key) - sizeDiff;
    resp.key.deviceAddress = 0x20;
    resp.key.fruID = device->first;
    resp.key.accessLun = 0x80; // logical / physical fru device
    resp.key.channelNumber = 0x0;
    resp.body.reserved = 0x0;
    resp.body.deviceType = 0x10;
    resp.body.deviceTypeModifier = 0x0;

    uint8_t entityID = 0;
    uint8_t entityInstance = 0x1;

#ifdef USING_ENTITY_MANAGER_DECORATORS
    if (entityData)
    {
        auto entityIdProperty = entityData->find("EntityId");
        auto entityInstanceProperty = entityData->find("EntityInstance");

        if (entityIdProperty != entityData->end())
        {
            entityID = static_cast<uint8_t>(
                std::get<uint64_t>(entityIdProperty->second));
        }
        if (entityInstanceProperty != entityData->end())
        {
            entityInstance = static_cast<uint8_t>(
                std::get<uint64_t>(entityInstanceProperty->second));
        }
    }
#endif

    resp.body.entityID = entityID;
    resp.body.entityInstance = entityInstance;

    resp.body.oem = 0x0;
    resp.body.deviceIDLen = name.size();
    name.copy(resp.body.deviceID, name.size());

    return IPMI_CC_OK;
}

static bool getSELLogFiles(std::vector<std::filesystem::path>& selLogFiles)
{
    // Loop through the directory looking for ipmi_sel log files
    for (const std::filesystem::directory_entry& dirEnt :
         std::filesystem::directory_iterator(intel_oem::ipmi::sel::selLogDir))
    {
        std::string filename = dirEnt.path().filename();
        if (boost::starts_with(filename, intel_oem::ipmi::sel::selLogFilename))
        {
            // If we find an ipmi_sel log file, save the path
            selLogFiles.emplace_back(intel_oem::ipmi::sel::selLogDir /
                                     filename);
        }
    }
    // As the log files rotate, they are appended with a ".#" that is higher for
    // the older logs. Since we don't expect more than 10 log files, we
    // can just sort the list to get them in order from newest to oldest
    std::sort(selLogFiles.begin(), selLogFiles.end());

    return !selLogFiles.empty();
}

static int countSELEntries()
{
    // Get the list of ipmi_sel log files
    std::vector<std::filesystem::path> selLogFiles;
    if (!getSELLogFiles(selLogFiles))
    {
        return 0;
    }
    int numSELEntries = 0;
    // Loop through each log file and count the number of logs
    for (const std::filesystem::path& file : selLogFiles)
    {
        std::ifstream logStream(file);
        if (!logStream.is_open())
        {
            continue;
        }

        std::string line;
        while (std::getline(logStream, line))
        {
            numSELEntries++;
        }
    }
    return numSELEntries;
}

static bool findSELEntry(const int recordID,
                         const std::vector<std::filesystem::path>& selLogFiles,
                         std::string& entry)
{
    // Record ID is the first entry field following the timestamp. It is
    // preceded by a space and followed by a comma
    std::string search = " " + std::to_string(recordID) + ",";

    // Loop through the ipmi_sel log entries
    for (const std::filesystem::path& file : selLogFiles)
    {
        std::ifstream logStream(file);
        if (!logStream.is_open())
        {
            continue;
        }

        while (std::getline(logStream, entry))
        {
            // Check if the record ID matches
            if (entry.find(search) != std::string::npos)
            {
                return true;
            }
        }
    }
    return false;
}

static uint16_t
    getNextRecordID(const uint16_t recordID,
                    const std::vector<std::filesystem::path>& selLogFiles)
{
    uint16_t nextRecordID = recordID + 1;
    std::string entry;
    if (findSELEntry(nextRecordID, selLogFiles, entry))
    {
        return nextRecordID;
    }
    else
    {
        return ipmi::sel::lastEntry;
    }
}

static int fromHexStr(const std::string& hexStr, std::vector<uint8_t>& data)
{
    for (unsigned int i = 0; i < hexStr.size(); i += 2)
    {
        try
        {
            data.push_back(static_cast<uint8_t>(
                std::stoul(hexStr.substr(i, 2), nullptr, 16)));
        }
        catch (std::invalid_argument& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
            return -1;
        }
        catch (std::out_of_range& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
            return -1;
        }
    }
    return 0;
}

ipmi::RspType<uint8_t,  // SEL version
              uint16_t, // SEL entry count
              uint16_t, // free space
              uint32_t, // last add timestamp
              uint32_t, // last erase timestamp
              uint8_t>  // operation support
    ipmiStorageGetSELInfo()
{
    constexpr uint8_t selVersion = ipmi::sel::selVersion;
    uint16_t entries = countSELEntries();
    uint32_t addTimeStamp = intel_oem::ipmi::sel::getFileTimestamp(
        intel_oem::ipmi::sel::selLogDir / intel_oem::ipmi::sel::selLogFilename);
    uint32_t eraseTimeStamp = intel_oem::ipmi::sel::erase_time::get();
    constexpr uint8_t operationSupport =
        intel_oem::ipmi::sel::selOperationSupport;
    constexpr uint16_t freeSpace =
        0xffff; // Spec indicates that more than 64kB is free

    return ipmi::responseSuccess(selVersion, entries, freeSpace, addTimeStamp,
                                 eraseTimeStamp, operationSupport);
}

using systemEventType = std::tuple<
    uint32_t, // Timestamp
    uint16_t, // Generator ID
    uint8_t,  // EvM Rev
    uint8_t,  // Sensor Type
    uint8_t,  // Sensor Number
    uint7_t,  // Event Type
    bool,     // Event Direction
    std::array<uint8_t, intel_oem::ipmi::sel::systemEventSize>>; // Event Data
using oemTsEventType = std::tuple<
    uint32_t,                                                   // Timestamp
    std::array<uint8_t, intel_oem::ipmi::sel::oemTsEventSize>>; // Event Data
using oemEventType =
    std::array<uint8_t, intel_oem::ipmi::sel::oemEventSize>; // Event Data

ipmi::RspType<uint16_t, // Next Record ID
              uint16_t, // Record ID
              uint8_t,  // Record Type
              std::variant<systemEventType, oemTsEventType,
                           oemEventType>> // Record Content
    ipmiStorageGetSELEntry(uint16_t reservationID, uint16_t targetID,
                           uint8_t offset, uint8_t size)
{
    // Only support getting the entire SEL record. If a partial size or non-zero
    // offset is requested, return an error
    if (offset != 0 || size != ipmi::sel::entireRecord)
    {
        return ipmi::responseRetBytesUnavailable();
    }

    // Check the reservation ID if one is provided or required (only if the
    // offset is non-zero)
    if (reservationID != 0 || offset != 0)
    {
        if (!checkSELReservation(reservationID))
        {
            return ipmi::responseInvalidReservationId();
        }
    }

    // Get the ipmi_sel log files
    std::vector<std::filesystem::path> selLogFiles;
    if (!getSELLogFiles(selLogFiles))
    {
        return ipmi::responseSensorInvalid();
    }

    std::string targetEntry;

    if (targetID == ipmi::sel::firstEntry)
    {
        // The first entry will be at the top of the oldest log file
        std::ifstream logStream(selLogFiles.back());
        if (!logStream.is_open())
        {
            return ipmi::responseUnspecifiedError();
        }

        if (!std::getline(logStream, targetEntry))
        {
            return ipmi::responseUnspecifiedError();
        }
    }
    else if (targetID == ipmi::sel::lastEntry)
    {
        // The last entry will be at the bottom of the newest log file
        std::ifstream logStream(selLogFiles.front());
        if (!logStream.is_open())
        {
            return ipmi::responseUnspecifiedError();
        }

        std::string line;
        while (std::getline(logStream, line))
        {
            targetEntry = line;
        }
    }
    else
    {
        if (!findSELEntry(targetID, selLogFiles, targetEntry))
        {
            return ipmi::responseSensorInvalid();
        }
    }

    // The format of the ipmi_sel message is "<Timestamp>
    // <ID>,<Type>,<EventData>,[<Generator ID>,<Path>,<Direction>]".
    // First get the Timestamp
    size_t space = targetEntry.find_first_of(" ");
    if (space == std::string::npos)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string entryTimestamp = targetEntry.substr(0, space);
    // Then get the log contents
    size_t entryStart = targetEntry.find_first_not_of(" ", space);
    if (entryStart == std::string::npos)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string_view entry(targetEntry);
    entry.remove_prefix(entryStart);
    // Use split to separate the entry into its fields
    std::vector<std::string> targetEntryFields;
    boost::split(targetEntryFields, entry, boost::is_any_of(","),
                 boost::token_compress_on);
    if (targetEntryFields.size() < 3)
    {
        return ipmi::responseUnspecifiedError();
    }
    std::string& recordIDStr = targetEntryFields[0];
    std::string& recordTypeStr = targetEntryFields[1];
    std::string& eventDataStr = targetEntryFields[2];

    uint16_t recordID;
    uint8_t recordType;
    try
    {
        recordID = std::stoul(recordIDStr);
        recordType = std::stoul(recordTypeStr, nullptr, 16);
    }
    catch (const std::invalid_argument&)
    {
        return ipmi::responseUnspecifiedError();
    }
    uint16_t nextRecordID = getNextRecordID(recordID, selLogFiles);
    std::vector<uint8_t> eventDataBytes;
    if (fromHexStr(eventDataStr, eventDataBytes) < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    if (recordType == intel_oem::ipmi::sel::systemEvent)
    {
        // Get the timestamp
        std::tm timeStruct = {};
        std::istringstream entryStream(entryTimestamp);

        uint32_t timestamp = ipmi::sel::invalidTimeStamp;
        if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
        {
            timestamp = std::mktime(&timeStruct);
        }

        // Set the event message revision
        uint8_t evmRev = intel_oem::ipmi::sel::eventMsgRev;

        uint16_t generatorID = 0;
        uint8_t sensorType = 0;
        uint8_t sensorNum = 0xFF;
        uint7_t eventType = 0;
        bool eventDir = 0;
        // System type events should have six fields
        if (targetEntryFields.size() >= 6)
        {
            std::string& generatorIDStr = targetEntryFields[3];
            std::string& sensorPath = targetEntryFields[4];
            std::string& eventDirStr = targetEntryFields[5];

            // Get the generator ID
            try
            {
                generatorID = std::stoul(generatorIDStr, nullptr, 16);
            }
            catch (const std::invalid_argument&)
            {
                std::cerr << "Invalid Generator ID\n";
            }

            // Get the sensor type, sensor number, and event type for the sensor
            sensorType = getSensorTypeFromPath(sensorPath);
            sensorNum = getSensorNumberFromPath(sensorPath);
            eventType = getSensorEventTypeFromPath(sensorPath);

            // Get the event direction
            try
            {
                eventDir = std::stoul(eventDirStr) ? 0 : 1;
            }
            catch (const std::invalid_argument&)
            {
                std::cerr << "Invalid Event Direction\n";
            }
        }

        // Only keep the eventData bytes that fit in the record
        std::array<uint8_t, intel_oem::ipmi::sel::systemEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(
            nextRecordID, recordID, recordType,
            systemEventType{timestamp, generatorID, evmRev, sensorType,
                            sensorNum, eventType, eventDir, eventData});
    }
    else if (recordType >= intel_oem::ipmi::sel::oemTsEventFirst &&
             recordType <= intel_oem::ipmi::sel::oemTsEventLast)
    {
        // Get the timestamp
        std::tm timeStruct = {};
        std::istringstream entryStream(entryTimestamp);

        uint32_t timestamp = ipmi::sel::invalidTimeStamp;
        if (entryStream >> std::get_time(&timeStruct, "%Y-%m-%dT%H:%M:%S"))
        {
            timestamp = std::mktime(&timeStruct);
        }

        // Only keep the bytes that fit in the record
        std::array<uint8_t, intel_oem::ipmi::sel::oemTsEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(nextRecordID, recordID, recordType,
                                     oemTsEventType{timestamp, eventData});
    }
    else if (recordType >= intel_oem::ipmi::sel::oemEventFirst)
    {
        // Only keep the bytes that fit in the record
        std::array<uint8_t, intel_oem::ipmi::sel::oemEventSize> eventData{};
        std::copy_n(eventDataBytes.begin(),
                    std::min(eventDataBytes.size(), eventData.size()),
                    eventData.begin());

        return ipmi::responseSuccess(nextRecordID, recordID, recordType,
                                     eventData);
    }

    return ipmi::responseUnspecifiedError();
}

ipmi::RspType<uint16_t> ipmiStorageAddSELEntry(
    uint16_t recordID, uint8_t recordType, uint32_t timestamp,
    uint16_t generatorID, uint8_t evmRev, uint8_t sensorType, uint8_t sensorNum,
    uint8_t eventType, uint8_t eventData1, uint8_t eventData2,
    uint8_t eventData3)
{
    // Per the IPMI spec, need to cancel any reservation when a SEL entry is
    // added
    cancelSELReservation();

    // Send this request to the Redfish hooks to log it as a Redfish message
    // instead.  There is no need to add it to the SEL, so just return success.
    intel_oem::ipmi::sel::checkRedfishHooks(
        recordID, recordType, timestamp, generatorID, evmRev, sensorType,
        sensorNum, eventType, eventData1, eventData2, eventData3);

    uint16_t responseID = 0xFFFF;
    return ipmi::responseSuccess(responseID);
}

ipmi::RspType<uint8_t> ipmiStorageClearSEL(ipmi::Context::ptr ctx,
                                           uint16_t reservationID,
                                           const std::array<uint8_t, 3>& clr,
                                           uint8_t eraseOperation)
{
    if (!checkSELReservation(reservationID))
    {
        return ipmi::responseInvalidReservationId();
    }

    static constexpr std::array<uint8_t, 3> clrExpected = {'C', 'L', 'R'};
    if (clr != clrExpected)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    // Erasure status cannot be fetched, so always return erasure status as
    // `erase completed`.
    if (eraseOperation == ipmi::sel::getEraseStatus)
    {
        return ipmi::responseSuccess(ipmi::sel::eraseComplete);
    }

    // Check that initiate erase is correct
    if (eraseOperation != ipmi::sel::initiateErase)
    {
        return ipmi::responseInvalidFieldRequest();
    }

    // Per the IPMI spec, need to cancel any reservation when the SEL is
    // cleared
    cancelSELReservation();

    // Save the erase time
    intel_oem::ipmi::sel::erase_time::save();

    // Clear the SEL by deleting the log files
    std::vector<std::filesystem::path> selLogFiles;
    if (getSELLogFiles(selLogFiles))
    {
        for (const std::filesystem::path& file : selLogFiles)
        {
            std::error_code ec;
            std::filesystem::remove(file, ec);
        }
    }

    // Reload rsyslog so it knows to start new log files
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    sdbusplus::message::message rsyslogReload = dbus->new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "ReloadUnit");
    rsyslogReload.append("rsyslog.service", "replace");
    try
    {
        sdbusplus::message::message reloadResponse = dbus->call(rsyslogReload);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
    }

    return ipmi::responseSuccess(ipmi::sel::eraseComplete);
}

ipmi::RspType<uint32_t> ipmiStorageGetSELTime()
{
    struct timespec selTime = {};

    if (clock_gettime(CLOCK_REALTIME, &selTime) < 0)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(selTime.tv_sec);
}

ipmi::RspType<> ipmiStorageSetSELTime(uint32_t selTime)
{
    // Set SEL Time is not supported
    return ipmi::responseInvalidCommand();
}

std::vector<uint8_t> getType12SDRs(uint16_t index, uint16_t recordId)
{
    std::vector<uint8_t> resp;
    if (index == 0)
    {
        Type12Record bmc = {};
        bmc.header.record_id_lsb = recordId;
        bmc.header.record_id_msb = recordId >> 8;
        bmc.header.sdr_version = ipmiSdrVersion;
        bmc.header.record_type = 0x12;
        bmc.header.record_length = 0x1b;
        bmc.slaveAddress = 0x20;
        bmc.channelNumber = 0;
        bmc.powerStateNotification = 0;
        bmc.deviceCapabilities = 0xBF;
        bmc.reserved = 0;
        bmc.entityID = 0x2E;
        bmc.entityInstance = 1;
        bmc.oem = 0;
        bmc.typeLengthCode = 0xD0;
        std::string bmcName = "Basbrd Mgmt Ctlr";
        std::copy(bmcName.begin(), bmcName.end(), bmc.name);
        uint8_t* bmcPtr = reinterpret_cast<uint8_t*>(&bmc);
        resp.insert(resp.end(), bmcPtr, bmcPtr + sizeof(Type12Record));
    }
    else if (index == 1)
    {
        Type12Record me = {};
        me.header.record_id_lsb = recordId;
        me.header.record_id_msb = recordId >> 8;
        me.header.sdr_version = ipmiSdrVersion;
        me.header.record_type = 0x12;
        me.header.record_length = 0x16;
        me.slaveAddress = 0x2C;
        me.channelNumber = 6;
        me.powerStateNotification = 0x24;
        me.deviceCapabilities = 0x21;
        me.reserved = 0;
        me.entityID = 0x2E;
        me.entityInstance = 2;
        me.oem = 0;
        me.typeLengthCode = 0xCB;
        std::string meName = "Mgmt Engine";
        std::copy(meName.begin(), meName.end(), me.name);
        uint8_t* mePtr = reinterpret_cast<uint8_t*>(&me);
        resp.insert(resp.end(), mePtr, mePtr + sizeof(Type12Record));
    }
    else
    {
        throw std::runtime_error("getType12SDRs:: Illegal index " +
                                 std::to_string(index));
    }

    return resp;
}

std::vector<uint8_t> getNMDiscoverySDR(uint16_t index, uint16_t recordId)
{
    std::vector<uint8_t> resp;
    if (index == 0)
    {
        NMDiscoveryRecord nm = {};
        nm.header.record_id_lsb = recordId;
        nm.header.record_id_msb = recordId >> 8;
        nm.header.sdr_version = ipmiSdrVersion;
        nm.header.record_type = 0xC0;
        nm.header.record_length = 0xB;
        nm.oemID0 = 0x57;
        nm.oemID1 = 0x1;
        nm.oemID2 = 0x0;
        nm.subType = 0x0D;
        nm.version = 0x1;
        nm.slaveAddress = 0x2C;
        nm.channelNumber = 0x60;
        nm.healthEventSensor = 0x19;
        nm.exceptionEventSensor = 0x18;
        nm.operationalCapSensor = 0x1A;
        nm.thresholdExceededSensor = 0x1B;

        uint8_t* nmPtr = reinterpret_cast<uint8_t*>(&nm);
        resp.insert(resp.end(), nmPtr, nmPtr + sizeof(NMDiscoveryRecord));
    }
    else
    {
        throw std::runtime_error("getNMDiscoverySDR:: Illegal index " +
                                 std::to_string(index));
    }

    return resp;
}

void registerStorageFunctions()
{
    createTimers();
    startMatch();

    // <Get FRU Inventory Area Info>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetFruInventoryAreaInfo,
                          ipmi::Privilege::User, ipmiStorageGetFruInvAreaInfo);
    // <READ FRU Data>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdReadFruData, ipmi::Privilege::User,
                          ipmiStorageReadFruData);

    // <WRITE FRU Data>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdWriteFruData,
                          ipmi::Privilege::Operator, ipmiStorageWriteFruData);

    // <Get SEL Info>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelInfo, ipmi::Privilege::User,
                          ipmiStorageGetSELInfo);

    // <Get SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelEntry, ipmi::Privilege::User,
                          ipmiStorageGetSELEntry);

    // <Add SEL Entry>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdAddSelEntry,
                          ipmi::Privilege::Operator, ipmiStorageAddSELEntry);

    // <Clear SEL>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdClearSel, ipmi::Privilege::Operator,
                          ipmiStorageClearSEL);

    // <Get SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdGetSelTime, ipmi::Privilege::User,
                          ipmiStorageGetSELTime);

    // <Set SEL Time>
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnStorage,
                          ipmi::storage::cmdSetSelTime,
                          ipmi::Privilege::Operator, ipmiStorageSetSELTime);
}
} // namespace storage
} // namespace ipmi
