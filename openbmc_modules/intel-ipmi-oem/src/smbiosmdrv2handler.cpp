/*
// Copyright (c) 2018 Intel Corporation
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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <commandutils.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <oemcommands.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/message/types.hpp>
#include <smbiosmdrv2handler.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

std::unique_ptr<MDRV2> mdrv2 = nullptr;
static constexpr const uint8_t ccOemInvalidChecksum = 0x85;
static constexpr size_t dataInfoSize = 16;
static constexpr const uint8_t ccStorageLeak = 0xC4;

static void register_netfn_smbiosmdrv2_functions() __attribute__((constructor));

int MDRV2::agentLookup(const uint16_t& agentId)
{
    int agentIndex = -1;

    if (lastAgentId == agentId)
    {
        return lastAgentIndex;
    }

    if (agentId == smbiosAgentId)
    {
        return firstAgentIndex;
    }

    return agentIndex;
}

int MDRV2::sdplusMdrv2GetProperty(const std::string& name,
                                  std::variant<uint8_t>& value,
                                  const std::string& service)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    sdbusplus::message::message method =
        bus->new_method_call(service.c_str(), mdrv2Path, dbusProperties, "Get");
    method.append(mdrv2Interface, name);

    sdbusplus::message::message reply = bus->call(method);

    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(value);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error get property, sdbusplus call failed",
            phosphor::logging::entry("ERROR=%s", e.what()));
        return -1;
    }

    return 0;
}

int MDRV2::syncDirCommonData(uint8_t idIndex, uint32_t size,
                             const std::string& service)
{
    std::vector<uint32_t> commonData;
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    sdbusplus::message::message method =
        bus->new_method_call(service.c_str(), mdrv2Path, mdrv2Interface,
                             "SynchronizeDirectoryCommonData");
    method.append(idIndex, size);

    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(commonData);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error sync dir common data with service",
            phosphor::logging::entry("ERROR=%s", e.what()));
        return -1;
    }

    if (commonData.size() < syncDirCommonSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error sync dir common data - data length invalid");
        return -1;
    }
    smbiosDir.dir[idIndex].common.dataSetSize = commonData.at(0);
    smbiosDir.dir[idIndex].common.dataVersion = commonData.at(1);
    smbiosDir.dir[idIndex].common.timestamp = commonData.at(2);

    return 0;
}

int MDRV2::findDataId(const uint8_t* dataInfo, const size_t& len,
                      const std::string& service)
{
    int idIndex = -1;

    if (dataInfo == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error dataInfo, input is null point");
        return -1;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "FindIdIndex");
    std::vector<uint8_t> info;
    info.resize(len);
    std::copy(dataInfo, dataInfo + len, info.data());
    method.append(info);

    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(idIndex);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error find id index",
            phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return -1;
    }

    return idIndex;
}

uint16_t MDRV2::getSessionHandle(Mdr2DirStruct* dir)
{
    if (dir == NULL)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Empty dir point");
        return 0;
    }
    dir->sessionHandle++;
    if (dir->sessionHandle == 0)
    {
        dir->sessionHandle = 1;
    }

    return dir->sessionHandle;
}

int MDRV2::findLockHandle(const uint16_t& lockHandle)
{
    int idIndex = -1;

    for (int index = 0; index < smbiosDir.dirEntries; index++)
    {
        if (lockHandle == smbiosDir.dir[index].lockHandle)
        {
            return index;
        }
    }

    return idIndex;
}

bool MDRV2::smbiosIsUpdating(uint8_t index)
{
    if (index > maxDirEntries)
    {
        return false;
    }
    if (smbiosDir.dir[index].stage == MDR2SMBIOSStatusEnum::mdr2Updating)
    {
        return true;
    }

    return false;
}

uint32_t MDRV2::calcChecksum32(uint8_t* buf, uint32_t len)
{
    uint32_t sum = 0;

    if (buf == nullptr)
    {
        return invalidChecksum;
    }

    for (uint32_t index = 0; index < len; index++)
    {
        sum += buf[index];
    }

    return sum;
}

/** @brief implements mdr2 agent status command
 *  @param agentId
 *  @param dirVersion
 *
 *  @returns IPMI completion code plus response data
 *  - mdrVersion
 *  - agentVersion
 *  - dirVersion
 *  - dirEntries
 *  - dataRequest
 */
ipmi::RspType<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>
    mdr2AgentStatus(uint16_t agentId, uint8_t dirVersion)
{
    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    constexpr uint8_t mdrVersion = mdr2Version;
    constexpr uint8_t agentVersion = smbiosAgentVersion;
    uint8_t dirVersionResp = mdrv2->smbiosDir.dirVersion;
    uint8_t dirEntries = mdrv2->smbiosDir.dirEntries;
    uint8_t dataRequest;

    if (mdrv2->smbiosDir.remoteDirVersion != dirVersion)
    {
        mdrv2->smbiosDir.remoteDirVersion = dirVersion;
        dataRequest =
            static_cast<uint8_t>(DirDataRequestEnum::dirDataRequested);
    }
    else
    {
        dataRequest =
            static_cast<uint8_t>(DirDataRequestEnum::dirDataNotRequested);
    }

    return ipmi::responseSuccess(mdrVersion, agentVersion, dirVersionResp,
                                 dirEntries, dataRequest);
}

/** @brief implements mdr2 get directory command
 *  @param agentId
 *  @param dirIndex
 *  @returns IPMI completion code plus response data
 *  - dataOut
 */
ipmi::RspType<std::vector<uint8_t>> mdr2GetDir(uint16_t agentId,
                                               uint8_t dirIndex)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    std::variant<uint8_t> value = static_cast<uint8_t>(0);
    if (0 != mdrv2->sdplusMdrv2GetProperty("DirectoryEntries", value, service))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error getting DirEnries");
        return ipmi::responseUnspecifiedError();
    }
    if (dirIndex > std::get<uint8_t>(value))
    {
        return ipmi::responseParmOutOfRange();
    }

    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "GetDirectoryInformation");

    method.append(dirIndex);

    std::vector<uint8_t> dataOut;
    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(dataOut);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error get dir", phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    constexpr size_t getDirRespSize = 6;
    if (dataOut.size() < getDirRespSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error get dir, response length invalid");
        return ipmi::responseUnspecifiedError();
    }

    if (dataOut.size() > MAX_IPMI_BUFFER) // length + completion code should no
                                          // more than MAX_IPMI_BUFFER
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Data length send from service is invalid");
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(dataOut);
}

ipmi::RspType<bool> mdr2SendDir(uint16_t agentId, uint8_t dirVersion,
                                uint8_t dirIndex, uint8_t returnedEntries,
                                uint8_t remainingEntries,
                                std::array<uint8_t, 16> dataInfo, uint32_t size,
                                uint32_t dataSetSize, uint32_t dataVersion,
                                uint32_t timestamp)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    if ((dirIndex + returnedEntries) > maxDirEntries)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Too many directory entries");
        return ipmi::response(ccStorageLeak);
    }

    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "SendDirectoryInformation");
    method.append(dirVersion, dirIndex, returnedEntries, remainingEntries,
                  dataInfo, size, dataSetSize, dataVersion, timestamp);

    bool terminate = false;
    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(terminate);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error send dir", phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(terminate);
}

/** @brief implements mdr2 get data info command
 *  @param agentId
 *  @param dataInfo
 *
 *  @returns IPMI completion code plus response data
 *  - response - mdrVersion, data info, validFlag,
 *               dataLength, dataVersion, timeStamp
 */
ipmi::RspType<std::vector<uint8_t>>
    mdr2GetDataInfo(uint16_t agentId, std::vector<uint8_t> dataInfo)
{
    constexpr size_t getDataInfoReqSize = 16;

    if (dataInfo.size() < getDataInfoReqSize)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findDataId(dataInfo.data(), dataInfo.size(), service);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "GetDataInformation");

    method.append(idIndex);

    std::vector<uint8_t> res;
    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(res);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error get data info",
            phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    if (res.size() != sizeof(MDRiiGetDataInfoResponse))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Get data info response length not invalid");
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(res);
}

/** @brief implements mdr2 data info offer command
 *  @param agentId - Offer a agent ID to get the "Data Set ID"
 *
 *  @returns IPMI completion code plus response data
 *  - dataOut - data Set Id
 */
ipmi::RspType<std::vector<uint8_t>> mdr2DataInfoOffer(uint16_t agentId)
{
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "GetDataOffer");

    std::vector<uint8_t> dataOut;
    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(dataOut);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error send data info offer",
            phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    constexpr size_t respInfoSize = 16;
    if (dataOut.size() != respInfoSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error send data info offer, return length invalid");
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(dataOut);
}

/** @brief implements mdr2 send data info command
 *  @param agentId
 *  @param dataInfo
 *  @param validFlag
 *  @param dataLength
 *  @param dataVersion
 *  @param timeStamp
 *
 *  @returns IPMI completion code plus response data
 *  - bool
 */
ipmi::RspType<bool> mdr2SendDataInfo(uint16_t agentId,
                                     std::array<uint8_t, dataInfoSize> dataInfo,
                                     uint8_t validFlag, uint32_t dataLength,
                                     uint32_t dataVersion, uint32_t timeStamp)
{
    if (dataLength > smbiosTableStorageSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Requested data length is out of SMBIOS Table storage size.");
        return ipmi::responseParmOutOfRange();
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findDataId(dataInfo.data(), dataInfo.size(), service);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "SendDataInformation");

    method.append((uint8_t)idIndex, validFlag, dataLength, dataVersion,
                  timeStamp);

    bool entryChanged = true;
    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(entryChanged);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error send data info",
            phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    return ipmi::responseSuccess(entryChanged);
}

/**
@brief This command is MDR related get data block command.

@param - agentId
@param - lockHandle
@param - xferOffset
@param - xferLength

@return on success
   - xferLength
   - checksum
   - data
**/
ipmi::RspType<uint32_t,            // xferLength
              uint32_t,            // Checksum
              std::vector<uint8_t> // data
              >
    mdr2GetDataBlock(uint16_t agentId, uint16_t lockHandle, uint32_t xferOffset,
                     uint32_t xferLength)
{
    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findLockHandle(lockHandle);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    if (xferOffset >= mdrv2->smbiosDir.dir[idIndex].common.size)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Offset is outside of range.");
        return ipmi::responseParmOutOfRange();
    }

    size_t outSize = (xferLength > mdrv2->smbiosDir.dir[idIndex].xferSize)
                         ? mdrv2->smbiosDir.dir[idIndex].xferSize
                         : xferLength;
    if (outSize > UINT_MAX - xferOffset)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Out size and offset are out of range");
        return ipmi::responseParmOutOfRange();
    }
    if ((xferOffset + outSize) > mdrv2->smbiosDir.dir[idIndex].common.size)
    {
        outSize = mdrv2->smbiosDir.dir[idIndex].common.size - xferOffset;
    }

    uint32_t respXferLength = outSize;

    if (respXferLength > xferLength)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Get data block unexpected error.");
        return ipmi::responseUnspecifiedError();
    }

    if ((xferOffset + outSize) >
        UINT_MAX -
            reinterpret_cast<size_t>(mdrv2->smbiosDir.dir[idIndex].dataStorage))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Input data to calculate checksum is out of range");
        return ipmi::responseParmOutOfRange();
    }

    uint32_t u32Checksum = mdrv2->calcChecksum32(
        mdrv2->smbiosDir.dir[idIndex].dataStorage + xferOffset, outSize);
    if (u32Checksum == invalidChecksum)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Get data block failed - invalid checksum");
        return ipmi::response(ccOemInvalidChecksum);
    }
    std::vector<uint8_t> data(outSize);

    std::copy(&mdrv2->smbiosDir.dir[idIndex].dataStorage[xferOffset],
              &mdrv2->smbiosDir.dir[idIndex].dataStorage[xferOffset + outSize],
              data.begin());

    return ipmi::responseSuccess(respXferLength, u32Checksum, data);
}

/** @brief implements mdr2 send data block command
 *  @param agentId
 *  @param lockHandle
 *  @param xferOffset
 *  @param xferLength
 *  @param checksum
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> mdr2SendDataBlock(uint16_t agentId, uint16_t lockHandle,
                                  uint32_t xferOffset, uint32_t xferLength,
                                  uint32_t checksum)
{

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findLockHandle(lockHandle);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    if (mdrv2->smbiosIsUpdating(idIndex))
    {
        if (xferOffset > UINT_MAX - xferLength)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Offset and length are out of range");
            return ipmi::responseParmOutOfRange();
        }
        if (((xferOffset + xferLength) >
             mdrv2->smbiosDir.dir[idIndex].maxDataSize) ||
            ((xferOffset + xferLength) >
             mdrv2->smbiosDir.dir[idIndex].common.dataSetSize))
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Send data block Invalid offset/length");
            return ipmi::responseReqDataLenExceeded();
        }
        if (reinterpret_cast<size_t>(
                mdrv2->smbiosDir.dir[idIndex].dataStorage) >
            UINT_MAX - xferOffset)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Offset is out of range");
            return ipmi::responseParmOutOfRange();
        }
        uint8_t* destAddr =
            mdrv2->smbiosDir.dir[idIndex].dataStorage + xferOffset;
        uint8_t* sourceAddr = reinterpret_cast<uint8_t*>(mdrv2->area->vPtr);
        uint32_t calcChecksum = mdrv2->calcChecksum32(sourceAddr, xferLength);
        if (calcChecksum != checksum)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Send data block Invalid checksum");
            return ipmi::response(ccOemInvalidChecksum);
        }
        else
        {
            if (reinterpret_cast<size_t>(sourceAddr) > UINT_MAX - xferLength)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Length is out of range");
                return ipmi::responseParmOutOfRange();
            }
            std::copy(sourceAddr, sourceAddr + xferLength, destAddr);
        }
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Send data block failed, other data is updating");
        return ipmi::responseDestinationUnavailable();
    }

    return ipmi::responseSuccess();
}

bool MDRV2::storeDatatoFlash(MDRSMBIOSHeader* mdrHdr, uint8_t* data)
{
    std::ofstream smbiosFile(mdrType2File,
                             std::ios_base::binary | std::ios_base::trunc);
    if (!smbiosFile.good())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Write data from flash error - Open MDRV2 table file failure");
        return false;
    }

    try
    {
        smbiosFile.write(reinterpret_cast<char*>(mdrHdr),
                         sizeof(MDRSMBIOSHeader));
        smbiosFile.write(reinterpret_cast<char*>(data), mdrHdr->dataSize);
    }
    catch (std::ofstream::failure& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Write data from flash error - write data error",
            phosphor::logging::entry("ERROR=%s", e.what()));
        return false;
    }

    return true;
}

void SharedMemoryArea::Initialize(uint32_t addr, uint32_t areaSize)
{
    int memDriver = 0;

    // open mem driver for the system memory access
    memDriver = open("/dev/vgasharedmem", O_RDONLY);
    if (memDriver < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Cannot access mem driver");
        throw std::system_error(EIO, std::generic_category());
    }

    // map the system memory
    vPtr = mmap(NULL,                       // where to map to: don't mind
                areaSize,                   // how many bytes ?
                PROT_READ,                  // want to read and write
                MAP_SHARED,                 // no copy on write
                memDriver,                  // handle to /dev/mem
                (physicalAddr & pageMask)); // hopefully the Text-buffer :-)

    close(memDriver);
    if (vPtr == MAP_FAILED)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to map share memory");
        throw std::system_error(EIO, std::generic_category());
    }
    size = areaSize;
    physicalAddr = addr;
}

bool MDRV2::smbiosUnlock(uint8_t index)
{
    bool ret;
    switch (smbiosDir.dir[index].stage)
    {
        case MDR2SMBIOSStatusEnum::mdr2Updating:
            smbiosDir.dir[index].stage = MDR2SMBIOSStatusEnum::mdr2Updated;
            smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirUnlock;

            timer->stop();
            smbiosDir.dir[index].lockHandle = 0;
            ret = true;
            break;

        case MDR2SMBIOSStatusEnum::mdr2Updated:
        case MDR2SMBIOSStatusEnum::mdr2Loaded:
            smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirUnlock;

            timer->stop();

            smbiosDir.dir[index].lockHandle = 0;
            ret = true;
            break;

        default:
            break;
    }

    return ret;
}

bool MDRV2::smbiosTryLock(uint8_t flag, uint8_t index, uint16_t* session,
                          uint16_t timeout)
{
    bool ret = false;
    uint32_t u32Status = 0;

    if (timeout == 0)
    {
        timeout = defaultTimeout;
    }
    std::chrono::microseconds usec(timeout * sysClock);

    switch (smbiosDir.dir[index].stage)
    {
        case MDR2SMBIOSStatusEnum::mdr2Updating:
            if (smbiosDir.dir[index].lock != MDR2DirLockEnum::mdr2DirLock)
            {
                smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirLock;
                timer->start(usec);
                lockIndex = index;

                *session = getSessionHandle(&smbiosDir);
                smbiosDir.dir[index].lockHandle = *session;
                ret = true;
            }
            break;
        case MDR2SMBIOSStatusEnum::mdr2Init:
            if (flag)
            {
                smbiosDir.dir[index].stage = MDR2SMBIOSStatusEnum::mdr2Updating;
                smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirUnlock;
                timer->start(usec);
                lockIndex = index;

                *session = getSessionHandle(&smbiosDir);
                smbiosDir.dir[index].lockHandle = *session;
                ret = true;
            }
            break;

        case MDR2SMBIOSStatusEnum::mdr2Updated:
        case MDR2SMBIOSStatusEnum::mdr2Loaded:
            if (smbiosDir.dir[index].lock != MDR2DirLockEnum::mdr2DirLock)
            {
                if (flag)
                {
                    smbiosDir.dir[index].stage =
                        MDR2SMBIOSStatusEnum::mdr2Updating;
                    smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirUnlock;
                }
                else
                {
                    smbiosDir.dir[index].lock = MDR2DirLockEnum::mdr2DirLock;
                }

                timer->start(usec);
                lockIndex = index;

                *session = getSessionHandle(&smbiosDir);
                smbiosDir.dir[index].lockHandle = *session;
                ret = true;
            }
            break;

        default:
            break;
    }
    return ret;
}

void MDRV2::timeoutHandler()
{
    smbiosUnlock(lockIndex);
    mdrv2->area.reset(nullptr);
}

/** @brief implements mdr2 lock data command
 *  @param agentId
 *  @param dataInfo
 *  @param timeout
 *
 *  @returns IPMI completion code plus response data
 *  - mdr2Version
 *  - session
 *  - dataLength
 *  - xferAddress
 *  - xferLength
 */
ipmi::RspType<uint8_t,  // mdr2Version
              uint16_t, // session
              uint32_t, // dataLength
              uint32_t, // xferAddress
              uint32_t  // xferLength
              >
    mdr2LockData(uint16_t agentId, std::array<uint8_t, dataInfoSize> dataInfo,
                 uint16_t timeout)
{
    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    int idIndex = mdrv2->findDataId(dataInfo.data(), dataInfo.size(), service);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    uint16_t session = 0;
    if (!mdrv2->smbiosTryLock(0, idIndex, &session, timeout))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Lock Data failed - cannot lock idIndex");
        return ipmi::responseCommandNotAvailable();
    }

    uint32_t dataLength = mdrv2->smbiosDir.dir[idIndex].common.size;
    uint32_t xferAddress = mdrv2->smbiosDir.dir[idIndex].xferBuff;
    uint32_t xferLength = mdrv2->smbiosDir.dir[idIndex].xferSize;

    return ipmi::responseSuccess(mdr2Version, session, dataLength, xferAddress,
                                 xferLength);
}

/** @brief implements mdr2 unlock data command
 *  @param agentId
 *  @param lockHandle
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> mdr2UnlockData(uint16_t agentId, uint16_t lockHandle)
{
    phosphor::logging::log<phosphor::logging::level::ERR>("unlock data");

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findLockHandle(lockHandle);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    if (!mdrv2->smbiosUnlock(idIndex))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unlock Data failed - cannot unlock idIndex");
        return ipmi::responseCommandNotAvailable();
    }

    return ipmi::responseSuccess();
}

/**
@brief This command is executed after POST BIOS to get the session info.

@param - agentId, dataInfo, dataLength, xferAddress, xferLength, timeout.

@return xferStartAck and session on success.
**/
ipmi::RspType<uint8_t, uint16_t>
    cmd_mdr2_data_start(uint16_t agentId, std::array<uint8_t, 16> dataInfo,
                        uint32_t dataLength, uint32_t xferAddress,
                        uint32_t xferLength, uint16_t timeout)
{
    uint16_t session = 0;

    if (dataLength > smbiosTableStorageSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Requested data length is out of SMBIOS Table storage size.");
        return ipmi::responseParmOutOfRange();
    }
    if ((xferLength + xferAddress) > mdriiSMSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid data address and size");
        return ipmi::responseParmOutOfRange();
    }

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findDataId(dataInfo.data(), dataInfo.size(), service);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    if (mdrv2->smbiosTryLock(1, idIndex, &session, timeout))
    {
        try
        {
            mdrv2->area =
                std::make_unique<SharedMemoryArea>(xferAddress, xferLength);
        }
        catch (const std::system_error& e)
        {
            mdrv2->smbiosUnlock(idIndex);
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Unable to access share memory",
                phosphor::logging::entry("ERROR=%s", e.what()));
            return ipmi::responseUnspecifiedError();
        }
        mdrv2->smbiosDir.dir[idIndex].common.size = dataLength;
        mdrv2->smbiosDir.dir[idIndex].lockHandle = session;
        if (-1 ==
            mdrv2->syncDirCommonData(
                idIndex, mdrv2->smbiosDir.dir[idIndex].common.size, service))
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Unable to sync data to service");
            return ipmi::responseResponseError();
        }
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Canot lock smbios");
        return ipmi::responseUnspecifiedError();
    }

    static constexpr uint8_t xferStartAck = 1;

    return ipmi::responseSuccess(xferStartAck, session);
}

/**
@brief This command is executed to close the session.

@param - agentId, lockHandle.

@return completion code on success.
**/
ipmi::RspType<> cmd_mdr2_data_done(uint16_t agentId, uint16_t lockHandle)
{

    if (mdrv2 == nullptr)
    {
        mdrv2 = std::make_unique<MDRV2>();
    }

    int agentIndex = mdrv2->agentLookup(agentId);
    if (agentIndex == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Unknown agent id", phosphor::logging::entry("ID=%x", agentId));
        return ipmi::responseParmOutOfRange();
    }

    int idIndex = mdrv2->findLockHandle(lockHandle);

    if ((idIndex < 0) || (idIndex >= maxDirEntries))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid Data ID", phosphor::logging::entry("IDINDEX=%x", idIndex));
        return ipmi::responseParmOutOfRange();
    }

    if (!mdrv2->smbiosUnlock(idIndex))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Send data done failed - cannot unlock idIndex");
        return ipmi::responseDestinationUnavailable();
    }

    mdrv2->area.reset(nullptr);
    MDRSMBIOSHeader mdr2Smbios;
    mdr2Smbios.mdrType = mdrTypeII;
    mdr2Smbios.dirVer = mdrv2->smbiosDir.dir[0].common.dataVersion;
    mdr2Smbios.timestamp = mdrv2->smbiosDir.dir[0].common.timestamp;
    mdr2Smbios.dataSize = mdrv2->smbiosDir.dir[0].common.size;

    if (access(smbiosPath, 0) == -1)
    {
        int flag = mkdir(smbiosPath, S_IRWXU);
        if (flag != 0)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "create folder failed for writting smbios file");
        }
    }
    if (!mdrv2->storeDatatoFlash(
            &mdr2Smbios, mdrv2->smbiosDir.dir[smbiosDirIndex].dataStorage))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "MDR2 Store data to flash failed");
        return ipmi::responseDestinationUnavailable();
    }
    bool status = false;
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    std::string service = ipmi::getService(*bus, mdrv2Interface, mdrv2Path);
    sdbusplus::message::message method = bus->new_method_call(
        service.c_str(), mdrv2Path, mdrv2Interface, "AgentSynchronizeData");

    try
    {
        sdbusplus::message::message reply = bus->call(method);
        reply.read(status);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error Sync data with service",
            phosphor::logging::entry("ERROR=%s", e.what()),
            phosphor::logging::entry("SERVICE=%s", service.c_str()),
            phosphor::logging::entry("PATH=%s", mdrv2Path));
        return ipmi::responseResponseError();
    }

    if (!status)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Sync data with service failure");
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

static void register_netfn_smbiosmdrv2_functions(void)
{
    // MDR V2 Command
    // <Get MDRII Status Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIAgentStatus,
                          ipmi::Privilege::Operator, mdr2AgentStatus);

    // <Get MDRII Directory Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIGetDir,
                          ipmi::Privilege::Operator, mdr2GetDir);

    // <Send MDRII Directory Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIISendDir,
                          ipmi::Privilege::Operator, mdr2SendDir);

    // <Get MDRII Data Info Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIGetDataInfo,
                          ipmi::Privilege::Operator, mdr2GetDataInfo);

    // <Send MDRII Info Offer>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIISendDataInfoOffer,
                          ipmi::Privilege::Operator, mdr2DataInfoOffer);

    // <Send MDRII Data Info>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIISendDataInfo,
                          ipmi::Privilege::Operator, mdr2SendDataInfo);

    // <Get MDRII Data Block Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIGetDataBlock,
                          ipmi::Privilege::Operator, mdr2GetDataBlock);

    // <Send MDRII Data Block>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIISendDataBlock,
                          ipmi::Privilege::Operator, mdr2SendDataBlock);

    // <Lock MDRII Data Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIILockData,
                          ipmi::Privilege::Operator, mdr2LockData);

    // <Unlock MDRII Data Command>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIUnlockData,
                          ipmi::Privilege::Operator, mdr2UnlockData);

    // <Send MDRII Data Start>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIDataStart,
                          ipmi::Privilege::Operator, cmd_mdr2_data_start);

    // <Send MDRII Data Done>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::intel::netFnApp,
                          ipmi::intel::app::cmdMdrIIDataDone,
                          ipmi::Privilege::Operator, cmd_mdr2_data_done);
}
