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

#include "lpc.hpp"

#include "data.hpp"

#include <ipmiblob/blob_errors.hpp>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace host_tool
{

bool LpcDataHandler::sendContents(const std::string& input,
                                  std::uint16_t session)
{
    LpcRegion host_lpc_buf;
    host_lpc_buf.address = address;
    host_lpc_buf.length = length;

    std::vector<std::uint8_t> payload(sizeof(host_lpc_buf));

    while (true)
    {
        /* If the writeMeta() is rejected we need to call sessionStat on it. */
        try
        {
            std::fprintf(stderr, "sending writeMeta\n");

            std::memcpy(payload.data(), &host_lpc_buf, sizeof(host_lpc_buf));
            blob->writeMeta(session, 0x00, payload);

            std::fprintf(stderr, "writemeta sent\n");

            break;
        }
        catch (...)
        {
            std::fprintf(stderr, "caught exception\n");

            ipmiblob::StatResponse resp = blob->getStat(session);
            if (resp.metadata.empty())
            {
                std::fprintf(stderr, "Received no metadata bytes back!");
                return false;
            }

            struct MemoryMapResultDetails
            {
                std::uint8_t code;
                std::uint32_t offset;
                std::uint32_t length;
            } __attribute__((packed));

            struct MemoryMapResultDetails bytes;

            if (resp.metadata.size() != sizeof(bytes))
            {
                std::fprintf(
                    stderr,
                    "Received insufficient bytes back on expected return!\n");
                return false;
            }

            std::memcpy(&bytes, resp.metadata.data(), sizeof(bytes));

            if (bytes.code == EFBIG)
            {
                std::fprintf(stderr, "EFBIG returned!\n");

                host_lpc_buf.length = bytes.length;
                host_lpc_buf.address += bytes.offset;
            }
            else if (bytes.code == 0)
            {
                /* We're good, continue! */
                break;
            }
        }
    }

    /* For data blockss, stage data, and send blob write command. */
    int inputFd = sys->open(input.c_str(), 0);
    if (inputFd < 0)
    {
        return false;
    }

    std::int64_t fileSize = sys->getSize(input.c_str());
    if (fileSize == 0)
    {
        std::fprintf(stderr, "Zero-length file, or other file access error\n");
        return false;
    }

    progress->start(fileSize);

    /* For Nuvoton the maximum is 4K */
    auto readBuffer = std::make_unique<std::uint8_t[]>(host_lpc_buf.length);
    if (nullptr == readBuffer)
    {
        sys->close(inputFd);
        std::fprintf(stderr, "Unable to allocate memory for read buffer.\n");
        return false;
    }

    /* TODO: This is similar to PCI insomuch as how it sends data, so combine.
     */
    try
    {
        int bytesRead = 0;
        std::uint32_t offset = 0;

        do
        {
            bytesRead =
                sys->read(inputFd, readBuffer.get(), host_lpc_buf.length);
            if (bytesRead > 0)
            {
                if (!io->write(host_lpc_buf.address, bytesRead,
                               readBuffer.get()))
                {
                    std::fprintf(stderr,
                                 "Failed to write to region in memory!\n");
                }

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
    }
    catch (const ipmiblob::BlobException& b)
    {
        sys->close(inputFd);
        return false;
    }

    sys->close(inputFd);
    return true;
}

} // namespace host_tool
