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

#include "system.hpp"

#include "mdrv2.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
namespace phosphor
{
namespace smbios
{

std::string System::uUID(std::string value)
{
    uint8_t* dataIn = storage;
    dataIn = getSMBIOSTypePtr(dataIn, systemType);
    if (dataIn != nullptr)
    {
        auto systemInfo = reinterpret_cast<struct SystemInfo*>(dataIn);
        std::stringstream stream;
        stream << std::setfill('0') << std::hex;
        stream << std::setw(8) << systemInfo->uUID.timeLow;
        stream << "-";
        stream << std::setw(4) << systemInfo->uUID.timeMid;
        stream << "-";
        stream << std::setw(4) << systemInfo->uUID.timeHiAndVer;
        stream << "-";
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.clockSeqHi);
        stream << std::setw(2)
               << static_cast<int>(systemInfo->uUID.clockSeqLow);
        stream << "-";
        static_assert(sizeof(systemInfo->uUID.node) == 6);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[0]);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[1]);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[2]);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[3]);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[4]);
        stream << std::setw(2) << static_cast<int>(systemInfo->uUID.node[5]);

        return sdbusplus::xyz::openbmc_project::Common::server::UUID::uUID(
            stream.str());
    }

    return sdbusplus::xyz::openbmc_project::Common::server::UUID::uUID(
        "00000000-0000-0000-0000-000000000000");
}

std::string System::version(std::string value)
{
    std::string result = "No BIOS Version";
    uint8_t* dataIn = storage;
    dataIn = getSMBIOSTypePtr(dataIn, biosType);
    if (dataIn != nullptr)
    {
        auto biosInfo = reinterpret_cast<struct BIOSInfo*>(dataIn);
        uint8_t biosVerByte = biosInfo->biosVersion;
        std::string tempS =
            positionToString(biosInfo->biosVersion, biosInfo->length, dataIn);
        result = tempS;
    }

    return sdbusplus::xyz::openbmc_project::Inventory::Decorator::server::
        Revision::version(result);
}

} // namespace smbios
} // namespace phosphor
