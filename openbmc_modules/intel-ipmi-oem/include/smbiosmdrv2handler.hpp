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

#pragma once
#include <ipmid/api.h>
#include <sys/mman.h>

#include <oemcommands.hpp>
#include <sdbusplus/timer.hpp>

static constexpr const char* mdrType2File = "/var/lib/smbios/smbios2";
static constexpr const char* smbiosPath = "/var/lib/smbios";
static constexpr const size_t msgPayloadSize =
    1024 * 60; // Total size will transfer for smbios table
static constexpr const size_t mdriiSMSize = 0x00100000;

static constexpr const uint16_t smbiosAgentId =
    0x0101; // Agent ID for smbios entry
static constexpr const int firstAgentIndex = 1;

static constexpr const uint8_t maxDirEntries = 4; // Maximum directory entries
static constexpr const uint32_t mdr2SMSize =
    0x00100000; // Size of VGA share memory
static constexpr const uint32_t mdr2SMBaseAddress =
    0x9FF00000; // Base address of VGA share memory

static constexpr const uint8_t mdrTypeII = 2; // MDR V2 type

static constexpr const uint8_t mdr2Version = 2;        // MDR V2 versoin
static constexpr const uint8_t smbiosAgentVersion = 1; // Agent version of
                                                       // smbios

static constexpr const uint32_t pageMask =
    0xf000; // To make data become n times of page
static constexpr const int smbiosDirIndex = 0; // SMBIOS directory index

static constexpr const uint32_t smbiosTableVersion =
    15; // Version of smbios table
static constexpr const uint32_t smbiosTableTimestamp =
    0x45464748; // Time stamp when smbios table created
static constexpr const size_t smbiosSMMemoryOffset =
    0; // Offset of VGA share memory
static constexpr const size_t smbiosSMMemorySize =
    1024 * 1024; // Total size of VGA share memory
static constexpr const size_t smbiosTableStorageSize =
    64 * 1024; // Total size of smbios table
static constexpr const uint32_t defaultTimeout = 200;
static constexpr const uint8_t sysClock = 100;
static constexpr const int lastAgentIndex = -1;
static constexpr const uint16_t lastAgentId = 0xFFFF;
constexpr const uint32_t invalidChecksum = 0xffffffff;
constexpr const char* dbusProperties = "org.freedesktop.DBus.Properties";
constexpr const char* mdrv2Path = "/xyz/openbmc_project/Smbios/MDR_V2";
constexpr const char* mdrv2Interface = "xyz.openbmc_project.Smbios.MDR_V2";

enum class MDR2SMBIOSStatusEnum
{
    mdr2Init = 0,
    mdr2Loaded = 1,
    mdr2Updated = 2,
    mdr2Updating = 3
};

enum class DirDataRequestEnum
{
    dirDataNotRequested = 0x00,
    dirDataRequested = 0x01
};

enum MDR2DirLockEnum
{
    mdr2DirUnlock = 0,
    mdr2DirLock = 1
};

#pragma pack(push)
#pragma pack(1)

struct MDRSMBIOSHeader
{
    uint8_t dirVer;
    uint8_t mdrType;
    uint32_t timestamp;
    uint32_t dataSize;
};

struct DataIdStruct
{
    uint8_t dataInfo[16];
};

struct Mdr2DirEntry
{
    DataIdStruct id;
    uint32_t size;
    uint32_t dataSetSize;
    uint32_t dataVersion;
    uint32_t timestamp;
};

struct Mdr2DirLocalStruct
{
    Mdr2DirEntry common;
    MDR2SMBIOSStatusEnum stage;
    MDR2DirLockEnum lock;
    uint16_t lockHandle;
    uint32_t xferBuff;
    uint32_t xferSize;
    uint32_t maxDataSize;
    uint8_t* dataStorage;
};

struct Mdr2DirStruct
{
    uint8_t agentVersion;
    uint8_t dirVersion;
    uint8_t dirEntries;
    uint8_t status; // valid / locked / etc
    uint8_t remoteDirVersion;
    uint16_t sessionHandle;
    Mdr2DirLocalStruct dir[maxDirEntries];
};

// Three members include dataSetSize, dataVersion and timestamp
static constexpr const size_t syncDirCommonSize = 3;

// ====================== MDR II Pull Command Structures ======================
struct MDRiiGetDataInfoRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
};

// MDR II data set information inquiry response
struct MDRiiGetDataInfoResponse
{
    uint8_t mdrVersion;
    DataIdStruct dataSetId;
    uint8_t validFlag;
    uint32_t dataLength;
    uint32_t dataVersion;
    uint32_t timeStamp;
};

// ====================== MDR II Push Command Structures ======================
// MDR II Client send data set info offer response
struct MDRiiOfferDataInfoResponse
{
    DataIdStruct dataSetInfo;
};

// MDR II Push Agent send data set info command
struct MDRiiSendDataInfoRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint8_t validFlag;
    uint32_t dataLength;
    uint32_t dataVersion; // Roughly equivalent to the "file name"
    uint32_t
        timeStamp; // More info on the identity of this particular set of data
};

// MDR II Pull Agent lock data set command
struct MDRiiLockDataRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint16_t timeout;
};

// MDR II Pull Agent lock data set response
struct MDRiiLockDataResponse
{
    uint8_t mdrVersion;
    uint16_t lockHandle;
    uint32_t dataLength;
    uint32_t xferAddress;
    uint32_t xferLength;
};

// MDR II Push Agent send data start command
struct MDRiiDataStartRequest
{
    uint16_t agentId;
    DataIdStruct dataSetInfo;
    uint32_t dataLength;
    uint32_t xferAddress;
    uint32_t xferLength;
    uint16_t timeout;
};

// MDR II Client send data start response
struct MDRiiDataStartResponse
{
    uint8_t xferStartAck;
    uint16_t sessionHandle;
};

// MDR II
struct MDRiiDataDoneRequest
{
    uint16_t agentId;
    uint16_t lockHandle;
};

#pragma pack(pop)

class SharedMemoryArea
{
  public:
    SharedMemoryArea(uint32_t addr, uint32_t areaSize) :
        vPtr(nullptr), physicalAddr(addr), size(areaSize)
    {
        Initialize(addr, areaSize);
    }

    ~SharedMemoryArea()
    {
        if ((vPtr != nullptr) && (vPtr != MAP_FAILED))
        {
            if (0 != munmap(vPtr, size))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Ummap share memory failed");
            }
        }
    }

    void* vPtr;

  private:
    uint32_t physicalAddr;
    uint32_t size;

    void Initialize(uint32_t addr, uint32_t areaSize);
};

class MDRV2
{
  public:
    MDRV2()
    {
        timer =
            std::make_unique<phosphor::Timer>([&](void) { timeoutHandler(); });
    }

    int agentLookup(const uint16_t& agentId);
    int findLockHandle(const uint16_t& lockHandle);
    int syncDirCommonData(uint8_t idIndex, uint32_t size,
                          const std::string& service);
    int findDataId(const uint8_t* dataInfo, const size_t& len,
                   const std::string& service);
    uint16_t getSessionHandle(Mdr2DirStruct* dir);
    bool smbiosIsUpdating(uint8_t index);
    uint32_t calcChecksum32(uint8_t* buf, uint32_t len);
    bool storeDatatoFlash(MDRSMBIOSHeader* mdrHdr, uint8_t* data);
    bool smbiosUnlock(uint8_t index);
    void timeoutHandler();
    bool smbiosTryLock(uint8_t flag, uint8_t index, uint16_t* session,
                       uint16_t timeout);
    int sdplusMdrv2GetProperty(const std::string& name,
                               std::variant<uint8_t>& value,
                               const std::string& service);

    Mdr2DirStruct smbiosDir{smbiosAgentVersion,
                            1,
                            1,
                            1,
                            0,
                            0,
                            {40,
                             41,
                             42,
                             43,
                             44,
                             45,
                             46,
                             47,
                             48,
                             49,
                             50,
                             51,
                             52,
                             53,
                             54,
                             0x42,
                             0,
                             smbiosTableStorageSize,
                             smbiosTableVersion,
                             smbiosTableTimestamp,
                             MDR2SMBIOSStatusEnum::mdr2Init,
                             MDR2DirLockEnum::mdr2DirUnlock,
                             0,
                             smbiosSMMemoryOffset,
                             smbiosSMMemorySize,
                             smbiosTableStorageSize,
                             smbiosTableStorage}};
    std::unique_ptr<SharedMemoryArea> area;
    std::unique_ptr<phosphor::Timer> timer;

  private:
    uint8_t lockIndex = 0;
    uint8_t smbiosTableStorage[smbiosTableStorageSize];
};
