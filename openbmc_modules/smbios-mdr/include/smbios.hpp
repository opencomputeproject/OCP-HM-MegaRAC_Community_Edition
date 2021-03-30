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

#pragma once

#include <array>

static constexpr uint16_t smbiosAgentId = 0x0101;
static constexpr int smbiosDirIndex = 0;
static constexpr int firstAgentIndex = 1;
static constexpr uint8_t maxDirEntries = 4;
static constexpr uint32_t pageMask = 0xf000;
static constexpr uint8_t smbiosAgentVersion = 1;
static constexpr uint32_t defaultTimeout = 20000;
static constexpr uint32_t smbiosTableVersion = 15;
static constexpr uint32_t smbiosSMMemoryOffset = 0;
static constexpr uint32_t smbiosSMMemorySize = 1024 * 1024;
static constexpr uint32_t smbiosTableTimestamp = 0x45464748;
static constexpr uint32_t smbiosTableStorageSize = 64 * 1024;
static constexpr const char* smbiosPath = "/var/lib/smbios";
static constexpr uint16_t mdrSMBIOSSize = 32 * 1024;

static constexpr const char* cpuPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/cpu";

static constexpr const char* dimmPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dimm";

static constexpr const char* systemPath =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/bios";

enum class DirDataRequestEnum
{
    dirDataNotRequested = 0x00,
    dirDataRequested = 0x01
};

enum class FlagStatus
{
    flagIsInvalid = 0,
    flagIsValid = 1,
    flagIsLocked = 2
};

typedef enum
{
    biosType = 0,
    systemType = 1,
    baseboardType = 2,
    chassisType = 3,
    processorsType = 4,
    memoryControllerType = 5,
    memoryModuleInformationType = 6,
    cacheType = 7,
    portConnectorType = 8,
    systemSlots = 9,
    onBoardDevicesType = 10,
    oemStringsType = 11,
    systemCconfigurationOptionsType = 12,
    biosLanguageType = 13,
    groupAssociatonsType = 14,
    systemEventLogType = 15,
    physicalMemoryArrayType = 16,
    memoryDeviceType = 17,
} SmbiosType;

static constexpr uint8_t separateLen = 2;

static inline uint8_t* smbiosNextPtr(uint8_t* smbiosDataIn)
{
    if (smbiosDataIn == nullptr)
    {
        return nullptr;
    }
    uint8_t* smbiosData = smbiosDataIn + *(smbiosDataIn + 1);
    int len = 0;
    while ((*smbiosData | *(smbiosData + 1)) != 0)
    {
        smbiosData++;
        len++;
        if (len >= mdrSMBIOSSize) // To avoid endless loop
        {
            return nullptr;
        }
    }
    return smbiosData + separateLen;
}

// When first time run getSMBIOSTypePtr, need to send the RegionS[].regionData
// to smbiosDataIn
static inline uint8_t* getSMBIOSTypePtr(uint8_t* smbiosDataIn, uint8_t typeId)
{
    if (smbiosDataIn == nullptr)
    {
        return nullptr;
    }
    char* smbiosData = reinterpret_cast<char*>(smbiosDataIn);
    while ((*smbiosData != '\0') || (*(smbiosData + 1) != '\0'))
    {
        if (*smbiosData != typeId)
        {
            uint32_t len = *(smbiosData + 1);
            smbiosData += len;
            while ((*smbiosData != '\0') || (*(smbiosData + 1) != '\0'))
            {
                smbiosData++;
                len++;
                if (len >= mdrSMBIOSSize) // To avoid endless loop
                {
                    return nullptr;
                }
            }
            smbiosData += separateLen;
            continue;
        }
        return reinterpret_cast<uint8_t*>(smbiosData);
    }
    return nullptr;
}

static inline std::string positionToString(uint8_t positionNum,
                                           uint8_t structLen, uint8_t* dataIn)
{
    if (dataIn == nullptr)
    {
        return "";
    }
    uint16_t limit = mdrSMBIOSSize; // set a limit to avoid endless loop

    char* target = reinterpret_cast<char*>(dataIn + structLen);
    for (uint8_t index = 1; index < positionNum; index++)
    {
        for (; *target != '\0'; target++)
        {
            limit--;
            if (limit < 1)
            {
                return "";
            }
        }
        target++;
        if (*target == '\0')
        {
            return ""; // 0x00 0x00 means end of the entry.
        }
    }

    std::string result = target;
    return result;
}
