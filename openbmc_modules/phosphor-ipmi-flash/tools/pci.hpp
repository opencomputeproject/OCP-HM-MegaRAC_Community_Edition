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

#include "data.hpp"
#include "internal/sys.hpp"
#include "pciaccess.hpp"

#include <linux/pci_regs.h>

#include <stdplus/types.hpp>

// Some versions of the linux/pci_regs.h header don't define this
#ifndef PCI_STD_NUM_BARS
#define PCI_STD_NUM_BARS 6
#endif // !PCI_STD_NUM_BARS

namespace host_tool
{

class PciBridgeIntf
{
  public:
    virtual ~PciBridgeIntf() = default;

    virtual void write(const stdplus::span<const std::uint8_t> data) = 0;
    virtual void configure(const ipmi_flash::PciConfigResponse& config) = 0;

    virtual std::size_t getDataLength() = 0;
};

class PciAccessBridge : public PciBridgeIntf
{
  public:
    virtual ~PciAccessBridge();

    virtual void write(const stdplus::span<const std::uint8_t> data) override;
    virtual void
        configure(const ipmi_flash::PciConfigResponse& configResp) override{};

    std::size_t getDataLength() override
    {
        return dataLength;
    }

  protected:
    /**
     * Finds the PCI device matching @a match and saves a reference to it in @a
     * dev. Also maps the memory region described in BAR number @a bar to
     * address @a addr,
     */
    PciAccessBridge(const struct pci_id_match* match, int bar,
                    std::size_t dataOffset, std::size_t dataLength,
                    const PciAccess* pci);

    struct pci_device* dev = nullptr;
    std::uint8_t* addr = nullptr;
    std::size_t size = 0;

  private:
    std::size_t dataOffset;
    std::size_t dataLength;

  protected:
    const PciAccess* pci;
};

class NuvotonPciBridge : public PciAccessBridge
{
  public:
    explicit NuvotonPciBridge(const PciAccess* pci) :
        PciAccessBridge(&match, bar, dataOffset, dataLength, pci)
    {
        enableBridge();
    }

    ~NuvotonPciBridge()
    {
        disableBridge();
    }

  private:
    static constexpr std::uint32_t vid = 0x1050;
    static constexpr std::uint32_t did = 0x0750;
    static constexpr int bar = 0;
    static constexpr struct pci_id_match match
    {
        vid, did, PCI_MATCH_ANY, PCI_MATCH_ANY
    };

    static constexpr pciaddr_t bridge = 0x04;
    static constexpr std::uint8_t bridgeEnabled = 0x02;

    static constexpr std::size_t dataOffset = 0x0;
    static constexpr std::size_t dataLength = 0x4000;

    void enableBridge();
    void disableBridge();
};

class AspeedPciBridge : public PciAccessBridge
{
  public:
    explicit AspeedPciBridge(const PciAccess* pci) :
        PciAccessBridge(&match, bar, dataOffset, dataLength, pci)
    {
        enableBridge();
    }

    ~AspeedPciBridge()
    {
        disableBridge();
    }

    void configure(const ipmi_flash::PciConfigResponse& configResp) override;

  private:
    static constexpr std::uint32_t vid = 0x1a03;
    static constexpr std::uint32_t did = 0x2000;
    static constexpr int bar = 1;
    static constexpr struct pci_id_match match
    {
        vid, did, PCI_MATCH_ANY, PCI_MATCH_ANY
    };

    static constexpr std::size_t config = 0x0f000;
    static constexpr std::size_t bridge = 0x0f004;
    static constexpr std::uint32_t bridgeEnabled = 0x1;

    static constexpr std::size_t dataOffset = 0x10000;
    static constexpr std::size_t dataLength = 0x10000;

    void enableBridge();
    void disableBridge();
};

} // namespace host_tool
