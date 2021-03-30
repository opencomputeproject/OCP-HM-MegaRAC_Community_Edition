/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once
#include <ipmid/api.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server/interface.hpp>

/**
 * @brief Response queue defines
 */
constexpr int responseQueueMaxSize = 20;

/**
 * @brief Ipmb misc
 */
constexpr uint8_t ipmbLunMask = 0x03;
constexpr uint8_t ipmbSeqMask = 0x3F;
constexpr uint8_t ipmbMeSlaveAddress = 0x2C;
constexpr uint8_t ipmbMeChannelNum = 1;

/**
 * @brief Ipmb getters
 */
constexpr uint8_t ipmbNetFnGet(uint8_t netFnLun)
{
    return netFnLun >> 2;
}

constexpr uint8_t ipmbLunFromNetFnLunGet(uint8_t netFnLun)
{
    return netFnLun & ipmbLunMask;
}

constexpr uint8_t ipmbSeqGet(uint8_t seqNumLun)
{
    return seqNumLun >> 2;
}

constexpr uint8_t ipmbLunFromSeqLunGet(uint8_t seqNumLun)
{
    return seqNumLun & ipmbLunMask;
}

/**
 * @brief Ipmb setters
 */
constexpr uint8_t ipmbNetFnLunSet(uint8_t netFn, uint8_t lun)
{
    return ((netFn << 2) | (lun & ipmbLunMask));
}

constexpr uint8_t ipmbSeqLunSet(uint8_t seq, uint8_t lun)
{
    return ((seq << 2) | (lun & ipmbLunMask));
}

constexpr size_t ipmbMaxDataSize = 256;
constexpr size_t ipmbConnectionHeaderLength = 3;
constexpr size_t ipmbResponseDataHeaderLength = 4;
constexpr size_t ipmbRequestDataHeaderLength = 3;
constexpr size_t ipmbChecksum2StartOffset = 3;
constexpr size_t ipmbChecksumSize = 1;
constexpr size_t ipmbMinFrameLength = 7;
constexpr size_t ipmbMaxFrameLength = ipmbConnectionHeaderLength +
                                      ipmbResponseDataHeaderLength +
                                      ipmbChecksumSize + ipmbMaxDataSize;

/**
 * @brief Channel types
 */
constexpr uint8_t targetChannelIpmb = 0x1;
constexpr uint8_t targetChannelIcmb10 = 0x2;
constexpr uint8_t targetChannelIcmb09 = 0x3;
constexpr uint8_t targetChannelLan = 0x4;
constexpr uint8_t targetChannelSerialModem = 0x5;
constexpr uint8_t targetChannelOtherLan = 0x6;
constexpr uint8_t targetChannelPciSmbus = 0x7;
constexpr uint8_t targetChannelSmbus10 = 0x8;
constexpr uint8_t targetChannelSmbus20 = 0x9;
constexpr uint8_t targetChannelSystemInterface = 0xC;

/**
 * @brief Channel modes
 */
constexpr uint8_t modeNoTracking = 0x0;
constexpr uint8_t modeTrackRequest = 0x1;
constexpr uint8_t modeSendRaw = 0x2;

/**
 * @brief Ipmb frame
 */
typedef struct
{
    /// @brief IPMB frame header
    union
    {
        /// @brief IPMB request header
        struct
        {
            /** @brief IPMB Connection Header Format */
            uint8_t address;
            uint8_t rsNetFnLUN;
            uint8_t checksum1;
            /** @brief IPMB Header */
            uint8_t rqSA;
            uint8_t rqSeqLUN;
            uint8_t cmd;
            uint8_t data[];
        } Req;
        /// @brief IPMB response header
        struct
        {
            uint8_t address;
            /** @brief IPMB Connection Header Format */
            uint8_t rqNetFnLUN;
            uint8_t checksum1;
            /** @brief IPMB Header */
            uint8_t rsSA;
            uint8_t rsSeqLUN;
            uint8_t cmd;
            uint8_t completionCode;
            uint8_t data[];
        } Resp;
    } Header;
} __attribute__((packed)) ipmbHeader;

/**
 * @brief Ipmb messages
 */
struct IpmbRequest
{
    uint8_t address;
    uint8_t netFn;
    uint8_t rsLun;
    uint8_t rqSA;
    uint8_t seq;
    uint8_t rqLun;
    uint8_t cmd;
    std::vector<uint8_t> data;

    IpmbRequest(const ipmbHeader* ipmbBuffer, size_t bufferLength);

    void prepareRequest(sdbusplus::message::message& mesg);
};

struct IpmbResponse
{
    uint8_t address;
    uint8_t netFn;
    uint8_t rqLun;
    uint8_t rsSA;
    uint8_t seq;
    uint8_t rsLun;
    uint8_t cmd;
    uint8_t completionCode;
    std::vector<uint8_t> data;

    IpmbResponse(uint8_t address, uint8_t netFn, uint8_t rqLun, uint8_t rsSA,
                 uint8_t seq, uint8_t rsLun, uint8_t cmd,
                 uint8_t completionCode, std::vector<uint8_t>& inputData);

    void ipmbToi2cConstruct(uint8_t* buffer, size_t* bufferLength);
};

/**
 * @brief Get Message Flags Response
 */
constexpr uint8_t getMsgFlagReceiveMessageBit = 0;
constexpr uint8_t getMsgFlagEventMessageBit = 1;
constexpr uint8_t getMsgFlagWatchdogPreTimeOutBit = 3;
constexpr uint8_t getMsgFlagOEM0Bit = 5;
constexpr uint8_t getMsgFlagOEM1Bit = 6;
constexpr uint8_t getMsgFlagOEM2Bit = 7;

/** @class Bridging
 *
 *  @brief Implement commands to support IPMI bridging.
 */
class Bridging
{
  public:
    Bridging() = default;
    std::size_t getResponseQueueSize();

    void clearResponseQueue();

    ipmi::Cc handleIpmbChannel(ipmi::Context::ptr ctx, const uint8_t tracking,
                               const std::vector<uint8_t>& msgData,
                               std::vector<uint8_t>& rspData);

    void insertMessageInQueue(IpmbResponse msg);

    IpmbResponse getMessageFromQueue();

    void eraseMessageFromQueue();

  private:
    std::vector<IpmbResponse> responseQueue;
};
