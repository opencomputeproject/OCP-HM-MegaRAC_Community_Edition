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

#include "net_handler.hpp"

#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>

namespace ipmi_flash
{

bool NetDataHandler::open()
{
    listenFd.reset(::socket(AF_INET6, SOCK_STREAM, 0));
    if (*listenFd < 0)
    {
        std::perror("Failed to create socket");
        (void)listenFd.release();
        return false;
    }

    struct sockaddr_in6 listenAddr;
    listenAddr.sin6_family = AF_INET6;
    listenAddr.sin6_port = htons(listenPort);
    listenAddr.sin6_flowinfo = 0;
    listenAddr.sin6_addr = in6addr_any;
    listenAddr.sin6_scope_id = 0;

    if (::bind(*listenFd, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) <
        0)
    {
        std::perror("Failed to bind");
        return false;
    }

    if (::listen(*listenFd, 1) < 0)
    {
        std::perror("Failed to listen");
        return false;
    }
    return true;
}

bool NetDataHandler::close()
{
    connFd.reset();
    listenFd.reset();

    return true;
}

std::vector<std::uint8_t> NetDataHandler::copyFrom(std::uint32_t length)
{
    if (!connFd)
    {
        struct pollfd fds;
        fds.fd = *listenFd;
        fds.events = POLLIN;

        int ret = ::poll(&fds, 1, timeoutS * 1000);
        if (ret < 0)
        {
            std::perror("Failed to poll");
            return std::vector<uint8_t>();
        }
        else if (ret == 0)
        {
            fprintf(stderr, "Timed out waiting for connection\n");
            return std::vector<uint8_t>();
        }
        else if (fds.revents != POLLIN)
        {
            fprintf(stderr, "Invalid poll state: 0x%x\n", fds.revents);
            return std::vector<uint8_t>();
        }

        connFd.reset(::accept(*listenFd, nullptr, nullptr));
        if (*connFd < 0)
        {
            std::perror("Failed to accept connection");
            (void)connFd.release();
            return std::vector<uint8_t>();
        }

        struct timeval tv = {};
        tv.tv_sec = timeoutS;
        if (setsockopt(*connFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            std::perror("Failed to set receive timeout");
            return std::vector<uint8_t>();
        }
    }

    std::vector<std::uint8_t> data(length);

    std::uint32_t bytesRead = 0;
    ssize_t ret;
    do
    {
        ret = read(*connFd, data.data() + bytesRead, length - bytesRead);
        if (ret < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            std::perror("Failed to read from socket");
            break;
        }

        bytesRead += ret;
    } while (ret > 0 && bytesRead < length);

    if (bytesRead != length)
    {
        fprintf(stderr,
                "Couldn't read full expected amount. Wanted %u but got %u\n",
                length, bytesRead);
        data.resize(bytesRead);
    }

    return data;
}

bool NetDataHandler::writeMeta(const std::vector<std::uint8_t>& configuration)
{
    // TODO: have the host tool send the expected IP address that it will
    // connect from
    return true;
}

std::vector<std::uint8_t> NetDataHandler::readMeta()
{
    return std::vector<std::uint8_t>();
}

} // namespace ipmi_flash
