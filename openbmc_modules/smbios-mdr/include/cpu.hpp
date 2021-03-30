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
#include "smbios.hpp"

#include <xyz/openbmc_project/Inventory/Decorator/Asset/server.hpp>
#include <xyz/openbmc_project/Inventory/Item/Cpu/server.hpp>
#include <xyz/openbmc_project/Inventory/Item/server.hpp>
#include <xyz/openbmc_project/State/Decorator/OperationalStatus/server.hpp>

namespace phosphor
{

namespace smbios
{

// Definition follow smbios spec DSP0134 3.0.0
static const std::map<uint8_t, const char*> processorTypeTable = {
    {0x1, "Other"},          {0x2, "Unknown"},       {0x3, "Central Processor"},
    {0x4, "Math Processor"}, {0x5, "DSP Processor"}, {0x6, "Video Processor"},
};

// Definition follow smbios spec DSP0134 3.0.0
static const std::map<uint8_t, const char*> familyTable = {
    {0x1, "Other"},
    {0x2, "Unknown"},
    {0x10, "Pentium II Xeon processor"},
    {0xa1, "Quad-Core Intel Xeon processor 3200 Series"},
    {0xa2, "Dual-Core Intel Xeon processor 3000 Series"},
    {0xa3, "Quad-Core Intel Xeon processor 5300 Series"},
    {0xa4, "Dual-Core Intel Xeon processor 5100 Series"},
    {0xa5, "Dual-Core Intel Xeon processor 5000 Series"},
    {0xa6, "Dual-Core Intel Xeon processor LV"},
    {0xa7, "Dual-Core Intel Xeon processor ULV"},
    {0xa8, "Dual-Core Intel Xeon processor 7100 Series"},
    {0xa9, "Quad-Core Intel Xeon processor 5400 Series"},
    {0xaa, "Quad-Core Intel Xeon processor"},
    {0xab, "Dual-Core Intel Xeon processor 5200 Series"},
    {0xac, "Dual-Core Intel Xeon processor 7200 Series"},
    {0xad, "Quad-Core Intel Xeon processor 7300 Series"},
    {0xae, "Quad-Core Intel Xeon processor 7400 Series"},
    {0xaf, "Multi-Core Intel Xeon processor 7400 Series"},
    {0xb0, "Pentium III Xeon processor"},
    {0xb3, "Intel Xeon processor"},
    {0xb5, "Intel Xeon processor MP"},
    {0xd6, "Multi-Core Intel Xeon processor"},
    {0xd7, "Dual-Core Intel Xeon processor 3xxx Series"},
    {0xd8, "Quad-Core Intel Xeon processor 3xxx Series"},
    {0xd9, "VIA Nano Processor Family"},
    {0xda, "Dual-Core Intel Xeon processor 5xxx Series"},
    {0xdb, "Quad-Core Intel Xeon processor 5xxx Series"},
    {0xdd, "Dual-Core Intel Xeon processor 7xxx Series"},
    {0xde, "Quad-Core Intel Xeon processor 7xxx Series"},
    {0xdf, "Multi-Core Intel Xeon processor 7xxx Series"},
    {0xe0, "Multi-Core Intel Xeon processor 3400 Series"}

};

// Definition follow smbios spec DSP0134 3.0.0
static const std::array<std::string, 16> characteristicsTable{
    "Reserved",
    "Unknown",
    "64-bit Capable",
    "Multi-Core",
    "Hardware Thread",
    "Execute Protection",
    "Enhanced Virtualization",
    "Power/Performance Control",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};

class Cpu
    : sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu>,
      sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::Inventory::server::Item>,
      sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::Asset>,
      sdbusplus::server::object::object<
          sdbusplus::xyz::openbmc_project::State::Decorator::server::
              OperationalStatus>
{
  public:
    Cpu() = delete;
    Cpu(const Cpu&) = delete;
    Cpu& operator=(const Cpu&) = delete;
    Cpu(Cpu&&) = delete;
    Cpu& operator=(Cpu&&) = delete;
    ~Cpu() = default;

    Cpu(sdbusplus::bus::bus& bus, const std::string& objPath,
        const uint8_t& cpuId, uint8_t* smbiosTableStorage) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Inventory::Item::server::Cpu>(
            bus, objPath.c_str()),
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
                Asset>(bus, objPath.c_str()),
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Inventory::server::Item>(
            bus, objPath.c_str()),
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::State::Decorator::server::
                OperationalStatus>(bus, objPath.c_str()),
        cpuNum(cpuId), storage(smbiosTableStorage)
    {
        processorInfoUpdate();
    }

    void processorInfoUpdate(void);

    std::string processorSocket(std::string value) override;
    std::string processorType(std::string value) override;
    std::string processorFamily(std::string value) override;
    std::string manufacturer(std::string value) override;
    uint32_t processorId(uint32_t value) override;
    std::string processorVersion(std::string value) override;
    uint16_t processorMaxSpeed(uint16_t value) override;
    std::string processorCharacteristics(std::string value) override;
    uint16_t processorCoreCount(uint16_t value) override;
    uint16_t processorThreadCount(uint16_t value) override;
    bool present(bool value) override;
    bool functional(bool value) override;

  private:
    uint8_t cpuNum;

    uint8_t* storage;

    struct ProcessorInfo
    {
        uint8_t type;
        uint8_t length;
        uint16_t handle;
        uint8_t socketDesignation;
        uint8_t processorType;
        uint8_t family;
        uint8_t manufacturer;
        uint64_t id;
        uint8_t version;
        uint8_t voltage;
        uint16_t exClock;
        uint16_t maxSpeed;
        uint16_t currSpeed;
        uint8_t status;
        uint8_t upgrade;
        uint16_t l1Handle;
        uint16_t l2Handle;
        uint16_t l3Handle;
        uint8_t serialNum;
        uint8_t assetTag;
        uint8_t partNum;
        uint8_t coreCount;
        uint8_t coreEnable;
        uint8_t threadCount;
        uint16_t characteristics;
        uint16_t family2;
        uint16_t coreCount2;
        uint16_t coreEnable2;
        uint16_t threadCount2;
    } __attribute__((packed));

    void cpuSocket(const uint8_t positionNum, const uint8_t structLen,
                   uint8_t* dataIn);
    void cpuType(const uint8_t value);
    void cpuFamily(const uint8_t value);
    void cpuManufacturer(const uint8_t positionNum, const uint8_t structLen,
                         uint8_t* dataIn);
    void cpuVersion(const uint8_t positionNum, const uint8_t structLen,
                    uint8_t* dataIn);
    void cpuCharacteristics(const uint16_t value);
    void cpuStatus(const uint8_t value);
};

} // namespace smbios

} // namespace phosphor
