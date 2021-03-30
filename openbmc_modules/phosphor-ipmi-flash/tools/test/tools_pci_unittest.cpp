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

#include "internal_sys_mock.hpp"
#include "pci.hpp"
#include "pciaccess_mock.hpp"
#include "tool_errors.hpp"

#include <stdplus/raw.hpp>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace host_tool
{
namespace
{

using namespace std::string_literals;

using ::testing::Assign;
using ::testing::ContainerEq;
using ::testing::DoAll;
using ::testing::Each;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;

// TODO: switch to ConainerEq for C++20
MATCHER_P(SpanEq, s, "")
{
    return arg.size() == s.size() && !memcmp(arg.data(), s.data(), s.size());
}

MATCHER_P(PciIdMatch, m, "")
{
    return (arg->vendor_id == m->vendor_id && arg->device_id == m->device_id &&
            arg->subvendor_id == m->subvendor_id &&
            arg->subdevice_id == m->subdevice_id);
}

pci_device_iterator* mockIter = reinterpret_cast<pci_device_iterator*>(0x42);

constexpr pciaddr_t mockBaseAddr = 0xdeadbeef;
constexpr pciaddr_t mockRegionSize = 0x20000;

class Device
{
  public:
    virtual const struct pci_id_match* getMatch() const = 0;
    virtual struct pci_device getDevice() const = 0;
    virtual void expectSetup(PciAccessMock& pciMock,
                             const struct pci_device& dev) const {};
    virtual std::unique_ptr<PciBridgeIntf> getBridge(PciAccess* pci) const = 0;
    virtual std::string getName() const = 0;
};

class NuvotonDevice : public Device
{
  public:
    const struct pci_id_match* getMatch() const override
    {
        return &match;
    }

    struct pci_device getDevice() const override
    {
        struct pci_device dev;

        dev.vendor_id = match.vendor_id;
        dev.device_id = match.device_id;

        dev.regions[0].is_IO = false;
        dev.regions[0].base_addr = mockBaseAddr;
        dev.regions[0].size = mockRegionSize;

        return dev;
    }

    void expectSetup(PciAccessMock& pciMock,
                     const struct pci_device& dev) const override
    {
        static constexpr std::uint8_t defaultVal = 0x40;

        InSequence in;

        EXPECT_CALL(pciMock,
                    pci_device_cfg_read_u8(Eq(&dev), NotNull(), config))
            .WillOnce(DoAll(SetArgPointee<1>(defaultVal), Return(0)));
        EXPECT_CALL(pciMock, pci_device_cfg_write_u8(
                                 Eq(&dev), defaultVal | bridgeEnabled, config))
            .WillOnce(Return(0));

        EXPECT_CALL(pciMock,
                    pci_device_cfg_read_u8(Eq(&dev), NotNull(), config))
            .WillOnce(
                DoAll(SetArgPointee<1>(defaultVal | bridgeEnabled), Return(0)));
        EXPECT_CALL(pciMock,
                    pci_device_cfg_write_u8(Eq(&dev), defaultVal, config))
            .WillOnce(Return(0));
    }

    std::unique_ptr<PciBridgeIntf> getBridge(PciAccess* pci) const override
    {
        return std::make_unique<NuvotonPciBridge>(pci);
    }

    std::string getName() const override
    {
        return "Nuvoton"s;
    }

    /* Offset to the config register */
    static constexpr int config = 0x04;
    /* Second bit determines whether bridge is enabled */
    static constexpr std::uint8_t bridgeEnabled = 0x02;

  private:
    static constexpr struct pci_id_match match
    {
        0x1050, 0x0750, PCI_MATCH_ANY, PCI_MATCH_ANY
    };
};

class AspeedDevice : public Device
{
  public:
    const struct pci_id_match* getMatch() const override
    {
        return &match;
    }

    struct pci_device getDevice() const override
    {
        struct pci_device dev;

        dev.vendor_id = match.vendor_id;
        dev.device_id = match.device_id;

        dev.regions[1].is_IO = false;
        dev.regions[1].base_addr = mockBaseAddr;
        dev.regions[1].size = mockRegionSize;

        return dev;
    }

    std::unique_ptr<PciBridgeIntf> getBridge(PciAccess* pci) const override
    {
        return std::make_unique<AspeedPciBridge>(pci);
    }

    std::string getName() const override
    {
        return "Aspeed"s;
    }

    /* Offset to the config region */
    static constexpr int config = 0x0f000;
    /* Lower bit determines whether bridge is enabled */
    static constexpr std::uint8_t bridgeEnabled = 0x01;
    /* Offset to the MMIO address configuration */
    static constexpr int bridge = 0x0f004;

  private:
    static constexpr struct pci_id_match match
    {
        0x1a03, 0x2000, PCI_MATCH_ANY, PCI_MATCH_ANY
    };
};

NuvotonDevice nuvotonDevice;
AspeedDevice aspeedDevice;

class PciSetupTest : public testing::TestWithParam<Device*>
{};

/* Handle device not found */
TEST_P(PciSetupTest, NotFound)
{
    PciAccessMock pciMock;

    InSequence in;

    EXPECT_CALL(pciMock, pci_id_match_iterator_create(
                             PciIdMatch(GetParam()->getMatch())))
        .WillOnce(Return(mockIter));
    EXPECT_CALL(pciMock, pci_device_next(Eq(mockIter)))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(pciMock, pci_iterator_destroy(Eq(mockIter))).Times(1);

    EXPECT_THROW(GetParam()->getBridge(&pciMock), NotFoundException);
}

/* Test finding device but probe fails */
TEST_P(PciSetupTest, ProbeFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;

    EXPECT_CALL(pciMock, pci_id_match_iterator_create(
                             PciIdMatch(GetParam()->getMatch())))
        .WillOnce(Return(mockIter));
    EXPECT_CALL(pciMock, pci_device_next(Eq(mockIter)))
        .WillOnce(Return(&dev))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(pciMock, pci_device_probe(Eq(&dev))).WillOnce(Return(EFAULT));

    EXPECT_CALL(pciMock, pci_iterator_destroy(Eq(mockIter))).Times(1);

    EXPECT_THROW(GetParam()->getBridge(&pciMock), std::system_error);
}

/* Test finding device but mapping fails */
TEST_P(PciSetupTest, MapFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;

    EXPECT_CALL(pciMock, pci_id_match_iterator_create(
                             PciIdMatch(GetParam()->getMatch())))
        .WillOnce(Return(mockIter));
    EXPECT_CALL(pciMock, pci_device_next(Eq(mockIter)))
        .WillOnce(Return(&dev))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(pciMock, pci_device_probe(Eq(&dev)))
        .WillOnce(DoAll(Assign(&dev, GetParam()->getDevice()), Return(0)));

    EXPECT_CALL(pciMock,
                pci_device_map_range(Eq(&dev), mockBaseAddr, mockRegionSize,
                                     PCI_DEV_MAP_FLAG_WRITABLE, NotNull()))
        .WillOnce(Return(EFAULT));

    EXPECT_CALL(pciMock, pci_iterator_destroy(Eq(mockIter))).Times(1);

    EXPECT_THROW(GetParam()->getBridge(&pciMock), std::system_error);
}

/* Test finding device but unmapping fails */
TEST_P(PciSetupTest, UnmapFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    EXPECT_CALL(pciMock, pci_id_match_iterator_create(
                             PciIdMatch(GetParam()->getMatch())))
        .WillOnce(Return(mockIter));
    EXPECT_CALL(pciMock, pci_device_next(Eq(mockIter)))
        .WillOnce(Return(&dev))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(pciMock, pci_device_probe(Eq(&dev)))
        .WillOnce(DoAll(Assign(&dev, GetParam()->getDevice()), Return(0)));

    EXPECT_CALL(pciMock,
                pci_device_map_range(Eq(&dev), mockBaseAddr, mockRegionSize,
                                     PCI_DEV_MAP_FLAG_WRITABLE, NotNull()))
        .WillOnce(DoAll(SetArgPointee<4>(region.data()), Return(0)));

    EXPECT_CALL(pciMock, pci_iterator_destroy(Eq(mockIter))).Times(1);
    EXPECT_CALL(pciMock, pci_device_unmap_range(Eq(&dev), Eq(region.data()),
                                                mockRegionSize))
        .WillOnce(Return(EFAULT));

    GetParam()->expectSetup(pciMock, dev);
    // This will print an error but not throw
    GetParam()->getBridge(&pciMock);
}

/* Create expectations on pciMock for finding device and mapping memory region
 */
void expectSetup(PciAccessMock& pciMock, struct pci_device& dev, Device* param,
                 std::uint8_t* region, bool deviceExpectations = true)
{
    EXPECT_CALL(pciMock,
                pci_id_match_iterator_create(PciIdMatch(param->getMatch())))
        .WillOnce(Return(mockIter));
    EXPECT_CALL(pciMock, pci_device_next(Eq(mockIter)))
        .WillOnce(Return(&dev))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(pciMock, pci_device_probe(Eq(&dev)))
        .WillOnce(DoAll(Assign(&dev, param->getDevice()), Return(0)));

    EXPECT_CALL(pciMock,
                pci_device_map_range(Eq(&dev), mockBaseAddr, mockRegionSize,
                                     PCI_DEV_MAP_FLAG_WRITABLE, NotNull()))
        .WillOnce(DoAll(SetArgPointee<4>(region), Return(0)));

    EXPECT_CALL(pciMock, pci_iterator_destroy(Eq(mockIter))).Times(1);
    EXPECT_CALL(pciMock,
                pci_device_unmap_range(Eq(&dev), Eq(region), mockRegionSize))
        .WillOnce(Return(0));

    if (deviceExpectations)
        param->expectSetup(pciMock, dev);
}

/* Test finding device and mapping memory region */
TEST_P(PciSetupTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    expectSetup(pciMock, dev, GetParam(), region.data());

    GetParam()->getBridge(&pciMock);
}

INSTANTIATE_TEST_SUITE_P(Default, PciSetupTest,
                         ::testing::Values(&nuvotonDevice, &aspeedDevice),
                         [](const testing::TestParamInfo<Device*>& info) {
                             return info.param->getName();
                         });

TEST(NuvotonWriteTest, TooLarge)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    std::vector<std::uint8_t> data(0x4001);

    expectSetup(pciMock, dev, &nuvotonDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = nuvotonDevice.getBridge(&pciMock);
    EXPECT_THROW(bridge->write(stdplus::span<std::uint8_t>(data)),
                 ToolException);
}

TEST(NuvotonWriteTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    std::vector<std::uint8_t> data(0x4000);

    std::generate(data.begin(), data.end(), std::rand);

    expectSetup(pciMock, dev, &nuvotonDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = nuvotonDevice.getBridge(&pciMock);
    bridge->write(stdplus::span<std::uint8_t>(data));

    EXPECT_THAT(stdplus::span<uint8_t>(&region[0], data.size()),
                SpanEq(stdplus::span<uint8_t>(data)));
}

TEST(NuvotonConfigureTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    ipmi_flash::PciConfigResponse config{0x123bea51};

    expectSetup(pciMock, dev, &nuvotonDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = nuvotonDevice.getBridge(&pciMock);
    bridge->configure(config);

    /* No effect from calling configure(), so the whole region should be 0 */
    EXPECT_THAT(region, Each(0));
}

TEST(NuvotonDataLengthTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    expectSetup(pciMock, dev, &nuvotonDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = nuvotonDevice.getBridge(&pciMock);
    EXPECT_EQ(bridge->getDataLength(), 0x4000);
}

/* Make sure config register is left alone if the bridge is already enabled */
TEST(NuvotonBridgeTest, AlreadyEnabledSuccess)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x40;

    /* Only set standard expectations; not those from nuvotonDevice */
    expectSetup(pciMock, dev, &nuvotonDevice, region.data(), false);

    {
        InSequence in;

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(DoAll(
                SetArgPointee<1>(defaultVal | NuvotonDevice::bridgeEnabled),
                Return(0)));

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(DoAll(
                SetArgPointee<1>(defaultVal | NuvotonDevice::bridgeEnabled),
                Return(0)));
        EXPECT_CALL(pciMock, pci_device_cfg_write_u8(Eq(&dev), defaultVal,
                                                     NuvotonDevice::config))
            .WillOnce(Return(0));
    }

    nuvotonDevice.getBridge(&pciMock);
}

/* Read fails when attempting to setup the bridge */
TEST(NuvotonBridgeTest, ReadSetupFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    /* Only set standard expectations; not those from nuvotonDevice */
    expectSetup(pciMock, dev, &nuvotonDevice, region.data(), false);

    EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                NuvotonDevice::config))
        .WillOnce(Return(EFAULT));

    EXPECT_THROW(nuvotonDevice.getBridge(&pciMock), std::system_error);
}

/* Write fails when attempting to setup the bridge */
TEST(NuvotonBridgeTest, WriteSetupFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x40;

    /* Only set standard expectations; not those from nuvotonDevice */
    expectSetup(pciMock, dev, &nuvotonDevice, region.data(), false);

    EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                NuvotonDevice::config))
        .WillOnce(DoAll(SetArgPointee<1>(defaultVal), Return(0)));
    EXPECT_CALL(pciMock,
                pci_device_cfg_write_u8(
                    Eq(&dev), defaultVal | NuvotonDevice::bridgeEnabled,
                    NuvotonDevice::config))
        .WillOnce(Return(EFAULT));

    EXPECT_THROW(nuvotonDevice.getBridge(&pciMock), std::system_error);
}

/* Read fails when attempting to disable the bridge */
TEST(NuvotonBridgeTest, ReadDisableFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x40;

    /* Only set standard expectations; not those from nuvotonDevice */
    expectSetup(pciMock, dev, &nuvotonDevice, region.data(), false);

    {
        InSequence in;

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(DoAll(SetArgPointee<1>(defaultVal), Return(0)));
        EXPECT_CALL(pciMock,
                    pci_device_cfg_write_u8(
                        Eq(&dev), defaultVal | NuvotonDevice::bridgeEnabled,
                        NuvotonDevice::config))
            .WillOnce(Return(0));

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(Return(EFAULT));
    }

    nuvotonDevice.getBridge(&pciMock);
}

/* Write fails when attempting to disable the bridge */
TEST(NuvotonBridgeTest, WriteDisableFail)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x40;

    /* Only set standard expectations; not those from nuvotonDevice */
    expectSetup(pciMock, dev, &nuvotonDevice, region.data(), false);

    {
        InSequence in;

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(DoAll(SetArgPointee<1>(defaultVal), Return(0)));
        EXPECT_CALL(pciMock,
                    pci_device_cfg_write_u8(
                        Eq(&dev), defaultVal | NuvotonDevice::bridgeEnabled,
                        NuvotonDevice::config))
            .WillOnce(Return(0));

        EXPECT_CALL(pciMock, pci_device_cfg_read_u8(Eq(&dev), NotNull(),
                                                    NuvotonDevice::config))
            .WillOnce(DoAll(
                SetArgPointee<1>(defaultVal | NuvotonDevice::bridgeEnabled),
                Return(0)));
        EXPECT_CALL(pciMock, pci_device_cfg_write_u8(Eq(&dev), defaultVal,
                                                     NuvotonDevice::config))
            .WillOnce(Return(EFAULT));
    }

    nuvotonDevice.getBridge(&pciMock);
}

/* Make sure the bridge gets enabled when needed */
TEST(NuvotonBridgeTest, NotEnabledSuccess)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    expectSetup(pciMock, dev, &nuvotonDevice, region.data());
    nuvotonDevice.getBridge(&pciMock);
}

TEST(AspeedWriteTest, TooLarge)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    std::vector<std::uint8_t> data(0x10001);

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);
    EXPECT_THROW(bridge->write(stdplus::span<std::uint8_t>(data)),
                 ToolException);
}

TEST(AspeedWriteTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    std::vector<std::uint8_t> data(0x10000);

    std::generate(data.begin(), data.end(), std::rand);

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);
    bridge->write(stdplus::span<std::uint8_t>(data));

    EXPECT_THAT(stdplus::span<uint8_t>(&region[0x10000], data.size()),
                SpanEq(stdplus::span<uint8_t>(data)));
}

TEST(AspeedConfigureTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);
    ipmi_flash::PciConfigResponse config{0x123bea51};

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);
    bridge->configure(config);

    auto configSpan = stdplus::raw::asSpan<uint8_t>(config);
    EXPECT_THAT(
        stdplus::span<uint8_t>(&region[AspeedDevice::bridge], sizeof(config)),
        SpanEq(configSpan));
}

TEST(AspeedDataLengthTest, Success)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);
    EXPECT_EQ(bridge->getDataLength(), 0x10000);
}

/* Make sure config region is left alone if the bridge is already enabled */
TEST(AspeedBridgeTest, AlreadyEnabledSuccess)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x42;

    region[AspeedDevice::config] = defaultVal | AspeedDevice::bridgeEnabled;

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);

    {
        std::vector<std::uint8_t> enabledRegion(mockRegionSize);
        enabledRegion[AspeedDevice::config] =
            defaultVal | AspeedDevice::bridgeEnabled;
        EXPECT_THAT(region, ContainerEq(enabledRegion));
    }

    bridge.reset();

    {
        std::vector<std::uint8_t> disabledRegion(mockRegionSize);
        disabledRegion[AspeedDevice::config] = defaultVal;
        EXPECT_THAT(region, ContainerEq(disabledRegion));
    }
}

/* Make sure the bridge gets enabled when needed */
TEST(AspeedBridgeTest, NotEnabledSuccess)
{
    PciAccessMock pciMock;
    struct pci_device dev;
    std::vector<std::uint8_t> region(mockRegionSize);

    constexpr std::uint8_t defaultVal = 0x42;

    region[AspeedDevice::config] = defaultVal;

    expectSetup(pciMock, dev, &aspeedDevice, region.data());

    std::unique_ptr<PciBridgeIntf> bridge = aspeedDevice.getBridge(&pciMock);

    {
        std::vector<std::uint8_t> enabledRegion(mockRegionSize);
        enabledRegion[AspeedDevice::config] =
            defaultVal | AspeedDevice::bridgeEnabled;
        EXPECT_THAT(region, ContainerEq(enabledRegion));
    }

    bridge.reset();

    {
        std::vector<std::uint8_t> disabledRegion(mockRegionSize);
        disabledRegion[AspeedDevice::config] = defaultVal;
        EXPECT_THAT(region, ContainerEq(disabledRegion));
    }
}

} // namespace
} // namespace host_tool
