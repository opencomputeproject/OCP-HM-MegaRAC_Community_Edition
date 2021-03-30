/*
 * Copyright 2019 Google Inc.
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

#include "p2a.hpp"

#include "data.hpp"
#include "flags.hpp"
#include "pci.hpp"
#include "tool_errors.hpp"

#include <fmt/format.h>

#include <ipmiblob/blob_errors.hpp>
#include <stdplus/handle/managed.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace host_tool
{

namespace
{

/** @brief RAII wrapper and its destructor for opening a file descriptor */
static void closeFd(int&& fd, const internal::Sys* const& sys)
{
    sys->close(fd);
}
using Fd = stdplus::Managed<int, const internal::Sys* const>::Handle<closeFd>;

} // namespace

bool P2aDataHandler::sendContents(const std::string& input,
                                  std::uint16_t session)
{
    std::unique_ptr<PciBridgeIntf> bridge;
    ipmi_flash::PciConfigResponse pciResp;
    std::int64_t fileSize;

    try
    {
        bridge = std::make_unique<NuvotonPciBridge>(pci);
    }
    catch (NotFoundException& e)
    {}

    try
    {
        bridge = std::make_unique<AspeedPciBridge>(pci);
    }
    catch (NotFoundException& e)
    {}

    if (!bridge)
    {
        throw NotFoundException("supported PCI device");
    }

    /* Read the configuration via blobs metadata (stat). */
    ipmiblob::StatResponse stat = blob->getStat(session);
    if (stat.metadata.size() != sizeof(ipmi_flash::PciConfigResponse))
    {
        throw ToolException("Didn't receive expected size of metadata for "
                            "PCI Configuration response");
    }

    std::memcpy(&pciResp, stat.metadata.data(), sizeof(pciResp));
    bridge->configure(pciResp);

    /* For data blocks in 64kb, stage data, and send blob write command. */
    Fd inputFd(sys->open(input.c_str(), 0), sys);
    if (*inputFd < 0)
    {
        (void)inputFd.release();
        throw internal::errnoException(
            fmt::format("Error opening file '{}'", input));
    }

    fileSize = sys->getSize(input.c_str());
    if (fileSize == 0)
    {
        throw ToolException("Zero-length file, or other file access error");
    }

    progress->start(fileSize);

    std::vector<std::uint8_t> readBuffer(bridge->getDataLength());

    int bytesRead = 0;
    std::uint32_t offset = 0;

    do
    {
        bytesRead = sys->read(*inputFd, readBuffer.data(), readBuffer.size());
        if (bytesRead > 0)
        {
            bridge->write(stdplus::span<const std::uint8_t>(readBuffer.data(),
                                                            bytesRead));

            /* Ok, so the data is staged, now send the blob write with the
             * details.
             */
            struct ipmi_flash::ExtChunkHdr chunk;
            chunk.length = bytesRead;
            std::vector<std::uint8_t> chunkBytes(sizeof(chunk));
            std::memcpy(chunkBytes.data(), &chunk, sizeof(chunk));

            /* This doesn't return anything on success. */
            blob->writeBytes(session, offset, chunkBytes);
            offset += bytesRead;
            progress->updateProgress(bytesRead);
        }
    } while (bytesRead > 0);

    return true;
}

} // namespace host_tool
