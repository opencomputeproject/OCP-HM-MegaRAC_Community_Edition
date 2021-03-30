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

#include "net.hpp"

#include "data.hpp"
#include "flags.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ipmiblob/blob_errors.hpp>
#include <stdplus/handle/managed.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace
{

void closefd(int&& fd, const internal::Sys*& sys)
{
    sys->close(fd);
}
using Fd = stdplus::Managed<int, const internal::Sys*>::Handle<closefd>;

} // namespace

namespace host_tool
{

bool NetDataHandler::sendContents(const std::string& input,
                                  std::uint16_t session)
{
    constexpr size_t blockSize = 64 * 1024;
    Fd inputFd(std::nullopt, sys);

    {
        inputFd.reset(sys->open(input.c_str(), O_RDONLY));
        if (*inputFd < 0)
        {
            (void)inputFd.release();
            std::fprintf(stderr, "Unable to open file: '%s'\n", input.c_str());
            return false;
        }

        std::int64_t fileSize = sys->getSize(input.c_str());
        if (fileSize == 0)
        {
            std::fprintf(stderr,
                         "Zero-length file, or other file access error\n");
            return false;
        }

        progress->start(fileSize);
    }

    Fd connFd(std::nullopt, sys);

    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo *addrs, *addr;
        int ret = sys->getaddrinfo(host.c_str(), port.c_str(), &hints, &addrs);
        if (ret < 0)
        {
            std::fprintf(stderr, "Couldn't parse address %s with port %s: %s\n",
                         host.c_str(), port.c_str(), gai_strerror(ret));
            return false;
        }

        for (addr = addrs; addr != nullptr; addr = addr->ai_next)
        {
            connFd.reset(sys->socket(addr->ai_family, addr->ai_socktype,
                                     addr->ai_protocol));
            if (*connFd == -1)
                continue;

            if (sys->connect(*connFd, addr->ai_addr, addr->ai_addrlen) != -1)
                break;
        }

        // TODO: use stdplus Managed for the addrinfo structs
        sys->freeaddrinfo(addrs);

        if (addr == nullptr)
        {
            std::fprintf(stderr, "Failed to connect\n");
            return false;
        }
    }

    try
    {
        int bytesSent = 0;
        off_t offset = 0;

        do
        {
            bytesSent = sys->sendfile(*connFd, *inputFd, &offset, blockSize);
            if (bytesSent < 0)
            {
                std::fprintf(stderr, "Failed to send data to BMC: %s\n",
                             strerror(errno));
                return false;
            }
            else if (bytesSent > 0)
            {
                /* Ok, so the data is staged, now send the blob write with
                 * the details.
                 */
                struct ipmi_flash::ExtChunkHdr chunk;
                chunk.length = bytesSent;
                std::vector<std::uint8_t> chunkBytes(sizeof(chunk));
                std::memcpy(chunkBytes.data(), &chunk, sizeof(chunk));

                /* This doesn't return anything on success. */
                blob->writeBytes(session, offset - bytesSent, chunkBytes);
                progress->updateProgress(bytesSent);
            }
        } while (bytesSent > 0);
    }
    catch (const ipmiblob::BlobException& b)
    {
        return false;
    }

    return true;
}

} // namespace host_tool
