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

#include "config.h"

#include "buildjson.hpp"
#include "file_handler.hpp"
#include "firmware_handler.hpp"
#include "flags.hpp"
#include "general_systemd.hpp"
#include "image_handler.hpp"
#include "lpc_aspeed.hpp"
#include "lpc_handler.hpp"
#include "lpc_nuvoton.hpp"
#include "net_handler.hpp"
#include "pci_handler.hpp"
#include "status.hpp"
#include "util.hpp"

#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ipmi_flash
{

namespace
{

#ifdef NUVOTON_P2A_MBOX
static constexpr std::size_t memoryRegionSize = 16 * 1024UL;
#elif defined NUVOTON_P2A_VGA
static constexpr std::size_t memoryRegionSize = 4 * 1024 * 1024UL;
#else
/* The maximum external buffer size we expect is 64KB. */
static constexpr std::size_t memoryRegionSize = 64 * 1024UL;
#endif

static constexpr const char* jsonConfigurationPath =
    "/usr/share/phosphor-ipmi-flash/";

#ifdef ENABLE_LPC_BRIDGE
#if defined(ASPEED_LPC)
LpcDataHandler lpcDataHandler(
    LpcMapperAspeed::createAspeedMapper(MAPPED_ADDRESS, memoryRegionSize));
#elif defined(NUVOTON_LPC)
LpcDataHandler lpcDataHandler(
    LpcMapperNuvoton::createNuvotonMapper(MAPPED_ADDRESS, memoryRegionSize));
#else
#error "You must specify a hardware implementation."
#endif
#endif

#ifdef ENABLE_PCI_BRIDGE
PciDataHandler pciDataHandler(MAPPED_ADDRESS, memoryRegionSize);
#endif

#ifdef ENABLE_NET_BRIDGE
NetDataHandler netDataHandler;
#endif

std::vector<DataHandlerPack> supportedTransports = {
    {FirmwareFlags::UpdateFlags::ipmi, nullptr},
#ifdef ENABLE_PCI_BRIDGE
    {FirmwareFlags::UpdateFlags::p2a, &pciDataHandler},
#endif
#ifdef ENABLE_LPC_BRIDGE
    {FirmwareFlags::UpdateFlags::lpc, &lpcDataHandler},
#endif
#ifdef ENABLE_NET_BRIDGE
    {FirmwareFlags::UpdateFlags::net, &netDataHandler},
#endif
};

/**
 * Given a name and path, create a HandlerPack.
 *
 * @param[in] name - the blob id path for this
 * @param[in] path - the file path to write the contents.
 * @return the HandlerPack.
 */
HandlerPack CreateFileHandlerPack(const std::string& name,
                                  const std::string& path)
{
    return HandlerPack(name, std::make_unique<FileHandler>(path));
}

} // namespace
} // namespace ipmi_flash

extern "C"
{
    std::unique_ptr<blobs::GenericBlobInterface> createHandler();
}

std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    ipmi_flash::ActionMap actionPacks = {};

    std::vector<ipmi_flash::HandlerConfig> configsFromJson =
        ipmi_flash::BuildHandlerConfigs(ipmi_flash::jsonConfigurationPath);

    std::vector<ipmi_flash::HandlerPack> supportedFirmware;

    supportedFirmware.push_back(std::move(ipmi_flash::CreateFileHandlerPack(
        ipmi_flash::hashBlobId, HASH_FILENAME)));

    for (auto& config : configsFromJson)
    {
        supportedFirmware.push_back(std::move(
            ipmi_flash::HandlerPack(config.blobId, std::move(config.handler))));
        actionPacks[config.blobId] = std::move(config.actions);

        std::fprintf(stderr, "config loaded: %s\n", config.blobId.c_str());
    }

    auto handler = ipmi_flash::FirmwareBlobHandler::CreateFirmwareBlobHandler(
        std::move(supportedFirmware), ipmi_flash::supportedTransports,
        std::move(actionPacks));

    if (!handler)
    {
        std::fprintf(stderr, "Firmware Handler has an invalid configuration");
        return nullptr;
    }

    return handler;
}
