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

#include "cpu.hpp"

#include <iostream>
#include <map>

namespace phosphor
{
namespace smbios
{

void Cpu::cpuSocket(const uint8_t positionNum, const uint8_t structLen,
                    uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    processorSocket(result);
}

std::string Cpu::processorSocket(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorSocket(value);
}

void Cpu::cpuType(const uint8_t value)
{
    std::map<uint8_t, const char*>::const_iterator it =
        processorTypeTable.find(value);
    if (it == processorTypeTable.end())
    {
        processorType("Unknown Processor Type");
    }
    else
    {
        processorType(it->second);
    }
}

std::string Cpu::processorType(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorType(value);
}

void Cpu::cpuFamily(const uint8_t value)
{
    std::map<uint8_t, const char*>::const_iterator it = familyTable.find(value);
    if (it == familyTable.end())
    {
        processorFamily("Unknown Processor Family");
    }
    else
    {
        processorFamily(it->second);
    }
}

std::string Cpu::processorFamily(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorFamily(value);
}

void Cpu::cpuManufacturer(const uint8_t positionNum, const uint8_t structLen,
                          uint8_t* dataIn)
{
    std::string result = positionToString(positionNum, structLen, dataIn);

    manufacturer(result);
}

std::string Cpu::manufacturer(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
        Asset::manufacturer(value);
}

uint32_t Cpu::processorId(uint32_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorId(value);
}

void Cpu::cpuVersion(const uint8_t positionNum, const uint8_t structLen,
                     uint8_t* dataIn)
{
    std::string result;

    result = positionToString(positionNum, structLen, dataIn);

    processorVersion(result);
}

std::string Cpu::processorVersion(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorVersion(value);
}

uint16_t Cpu::processorMaxSpeed(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorMaxSpeed(value);
}

void Cpu::cpuCharacteristics(uint16_t value)
{
    std::string result = "";
    for (uint8_t index = 0; index < (8 * sizeof(value)); index++)
    {
        if (value & 0x01)
        {
            result += characteristicsTable[index];
        }
        value >>= 1;
    }

    processorCharacteristics(result);
}

std::string Cpu::processorCharacteristics(std::string value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorCharacteristics(value);
}

uint16_t Cpu::processorCoreCount(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorCoreCount(value);
}

uint16_t Cpu::processorThreadCount(uint16_t value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu::
        processorThreadCount(value);
}

static constexpr const uint8_t populateMask = 1 << 6;
static constexpr const uint8_t statusMask = 0x07;
void Cpu::cpuStatus(uint8_t value)
{
    if (!(value & populateMask))
    {
        present(false);
        functional(false);
        return;
    }
    present(true);
    if ((value & statusMask) == 1)
    {
        functional(true);
    }
    else
    {
        functional(false);
    }
}

bool Cpu::present(bool value)
{
    return sdbusplus::xyz::openbmc_project::Inventory::server::Item::present(
        value);
}

bool Cpu::functional(bool value)
{
    return sdbusplus::xyz::openbmc_project::State::Decorator::server::
        OperationalStatus::functional(value);
}

static constexpr uint8_t maxOldVersionCount = 0xff;
void Cpu::processorInfoUpdate(void)
{
    uint8_t* dataIn = storage;

    dataIn = getSMBIOSTypePtr(dataIn, processorsType);
    if (dataIn == nullptr)
    {
        return;
    }

    for (uint8_t index = 0; index < cpuNum; index++)
    {
        dataIn = smbiosNextPtr(dataIn);
        if (dataIn == nullptr)
        {
            return;
        }
        dataIn = getSMBIOSTypePtr(dataIn, processorsType);
        if (dataIn == nullptr)
        {
            return;
        }
    }

    auto cpuInfo = reinterpret_cast<struct ProcessorInfo*>(dataIn);

    cpuSocket(cpuInfo->socketDesignation, cpuInfo->length,
              dataIn);               // offset 4h
    cpuType(cpuInfo->processorType); // offset 5h
    cpuFamily(cpuInfo->family);      // offset 6h
    cpuManufacturer(cpuInfo->manufacturer, cpuInfo->length,
                    dataIn);                               // offset 7h
    processorId(cpuInfo->id);                              // offset 8h
    cpuVersion(cpuInfo->version, cpuInfo->length, dataIn); // offset 10h
    processorMaxSpeed(cpuInfo->maxSpeed);                  // offset 14h
    if (cpuInfo->coreCount < maxOldVersionCount)           // offset 23h or 2Ah
    {
        processorCoreCount(cpuInfo->coreCount);
    }
    else
    {
        processorCoreCount(cpuInfo->coreCount2);
    }

    if (cpuInfo->threadCount < maxOldVersionCount) // offset 25h or 2Eh)
    {
        processorThreadCount(cpuInfo->threadCount);
    }
    else
    {
        processorThreadCount(cpuInfo->threadCount2);
    }

    cpuCharacteristics(cpuInfo->characteristics); // offset 26h

    cpuStatus(cpuInfo->status);
}

} // namespace smbios
} // namespace phosphor
