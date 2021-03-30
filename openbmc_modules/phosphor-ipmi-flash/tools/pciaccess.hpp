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

#pragma once

extern "C"
{
#include <pciaccess.h>
} // extern "C"

#include <cstdint>

namespace host_tool
{

/**
 * @class PciAccess
 * @brief Overridable interface to libpciacess for unit testing
 */
class PciAccess
{
  public:
    virtual struct pci_device_iterator* pci_id_match_iterator_create(
        const struct pci_id_match* match) const = 0;
    virtual void
        pci_iterator_destroy(struct pci_device_iterator* iter) const = 0;
    virtual struct pci_device*
        pci_device_next(struct pci_device_iterator* iter) const = 0;
    virtual int pci_device_probe(struct pci_device* dev) const = 0;
    virtual int pci_device_cfg_read_u8(struct pci_device* dev,
                                       std::uint8_t* data,
                                       pciaddr_t offset) const = 0;
    virtual int pci_device_cfg_write_u8(struct pci_device* dev,
                                        std::uint8_t data,
                                        pciaddr_t offset) const = 0;
    virtual int pci_device_map_range(struct pci_device* dev, pciaddr_t base,
                                     pciaddr_t size, unsigned map_flags,
                                     void** addr) const = 0;
    virtual int pci_device_unmap_range(struct pci_device* dev, void* memory,
                                       pciaddr_t size) const = 0;

    virtual ~PciAccess() = default;
};

/**
 * @class PciAccessImpl
 * @brief libpciaccess concrete implementation
 * @details Passes through all calls to the underlying library.
 */
class PciAccessImpl : public PciAccess
{
  public:
    struct pci_device_iterator* pci_id_match_iterator_create(
        const struct pci_id_match* match) const override;
    void pci_iterator_destroy(struct pci_device_iterator* iter) const override;
    struct pci_device*
        pci_device_next(struct pci_device_iterator* iter) const override;
    int pci_device_probe(struct pci_device* dev) const override;
    int pci_device_cfg_read_u8(struct pci_device* dev, std::uint8_t* data,
                               pciaddr_t offset) const override;
    int pci_device_cfg_write_u8(struct pci_device* dev, std::uint8_t data,
                                pciaddr_t offset) const override;
    int pci_device_map_range(struct pci_device* dev, pciaddr_t base,
                             pciaddr_t size, unsigned map_flags,
                             void** addr) const override;
    int pci_device_unmap_range(struct pci_device* dev, void* memory,
                               pciaddr_t size) const override;

    static PciAccessImpl& getInstance()
    {
        static PciAccessImpl instance;
        return instance;
    }

  private:
    PciAccessImpl();
    virtual ~PciAccessImpl();
};

} // namespace host_tool
