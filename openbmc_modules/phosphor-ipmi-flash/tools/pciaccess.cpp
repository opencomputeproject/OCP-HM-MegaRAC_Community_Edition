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

#include "pciaccess.hpp"

#include <cstdio>
#include <cstring>
#include <system_error>

namespace host_tool
{

struct pci_device_iterator* PciAccessImpl::pci_id_match_iterator_create(
    const struct pci_id_match* match) const
{
    return ::pci_id_match_iterator_create(match);
}

void PciAccessImpl::pci_iterator_destroy(struct pci_device_iterator* iter) const
{
    return ::pci_iterator_destroy(iter);
}

struct pci_device*
    PciAccessImpl::pci_device_next(struct pci_device_iterator* iter) const
{
    return ::pci_device_next(iter);
}

int PciAccessImpl::pci_device_probe(struct pci_device* dev) const
{
    return ::pci_device_probe(dev);
}

int PciAccessImpl::pci_device_cfg_read_u8(struct pci_device* dev,
                                          std::uint8_t* data,
                                          pciaddr_t offset) const
{
    return ::pci_device_cfg_read_u8(dev, data, offset);
}

int PciAccessImpl::pci_device_cfg_write_u8(struct pci_device* dev,
                                           std::uint8_t data,
                                           pciaddr_t offset) const
{
    return ::pci_device_cfg_write_u8(dev, data, offset);
}

int PciAccessImpl::pci_device_map_range(struct pci_device* dev, pciaddr_t base,
                                        pciaddr_t size, unsigned map_flags,
                                        void** addr) const
{
    return ::pci_device_map_range(dev, base, size, map_flags, addr);
}

int PciAccessImpl::pci_device_unmap_range(struct pci_device* dev, void* memory,
                                          pciaddr_t size) const
{
    return ::pci_device_unmap_range(dev, memory, size);
}

PciAccessImpl::PciAccessImpl()
{
    int ret = ::pci_system_init();
    if (ret)
    {
        throw std::system_error(ret, std::generic_category(),
                                "Error setting up libpciaccess");
    }
}

PciAccessImpl::~PciAccessImpl()
{
    ::pci_system_cleanup();
}

} // namespace host_tool
