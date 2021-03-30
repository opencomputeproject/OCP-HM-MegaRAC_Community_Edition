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

#include "dimm.hpp"

#include "mdrv2.hpp"

namespace phosphor
{
namespace smbios
{

using DeviceType =
    sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::DeviceType;

static constexpr uint16_t maxOldDimmSize = 0x7fff;
void Dimm::memoryInfoUpdate(void)
{
    uint8_t* dataIn = storage;

    dataIn = getSMBIOSTypePtr(dataIn, memoryDeviceType);

    if (dataIn == nullptr)
    {
        return;
    }
    for (uint8_t index = 0; index < dimmNum; index++)
    {
        dataIn = smbiosNextPtr(dataIn);
        if (dataIn == nullptr)
        {
            return;
        }
        dataIn = getSMBIOSTypePtr(dataIn, memoryDeviceType);
        if (dataIn == nullptr)
        {
            return;
        }
    }

    auto memoryInfo = reinterpret_cast<struct MemoryInfo*>(dataIn);

    memoryDataWidth(memoryInfo->dataWidth);

    if (memoryInfo->size == maxOldDimmSize)
    {
        dimmSizeExt(memoryInfo->extendedSize);
    }
    else
    {
        dimmSize(memoryInfo->size);
    }

    dimmDeviceLocator(memoryInfo->deviceLocator, memoryInfo->length, dataIn);
    dimmType(memoryInfo->memoryType);
    dimmTypeDetail(memoryInfo->typeDetail);
    maxMemorySpeedInMhz(memoryInfo->speed);
    dimmManufacturer(memoryInfo->manufacturer, memoryInfo->length, dataIn);
    dimmSerialNum(memoryInfo->serialNum, memoryInfo->length, dataIn);
    dimmPartNum(memoryInfo->partNum, memoryInfo->length, dataIn);
    memoryAttributes(memoryInfo->attributes);
    memoryConfiguredSpeedInMhz(memoryInfo->confClockSpeed);

    return;
}

uint16_t Dimm::memoryDataWidth(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryDataWidth(value);
}

static constexpr uint16_t baseNewVersionDimmSize = 0x8000;
static constexpr uint16_t dimmSizeUnit = 1024;
void Dimm::dimmSize(const uint16_t size)
{
    uint32_t result = size & maxOldDimmSize;
    if (0 == (size & baseNewVersionDimmSize))
    {
        result = result * dimmSizeUnit;
    }
    memorySizeInKB(result);
}

void Dimm::dimmSizeExt(uint32_t size)
{
    size = size * dimmSizeUnit;
    memorySizeInKB(size);
}

uint32_t Dimm::memorySizeInKB(uint32_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memorySizeInKB(value);
}

void Dimm::dimmDeviceLocator(const uint8_t positionNum, const uint8_t structLen,
                             uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    memoryDeviceLocator(result);
}

std::string Dimm::memoryDeviceLocator(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryDeviceLocator(value);
}

void Dimm::dimmType(const uint8_t type)
{
    std::map<uint8_t, DeviceType>::const_iterator it = dimmTypeTable.find(type);
    if (it == dimmTypeTable.end())
    {
        memoryType(DeviceType::Unknown);
    }
    else
    {
        memoryType(it->second);
    }
}

DeviceType Dimm::memoryType(DeviceType value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryType(value);
}

void Dimm::dimmTypeDetail(uint16_t detail)
{
    std::string result;
    for (uint8_t index = 0; index < (8 * sizeof(detail)); index++)
    {
        if (detail & 0x01)
        {
            result += detailTable[index];
        }
        detail >>= 1;
    }
    memoryTypeDetail(result);
}

std::string Dimm::memoryTypeDetail(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryTypeDetail(value);
}

uint16_t Dimm::maxMemorySpeedInMhz(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        maxMemorySpeedInMhz(value);
}

void Dimm::dimmManufacturer(const uint8_t positionNum, const uint8_t structLen,
                            uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    manufacturer(result);
}

std::string Dimm::manufacturer(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
        Asset::manufacturer(value);
}

void Dimm::dimmSerialNum(const uint8_t positionNum, const uint8_t structLen,
                         uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    serialNumber(result);
}

std::string Dimm::serialNumber(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
        Asset::serialNumber(value);
}

void Dimm::dimmPartNum(const uint8_t positionNum, const uint8_t structLen,
                       uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    partNumber(result);
}

std::string Dimm::partNumber(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
        Asset::partNumber(value);
}

uint8_t Dimm::memoryAttributes(uint8_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryAttributes(value);
}

uint16_t Dimm::memoryConfiguredSpeedInMhz(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Dimm::
        memoryConfiguredSpeedInMhz(value);
}

} // namespace smbios
} // namespace phosphor
