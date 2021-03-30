/*
 * Copyright 2020 Google Inc.
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

#include "pci.hpp"

#include "tool_errors.hpp"

extern "C"
{
#include <pciaccess.h>
} // extern "C"

#include <fmt/format.h>

#include <stdplus/handle/managed.hpp>

#include <cstring>
#include <system_error>

namespace host_tool
{

namespace
{

/** @brief RAII wrapper and its destructor for creating a pci_device_iterator */
static void closeIt(struct pci_device_iterator*&& it,
                    const PciAccess* const& pci)
{
    pci->pci_iterator_destroy(it);
}
using It = stdplus::Managed<struct pci_device_iterator*,
                            const PciAccess* const>::Handle<closeIt>;

} // namespace

PciAccessBridge::PciAccessBridge(const struct pci_id_match* match, int bar,
                                 std::size_t dataOffset, std::size_t dataLength,
                                 const PciAccess* pci) :
    dataOffset(dataOffset),
    dataLength(dataLength), pci(pci)
{
    It it(pci->pci_id_match_iterator_create(match), pci);

    while ((dev = pci->pci_device_next(*it)))
    {
        int ret = pci->pci_device_probe(dev);
        if (ret)
        {
            throw std::system_error(ret, std::generic_category(),
                                    "Error probing PCI device");
        }

        /* Verify it's a memory-based bar. */
        if (!dev->regions[bar].is_IO)
            break;
    }

    if (!dev)
    {
        throw NotFoundException(fmt::format(
            "PCI device {:#04x}:{:#04x}", match->vendor_id, match->device_id));
    }

    std::fprintf(stderr, "Find [0x%x 0x%x] \n", match->vendor_id,
                 match->device_id);
    std::fprintf(stderr, "bar%d[0x%x] \n", bar,
                 static_cast<unsigned int>(dev->regions[bar].base_addr));

    size = dev->regions[bar].size;
    int ret = pci->pci_device_map_range(
        dev, dev->regions[bar].base_addr, dev->regions[bar].size,
        PCI_DEV_MAP_FLAG_WRITABLE, reinterpret_cast<void**>(&addr));
    if (ret)
    {
        throw std::system_error(ret, std::generic_category(),
                                "Error mapping PCI device memory");
    }
}

PciAccessBridge::~PciAccessBridge()
{
    int ret = pci->pci_device_unmap_range(dev, addr, size);

    if (ret)
    {
        std::fprintf(stderr, "Error while unmapping PCI device memory: %s\n",
                     std::strerror(ret));
    }
}

void PciAccessBridge::write(const stdplus::span<const std::uint8_t> data)
{
    if (data.size() > dataLength)
    {
        throw ToolException(
            fmt::format("Write of {} bytes exceeds maximum of {}", data.size(),
                        dataLength));
    }

    std::memcpy(addr + dataOffset, data.data(), data.size());
}

void NuvotonPciBridge::enableBridge()
{
    std::uint8_t value;
    int ret;

    ret = pci->pci_device_cfg_read_u8(dev, &value, bridge);
    if (ret)
    {
        throw std::system_error(ret, std::generic_category(),
                                "Error reading bridge status");
    }

    if (value & bridgeEnabled)
    {
        std::fprintf(stderr, "Bridge already enabled\n");
        return;
    }

    value |= bridgeEnabled;

    ret = pci->pci_device_cfg_write_u8(dev, value, bridge);
    if (ret)
    {
        throw std::system_error(ret, std::generic_category(),
                                "Error enabling bridge");
    }
}

void NuvotonPciBridge::disableBridge()
{
    std::uint8_t value;
    int ret;

    ret = pci->pci_device_cfg_read_u8(dev, &value, bridge);
    if (ret)
    {
        std::fprintf(stderr, "Error reading bridge status: %s\n",
                     std::strerror(ret));
        return;
    }
    value &= ~bridgeEnabled;

    ret = pci->pci_device_cfg_write_u8(dev, value, bridge);
    if (ret)
    {
        std::fprintf(stderr, "Error disabling bridge: %s\n",
                     std::strerror(ret));
    }
}

void AspeedPciBridge::enableBridge()
{
    /* We sent the open command before this, so the window should be open and
     * the bridge enabled on the BMC.
     */
    std::uint32_t value;
    std::memcpy(&value, addr + config, sizeof(value));

    if (0 == (value & bridgeEnabled))
    {
        std::fprintf(stderr, "Bridge not enabled - Enabling from host\n");

        value |= bridgeEnabled;
        std::memcpy(addr + config, &value, sizeof(value));
    }

    std::fprintf(stderr, "The bridge is enabled!\n");
}

void AspeedPciBridge::disableBridge()
{
    /* addr is valid if the constructor completed */

    /* Read current value, and just blindly unset the bit. */
    std::uint32_t value;
    std::memcpy(&value, addr + config, sizeof(value));

    value &= ~bridgeEnabled;
    std::memcpy(addr + config, &value, sizeof(value));
}

void AspeedPciBridge::configure(const ipmi_flash::PciConfigResponse& configResp)
{
    std::fprintf(stderr, "Received address: 0x%x\n", configResp.address);

    /* Configure the mmio to point there. */
    std::memcpy(addr + bridge, &configResp.address, sizeof(configResp.address));
}

} // namespace host_tool
