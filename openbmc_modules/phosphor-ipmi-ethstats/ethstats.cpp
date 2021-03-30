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

#include "ethstats.hpp"

#include "handler.hpp"

#include <ipmid/api.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>

namespace ethstats
{
// If this changes in the future, there should be some alternative
// source for the information if possible to provide continuined functionality.
static const std::map<std::uint8_t, std::string> statLookup = {
    {RX_BYTES, "rx_bytes"},
    {RX_COMPRESSED, "rx_compressed"},
    {RX_CRC_ERRORS, "rx_crc_errors"},
    {RX_DROPPED, "rx_dropped"},
    {RX_ERRORS, "rx_errors"},
    {RX_FIFO_ERRORS, "rx_fifo_errors"},
    {RX_FRAME_ERRORS, "rx_frame_errors"},
    {RX_LENGTH_ERRORS, "rx_length_errors"},
    {RX_MISSED_ERRORS, "rx_missed_errors"},
    {RX_NOHANDLER, "rx_nohandler"},
    {RX_OVER_ERRORS, "rx_over_errors"},
    {RX_PACKETS, "rx_packets"},
    {TX_ABORTED_ERRORS, "tx_aborted_errors"},
    {TX_BYTES, "tx_bytes"},
    {TX_CARRIER_ERRORS, "tx_carrier_errors"},
    {TX_COMPRESSED, "tx_compressed"},
    {TX_DROPPED, "tx_dropped"},
    {TX_ERRORS, "tx_errors"},
    {TX_FIFO_ERRORS, "tx_fifo_errors"},
    {TX_HEARTBEAT_ERRORS, "tx_heartbeat_errors"},
    {TX_PACKETS, "tx_packets"},
    {TX_WINDOW_ERRORS, "tx_window_errors"},
};

std::string buildPath(const std::string& ifName, const std::string& field)
{
    std::ostringstream opath;
    opath << "/sys/class/net/" << ifName << "/statistics/" << field;

    return opath.str();
}

ipmi_ret_t handleEthStatCommand(const std::uint8_t* reqBuf,
                                std::uint8_t* replyCmdBuf, size_t* dataLen,
                                const EthStatsInterface* handler)
{
    auto reqLength = (*dataLen);

    // Verify the reqBuf is the minimum length.
    // [0] == statistics id
    // [1] == if_name_length
    // [2..N] == if_name
    // In theory the smallest can be a one-letter name. (3 bytes).
    if (reqLength < sizeof(struct EthStatRequest) + sizeof(std::uint8_t))
    {
        std::fprintf(stderr, "*dataLen too small: %u\n",
                     static_cast<std::uint32_t>(reqLength));
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    // using struct prefix due to nature as c-style pod struct.
    struct EthStatRequest request;
    std::memcpy(&request, &reqBuf[0], sizeof(request));
    auto nameLen = static_cast<std::uint32_t>(request.if_name_len);

    if (reqLength < (sizeof(request) + nameLen))
    {
        std::fprintf(stderr, "*dataLen too small: %u\n",
                     static_cast<std::uint32_t>(reqLength));
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    // Check the statistic to see if we recognize it.
    auto stat = statLookup.find(request.statId);
    if (stat == statLookup.end())
    {
        std::fprintf(stderr, "stat not known: 0x%x\n", request.statId);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    // The if_name handling plus a few other things was taken from the
    // CableCheck command implementation.
    //
    // Ok, so we know what statistic they want.  Let's validate their
    // if_name.  The string is length delimited (like dns).

    // Copy the string out of the request buffer.
    // Maximum length is 256 bytes, excluding the nul-terminator.
    auto name = std::string(
        reinterpret_cast<const char*>(&reqBuf[0] + sizeof(request)), nameLen);

    // Minor sanity & security check (of course, I'm less certain if unicode
    // comes into play here.
    //
    // Basically you can't easily inject ../ or /../ into the path below.
    // Decided to make this more robust, although since it appends to the path
    // it would limit any exposure.
    if (name.find("/") != std::string::npos)
    {
        std::fprintf(stderr, "Invalid or illegal name: '%s'\n", name.c_str());
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    std::string fullPath = buildPath(name, stat->second);

    if (!handler->validIfNameAndField(fullPath))
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    struct EthStatReply reply;
    reply.statId = request.statId;
    reply.value = handler->readStatistic(fullPath);

    // Store the result.
    std::memcpy(&replyCmdBuf[0], &reply, sizeof(reply));
    (*dataLen) = sizeof(reply);

    return IPMI_CC_OK;
}

} // namespace ethstats
