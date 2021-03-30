/*
// Copyright (c) 2019 Intel Corporation
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

#include "mdrv2.hpp"

#include <sys/mman.h>

#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Smbios/MDR_V2/error.hpp>

namespace phosphor
{
namespace smbios
{

std::vector<uint8_t> MDR_V2::getDirectoryInformation(uint8_t dirIndex)
{
    std::vector<uint8_t> responseDir;
    if (dirIndex > smbiosDir.dirEntries)
    {
        responseDir.push_back(0);
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }
    responseDir.push_back(mdr2Version);
    responseDir.push_back(smbiosDir.dirVersion);
    uint8_t returnedEntries = smbiosDir.dirEntries - dirIndex;
    responseDir.push_back(returnedEntries);
    if ((dirIndex + returnedEntries) >= smbiosDir.dirEntries)
    {
        responseDir.push_back(0);
    }
    else
    {
        responseDir.push_back(smbiosDir.dirEntries - dirIndex -
                              returnedEntries);
    }
    for (uint8_t index = dirIndex; index < smbiosDir.dirEntries; index++)
    {
        for (uint8_t indexId = 0; indexId < sizeof(DataIdStruct); indexId++)
        {
            responseDir.push_back(
                smbiosDir.dir[index].common.id.dataInfo[indexId]);
        }
    }

    return responseDir;
}

bool MDR_V2::smbiosIsAvailForUpdate(uint8_t index)
{
    bool ret = false;
    if (index > maxDirEntries)
    {
        return ret;
    }

    switch (smbiosDir.dir[index].stage)
    {
        case MDR2SMBIOSStatusEnum::mdr2Updating:
            ret = false;
            break;

        case MDR2SMBIOSStatusEnum::mdr2Init:
            // This *looks* like there should be a break statement here,
            // as the effects of the previous statement are a noop given
            // the following code that this falls through to.
            // We've elected not to change it, though, since it's been
            // this way since the old generation, and it would affect
            // the way the code functions.
            // If it ain't broke, don't fix it.

        case MDR2SMBIOSStatusEnum::mdr2Loaded:
        case MDR2SMBIOSStatusEnum::mdr2Updated:
            if (smbiosDir.dir[index].lock == MDR2DirLockEnum::mdr2DirLock)
            {
                ret = false;
            }
            else
            {
                ret = true;
            }
            break;

        default:
            break;
    }

    return ret;
}

std::vector<uint8_t> MDR_V2::getDataOffer()
{
    std::vector<uint8_t> offer(sizeof(DataIdStruct));
    if (smbiosIsAvailForUpdate(0))
    {
        std::copy(smbiosDir.dir[0].common.id.dataInfo,
                  &smbiosDir.dir[0].common.id.dataInfo[16], offer.data());
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "smbios is not ready for update");
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            UpdateInProgress();
    }
    return offer;
}

inline uint8_t MDR_V2::smbiosValidFlag(uint8_t index)
{
    FlagStatus ret = FlagStatus::flagIsInvalid;
    MDR2SMBIOSStatusEnum stage = smbiosDir.dir[index].stage;
    MDR2DirLockEnum lock = smbiosDir.dir[index].lock;

    switch (stage)
    {
        case MDR2SMBIOSStatusEnum::mdr2Loaded:
        case MDR2SMBIOSStatusEnum::mdr2Updated:
            if (lock == MDR2DirLockEnum::mdr2DirLock)
            {
                ret = FlagStatus::flagIsLocked; // locked
            }
            else
            {
                ret = FlagStatus::flagIsValid; // valid
            }
            break;

        case MDR2SMBIOSStatusEnum::mdr2Updating:
        case MDR2SMBIOSStatusEnum::mdr2Init:
            ret = FlagStatus::flagIsInvalid; // invalid
            break;

        default:
            break;
    }

    return static_cast<uint8_t>(ret);
}

std::vector<uint8_t> MDR_V2::getDataInformation(uint8_t idIndex)
{
    std::vector<uint8_t> responseInfo;
    responseInfo.push_back(mdr2Version);

    if (idIndex >= maxDirEntries)
    {
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }

    for (uint8_t index = 0; index < sizeof(DataIdStruct); index++)
    {
        responseInfo.push_back(
            smbiosDir.dir[idIndex].common.id.dataInfo[index]);
    }
    responseInfo.push_back(smbiosValidFlag(idIndex));
    responseInfo.push_back(smbiosDir.dir[idIndex].common.size);
    responseInfo.push_back(smbiosDir.dir[idIndex].common.dataVersion);
    responseInfo.push_back(smbiosDir.dir[idIndex].common.timestamp);

    return responseInfo;
}

bool MDR_V2::sendDirectoryInformation(uint8_t dirVersion, uint8_t dirIndex,
                                      uint8_t returnedEntries,
                                      uint8_t remainingEntries,
                                      std::vector<uint8_t> dirEntry)
{
    bool teminate = false;
    if ((dirIndex >= maxDirEntries) || (returnedEntries < 1))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Send Dir info failed - input parameter invalid");
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }
    if (dirEntry.size() < sizeof(Mdr2DirEntry))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Directory size invalid");
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }
    if (dirVersion == smbiosDir.dirVersion)
    {
        teminate = true;
    }
    else
    {
        if (remainingEntries > 0)
        {
            teminate = false;
        }
        else
        {
            teminate = true;
            smbiosDir.dirVersion = dirVersion;
        }
        uint8_t idIndex = dirIndex;

        uint8_t* pData = dirEntry.data();
        if (pData == nullptr)
        {
            return false;
        }
        for (uint8_t index = 0; index < returnedEntries; index++)
        {
            auto data = reinterpret_cast<const Mdr2DirEntry*>(pData);
            smbiosDir.dir[idIndex + index].common.dataVersion =
                data->dataVersion;
            std::copy(data->id.dataInfo,
                      data->id.dataInfo + sizeof(DataIdStruct),
                      smbiosDir.dir[idIndex + index].common.id.dataInfo);
            smbiosDir.dir[idIndex + index].common.dataSetSize = data->size;
            smbiosDir.dir[idIndex + index].common.timestamp = data->timestamp;
            pData += sizeof(returnedEntries);
        }
    }
    return teminate;
}

bool MDR_V2::readDataFromFlash(MDRSMBIOSHeader* mdrHdr, uint8_t* data)
{
    if (mdrHdr == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Read data from flash error - Invalid mdr header");
        return false;
    }
    if (data == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Read data from flash error - Invalid data point");
        return false;
    }
    std::ifstream smbiosFile(mdrType2File, std::ios_base::binary);
    if (!smbiosFile.good())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Read data from flash error - Open MDRV2 table file failure");
        return false;
    }
    smbiosFile.clear();
    smbiosFile.seekg(0, std::ios_base::end);
    int fileLength = smbiosFile.tellg();
    smbiosFile.seekg(0, std::ios_base::beg);
    if (fileLength < sizeof(MDRSMBIOSHeader))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "MDR V2 file size is smaller than mdr header");
        return false;
    }
    smbiosFile.read(reinterpret_cast<char*>(mdrHdr), sizeof(MDRSMBIOSHeader));
    if (mdrHdr->dataSize > smbiosTableStorageSize)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Data size out of limitation");
        smbiosFile.close();
        return false;
    }
    fileLength -= sizeof(MDRSMBIOSHeader);
    if (fileLength < mdrHdr->dataSize)
    {
        smbiosFile.read(reinterpret_cast<char*>(data), fileLength);
    }
    else
    {
        smbiosFile.read(reinterpret_cast<char*>(data), mdrHdr->dataSize);
    }
    smbiosFile.close();
    return true;
}

bool MDR_V2::sendDataInformation(uint8_t idIndex, uint8_t flag,
                                 uint32_t dataLen, uint32_t dataVer,
                                 uint32_t timeStamp)
{
    if (idIndex >= maxDirEntries)
    {
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }
    int entryChanged = 0;
    if (smbiosDir.dir[idIndex].common.dataSetSize != dataLen)
    {
        entryChanged++;
        smbiosDir.dir[idIndex].common.dataSetSize = dataLen;
    }

    if (smbiosDir.dir[idIndex].common.dataVersion != dataVer)
    {
        entryChanged++;
        smbiosDir.dir[idIndex].common.dataVersion = dataVer;
    }

    if (smbiosDir.dir[idIndex].common.timestamp != timeStamp)
    {
        entryChanged++;
        smbiosDir.dir[idIndex].common.timestamp = timeStamp;
    }
    if (entryChanged == 0)
    {
        return false;
    }
    return true;
}

int MDR_V2::findIdIndex(std::vector<uint8_t> dataInfo)
{
    if (dataInfo.size() != sizeof(DataIdStruct))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Length of dataInfo invalid");
        throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::
            InvalidParameter();
    }
    std::array<uint8_t, 16> arrayDataInfo;

    std::copy(dataInfo.begin(), dataInfo.end(), arrayDataInfo.begin());

    for (int index = 0; index < smbiosDir.dirEntries; index++)
    {
        int info = 0;
        for (; info < arrayDataInfo.size(); info++)
        {
            if (arrayDataInfo[info] !=
                smbiosDir.dir[index].common.id.dataInfo[info])
            {
                break;
            }
        }
        if (info == arrayDataInfo.size())
        {
            return index;
        }
    }
    throw sdbusplus::xyz::openbmc_project::Smbios::MDR_V2::Error::InvalidId();
}

uint8_t MDR_V2::directoryEntries(uint8_t value)
{
    value = smbiosDir.dirEntries;
    return sdbusplus::xyz::openbmc_project::Smbios::server::MDR_V2::
        directoryEntries(value);
}

void MDR_V2::systemInfoUpdate()
{
    cpus.clear();

    int num = getTotalCpuSlot();
    if (num == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "get cpu total slot failed");
        return;
    }

    for (int index = 0; index < num; index++)
    {
        std::string path = cpuPath + std::to_string(index);
        cpus.emplace_back(std::make_unique<phosphor::smbios::Cpu>(
            bus, path, index, smbiosDir.dir[smbiosDirIndex].dataStorage));
    }

    dimms.clear();

    num = getTotalDimmSlot();
    if (num == -1)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "get dimm total slot failed");
        return;
    }

    for (int index = 0; index < num; index++)
    {
        std::string path = dimmPath + std::to_string(index);
        dimms.emplace_back(std::make_unique<phosphor::smbios::Dimm>(
            bus, path, index, smbiosDir.dir[smbiosDirIndex].dataStorage));
    }

    system.reset();
    system = std::make_unique<System>(
        bus, systemPath, smbiosDir.dir[smbiosDirIndex].dataStorage);
}

int MDR_V2::getTotalCpuSlot()
{
    uint8_t* dataIn = smbiosDir.dir[smbiosDirIndex].dataStorage;
    int num = 0;

    if (dataIn == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "get cpu total slot failed - no storage data");
        return -1;
    }

    while (1)
    {
        dataIn = getSMBIOSTypePtr(dataIn, processorsType);
        if (dataIn == nullptr)
        {
            break;
        }
        num++;
        dataIn = smbiosNextPtr(dataIn);
        if (dataIn == nullptr)
        {
            break;
        }
        if (num >= limitEntryLen)
        {
            break;
        }
    }

    return num;
}

int MDR_V2::getTotalDimmSlot()
{
    uint8_t* dataIn = smbiosDir.dir[smbiosDirIndex].dataStorage;
    uint8_t num = 0;

    if (dataIn == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Fail to get dimm total slot - no storage data");
        return -1;
    }

    while (1)
    {
        dataIn = getSMBIOSTypePtr(dataIn, memoryDeviceType);
        if (dataIn == nullptr)
        {
            break;
        }
        num++;
        dataIn = smbiosNextPtr(dataIn);
        if (dataIn == nullptr)
        {
            break;
        }
        if (num >= limitEntryLen)
        {
            break;
        }
    }

    return num;
}

bool MDR_V2::agentSynchronizeData()
{
    struct MDRSMBIOSHeader mdr2SMBIOS;
    bool status = readDataFromFlash(&mdr2SMBIOS,
                                    smbiosDir.dir[smbiosDirIndex].dataStorage);
    if (!status)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "agent data sync failed - read data from flash failed");
        return false;
    }
    else
    {
        systemInfoUpdate();
        smbiosDir.dir[smbiosDirIndex].common.dataVersion = mdr2SMBIOS.dirVer;
        smbiosDir.dir[smbiosDirIndex].common.timestamp = mdr2SMBIOS.timestamp;
        smbiosDir.dir[smbiosDirIndex].common.size = mdr2SMBIOS.dataSize;
        smbiosDir.dir[smbiosDirIndex].stage = MDR2SMBIOSStatusEnum::mdr2Loaded;
        smbiosDir.dir[smbiosDirIndex].lock = MDR2DirLockEnum::mdr2DirUnlock;
    }
    timer.stop();
    return true;
}

std::vector<uint32_t> MDR_V2::synchronizeDirectoryCommonData(uint8_t idIndex,
                                                             uint32_t size)
{
    std::chrono::microseconds usec(
        defaultTimeout); // default lock time out is 2s
    std::vector<uint32_t> result;
    smbiosDir.dir[idIndex].common.size = size;
    result.push_back(smbiosDir.dir[idIndex].common.dataSetSize);
    result.push_back(smbiosDir.dir[idIndex].common.dataVersion);
    result.push_back(smbiosDir.dir[idIndex].common.timestamp);

    timer.start(usec);
    return result;
}

} // namespace smbios
} // namespace phosphor
