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

#include "ipmi_handler.hpp"

#include "ipmi_errors.hpp"

#include <fcntl.h>
#include <linux/ipmi.h>
#include <linux/ipmi_msgdefs.h>
#include <sys/ioctl.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ipmiblob
{

std::unique_ptr<IpmiInterface> IpmiHandler::CreateIpmiHandler()
{
    return std::make_unique<IpmiHandler>();
}

void IpmiHandler::open()
{
    const int device = 0;
    const std::vector<std::string> formats = {"/dev/ipmi", "/dev/ipmi/",
                                              "/dev/ipmidev/"};

    for (const auto& format : formats)
    {
        std::ostringstream path;
        path << format << device;

        fd = sys->open(path.str().c_str(), O_RDWR);
        if (fd < 0)
        {
            continue;
        }
        break;
    }

    if (fd < 0)
    {
        throw IpmiException("Unable to open any ipmi devices");
    }
}

std::vector<std::uint8_t>
    IpmiHandler::sendPacket(std::uint8_t netfn, std::uint8_t cmd,
                            std::vector<std::uint8_t>& data)
{
    if (fd < 0)
    {
        open();
    }

    constexpr int ipmiOEMLun = 0;
    constexpr int fifteenMs = 15 * 1000;
    constexpr int ipmiReadTimeout = fifteenMs;
    constexpr int ipmiResponseBufferLen = IPMI_MAX_MSG_LENGTH;
    constexpr int ipmiOk = 0;

    /* We have a handle to the IPMI device. */
    std::array<std::uint8_t, ipmiResponseBufferLen> responseBuffer = {};

    /* Build address. */
    ipmi_system_interface_addr systemAddress{};
    systemAddress.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
    systemAddress.channel = IPMI_BMC_CHANNEL;
    systemAddress.lun = ipmiOEMLun;

    /* Build request. */
    ipmi_req request{};
    request.addr = reinterpret_cast<unsigned char*>(&systemAddress);
    request.addr_len = sizeof(systemAddress);
    request.msgid = sequence++;
    request.msg.data = reinterpret_cast<unsigned char*>(data.data());
    request.msg.data_len = data.size();
    request.msg.netfn = netfn;
    request.msg.cmd = cmd;

    ipmi_recv reply{};
    reply.addr = reinterpret_cast<unsigned char*>(&systemAddress);
    reply.addr_len = sizeof(systemAddress);
    reply.msg.data = reinterpret_cast<unsigned char*>(responseBuffer.data());
    reply.msg.data_len = responseBuffer.size();

    /* Try to send request. */
    int rc = sys->ioctl(fd, IPMICTL_SEND_COMMAND, &request);
    if (rc < 0)
    {
        throw IpmiException("Unable to send IPMI request.");
    }

    /* Could use sdeventplus, but for only one type of event is it worth it? */
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;

    do
    {
        rc = sys->poll(&pfd, 1, ipmiReadTimeout);
        if (rc < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            throw IpmiException("Error occurred.");
        }
        else if (rc == 0)
        {
            throw IpmiException("Timeout waiting for reply.");
        }

        /* Yay, happy case! */
        rc = sys->ioctl(fd, IPMICTL_RECEIVE_MSG_TRUNC, &reply);
        if (rc < 0)
        {
            throw IpmiException("Unable to read reply.");
        }

        if (request.msgid != reply.msgid)
        {
            std::fprintf(stderr, "Received wrong message, trying again.\n");
        }
    } while (request.msgid != reply.msgid);

    if (responseBuffer[0] != ipmiOk)
    {
        throw IpmiException(static_cast<int>(responseBuffer[0]));
    }

    std::vector<std::uint8_t> returning;
    auto dataLen = reply.msg.data_len - 1;

    returning.insert(returning.begin(), responseBuffer.begin() + 1,
                     responseBuffer.begin() + dataLen + 1);

    return returning;
}

} // namespace ipmiblob
