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

#include <xyz/openbmc_project/Common/UUID/server.hpp>
#include <xyz/openbmc_project/Inventory/Decorator/Revision/server.hpp>

namespace phosphor
{

namespace smbios
{

class System : sdbusplus::server::object::object<
                   sdbusplus::xyz::openbmc_project::Common::server::UUID>,
               sdbusplus::server::object::object<
                   sdbusplus::xyz::openbmc_project::Inventory::Decorator::
                       server::Revision>
{
  public:
    System() = delete;
    ~System() = default;
    System(const System&) = delete;
    System& operator=(const System&) = delete;
    System(System&&) = default;
    System& operator=(System&&) = default;

    System(sdbusplus::bus::bus& bus, const std::string& objPath,
           uint8_t* smbiosTableStorage) :
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Common::server::UUID>(
            bus, objPath.c_str()),
        sdbusplus::server::object::object<
            sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
                Revision>(bus, objPath.c_str()),
        path(objPath), storage(smbiosTableStorage)
    {
        std::string input = "0";
        uUID(input);
        version("0.00");
    }

    std::string uUID(std::string value) override;

    std::string version(std::string value) override;

  private:
    /** @brief Path of the group instance */
    std::string path;

    uint8_t* storage;

    struct BIOSInfo
    {
        uint8_t type;
        uint8_t length;
        uint16_t handle;
        uint8_t vendor;
        uint8_t biosVersion;
        uint16_t startAddrSegment;
        uint8_t releaseData;
        uint8_t romSize;
        uint64_t characteristics;
        uint16_t externCharacteristics;
        uint8_t systemBIOSMajor;
        uint8_t systemBIOSMinor;
        uint8_t embeddedFirmwareMajor;
        uint8_t embeddedFirmwareMinor;
    } __attribute__((packed));

    struct UUID
    {
        uint32_t timeLow;
        uint16_t timeMid;
        uint16_t timeHiAndVer;
        uint8_t clockSeqHi;
        uint8_t clockSeqLow;
        uint8_t node[6];
    } __attribute__((packed));

    struct SystemInfo
    {
        uint8_t type;
        uint8_t length;
        uint16_t handle;
        uint8_t manufacturer;
        uint8_t productName;
        uint8_t version;
        uint8_t serialNum;
        struct UUID uUID;
        uint8_t wakeupType;
        uint8_t skuNum;
        uint8_t family;
    } __attribute__((packed));
};

} // namespace smbios

} // namespace phosphor
