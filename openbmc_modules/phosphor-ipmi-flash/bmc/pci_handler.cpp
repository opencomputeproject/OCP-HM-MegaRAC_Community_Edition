/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pci_handler.hpp"

#include "data.hpp"

#include <fcntl.h>
#include <linux/aspeed-p2a-ctrl.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ipmi_flash
{

const std::string PciDataHandler::p2aControlPath = "/dev/aspeed-p2a-ctrl";

bool PciDataHandler::open()
{
    mappedFd = sys->open(p2aControlPath.c_str(), O_RDWR);
    if (mappedFd == -1)
    {
        return false;
    }

    struct aspeed_p2a_ctrl_mapping map;
    map.addr = regionAddress;
    map.length = memoryRegionSize;
    map.flags = ASPEED_P2A_CTRL_READWRITE;

    if (sys->ioctl(mappedFd, ASPEED_P2A_CTRL_IOCTL_SET_WINDOW, &map))
    {
        sys->close(mappedFd);
        mappedFd = -1;

        return false;
    }

    if (sys->ioctl(mappedFd, ASPEED_P2A_CTRL_IOCTL_GET_MEMORY_CONFIG, &map))
    {
        sys->close(mappedFd);
        mappedFd = -1;

        return false;
    }

    /* The length of the region reserved is reported, and it's important
     * because the offset + memory region could be beyond that reserved
     * region.
     */
    std::uint64_t offset = regionAddress - map.addr;

    mapped = reinterpret_cast<std::uint8_t*>(
        mmap(0, memoryRegionSize, PROT_READ, MAP_SHARED, mappedFd, offset));
    if (mapped == MAP_FAILED)
    {
        sys->close(mappedFd);
        mappedFd = -1;
        mapped = nullptr;

        return false;
    }

    return true;
}

bool PciDataHandler::close()
{
    /* TODO: Turn off the P2A bridge and region to disable host-side access.
     */
    if (mapped)
    {
        sys->munmap(mapped, memoryRegionSize);
        mapped = nullptr;
    }

    if (mappedFd != -1)
    {
        sys->close(mappedFd);
        mappedFd = -1;
    }

    return true;
}

std::vector<std::uint8_t> PciDataHandler::copyFrom(std::uint32_t length)
{
    std::vector<std::uint8_t> results(length);
    std::memcpy(results.data(), mapped, length);

    return results;
}

bool PciDataHandler::writeMeta(const std::vector<std::uint8_t>& configuration)
{
    /* PCI handler doesn't require configuration write, only read. */
    return false;
}

std::vector<std::uint8_t> PciDataHandler::readMeta()
{
    /* PCI handler does require returning a configuration from read. */
    struct PciConfigResponse reply;
    reply.address = regionAddress;

    std::vector<std::uint8_t> bytes;
    bytes.resize(sizeof(reply));
    std::memcpy(bytes.data(), &reply, sizeof(reply));

    return bytes;
}

} // namespace ipmi_flash
