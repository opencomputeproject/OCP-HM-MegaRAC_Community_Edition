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
#include "cpu.hpp"
#include "dimm.hpp"
#include "smbios.hpp"
#include "system.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>
#include <smbios.hpp>
#include <xyz/openbmc_project/Smbios/MDR_V2/server.hpp>

static constexpr int limitEntryLen = 0xff;
static constexpr uint8_t mdr2Version = 2;
static constexpr uint32_t mdr2SMSize = 0x00100000;
static constexpr uint32_t mdr2SMBaseAddress = 0x9FF00000;
static constexpr const char* mdrType2File = "/var/lib/smbios/smbios2";
static constexpr const char* mdrV2Path = "/xyz/openbmc_project/Smbios/MDR_V2";

enum class MDR2SMBIOSStatusEnum
{
    mdr2Init = 0,
    mdr2Loaded = 1,
    mdr2Updated = 2,
    mdr2Updating = 3
};

enum class MDR2DirLockEnum
{
    mdr2DirUnlock = 0,
    mdr2DirLock = 1
};

typedef struct
{
    uint8_t dataInfo[16];
} DataIdStruct;

typedef struct
{
    DataIdStruct id;
    uint32_t size;
    uint32_t dataSetSize;
    uint32_t dataVersion;
    uint32_t timestamp;
} Mdr2DirEntry;

typedef struct
{
    Mdr2DirEntry common;
    MDR2SMBIOSStatusEnum stage;
    MDR2DirLockEnum lock;
    uint16_t lockHandle;
    uint32_t xferBuff;
    uint32_t xferSize;
    uint32_t maxDataSize;
    uint8_t* dataStorage;
} Mdr2DirLocalStruct;

typedef struct
{
    uint8_t agentVersion;
    uint8_t dirVersion;
    uint8_t dirEntries;
    uint8_t status; // valid / locked / etc
    uint8_t remoteDirVersion;
    uint16_t sessionHandle;
    Mdr2DirLocalStruct dir[maxDirEntries];
} Mdr2DirStruct;

struct MDRSMBIOSHeader
{
    uint8_t dirVer;
    uint8_t mdrType;
    uint32_t timestamp;
    uint32_t dataSize;
} __attribute__((packed));

namespace phosphor
{
namespace smbios
{

class MDR_V2 : sdbusplus::server::object::object<
                   sdbusplus::xyz::openbmc_project::Smbios::server::MDR_V2>
{
  public:
    MDR_V2() = delete;
    MDR_V2(const MDR_V2&) = delete;
    MDR_V2& operator=(const MDR_V2&) = delete;
    MDR_V2(MDR_V2&&) = delete;
    MDR_V2& operator=(MDR_V2&&) = delete;
    ~MDR_V2() = default;

    MDR_V2(sdbusplus::bus::bus& bus, const char* path, sd_event* event) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Smbios::server::MDR_V2>(bus, path),
        bus(bus), timer(event, [&](void) { agentSynchronizeData(); })
    {

        smbiosDir.agentVersion = smbiosAgentVersion;
        smbiosDir.dirVersion = 1;
        smbiosDir.dirEntries = 1;
        directoryEntries(smbiosDir.dirEntries);
        smbiosDir.status = 1;
        smbiosDir.remoteDirVersion = 0;

        std::copy(smbiosTableId.begin(), smbiosTableId.end(),
                  smbiosDir.dir[smbiosDirIndex].common.id.dataInfo);

        smbiosDir.dir[smbiosDirIndex].dataStorage = smbiosTableStorage;

        agentSynchronizeData();
    }

    std::vector<uint8_t> getDirectoryInformation(uint8_t dirIndex) override;

    std::vector<uint8_t> getDataInformation(uint8_t idIndex) override;

    bool sendDirectoryInformation(uint8_t dirVersion, uint8_t dirIndex,
                                  uint8_t returnedEntries,
                                  uint8_t remainingEntries,
                                  std::vector<uint8_t> dirEntry) override;

    std::vector<uint8_t> getDataOffer() override;

    bool sendDataInformation(uint8_t idIndex, uint8_t flag, uint32_t dataLen,
                             uint32_t dataVer, uint32_t timeStamp) override;

    int findIdIndex(std::vector<uint8_t> dataInfo) override;

    bool agentSynchronizeData() override;

    std::vector<uint32_t>
        synchronizeDirectoryCommonData(uint8_t idIndex, uint32_t size) override;

    uint8_t directoryEntries(uint8_t value) override;

  private:
    Timer timer;

    sdbusplus::bus::bus& bus;

    Mdr2DirStruct smbiosDir;

    bool readDataFromFlash(MDRSMBIOSHeader* mdrHdr, uint8_t* data);

    const std::array<uint8_t, 16> smbiosTableId{
        40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 0x42};
    uint8_t smbiosTableStorage[smbiosTableStorageSize];

    bool smbiosIsUpdating(uint8_t index);
    bool smbiosIsAvailForUpdate(uint8_t index);
    inline uint8_t smbiosValidFlag(uint8_t index);
    void systemInfoUpdate(void);

    int getTotalCpuSlot(void);
    int getTotalDimmSlot(void);
    std::vector<std::unique_ptr<Cpu>> cpus;
    std::vector<std::unique_ptr<Dimm>> dimms;
    std::unique_ptr<System> system;
};

} // namespace smbios
} // namespace phosphor
