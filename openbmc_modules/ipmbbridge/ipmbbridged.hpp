/* Copyright 2018 Intel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "ipmbdefines.hpp"

#include <boost/container/flat_set.hpp>
#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message.hpp>
#include <vector>

extern "C" {
#include <i2c/smbus.h>
#include <linux/i2c-dev.h>
}

#ifndef IPMBBRIDGED_HPP
#define IPMBBRIDGED_HPP

/**
 * @brief Ipmb return status codes (sendRequest API call)
 */
enum class ipmbResponseStatus
{
    success = 0,
    error = 1,
    invalid_param = 2,
    busy = 3,
    timeout = 4,
};

/**
 * @brief Ipmb outstanding requests defines
 */
constexpr int ipmbMaxOutstandingRequestsCount = 64;
constexpr int ipmbNumberOfTries = 6;
constexpr uint64_t ipmbRequestRetryTimeout = 250; // ms

/**
 * @brief Ipmb I2C communication
 */
constexpr uint8_t ipmbI2cNumberOfRetries = 2;

/**
 * @brief Ipmb boardcast address
 */
constexpr uint8_t broadcastAddress = 0x0;

/**
 * @brief Ipmb defines
 */
constexpr size_t ipmbMaxDataSize = 256;
constexpr size_t ipmbConnectionHeaderLength = 3;
constexpr size_t ipmbResponseDataHeaderLength = 4;
constexpr size_t ipmbRequestDataHeaderLength = 3;
constexpr size_t ipmbAddressSize = 1;
constexpr size_t ipmbPktLenSize = 1;
constexpr size_t ipmbChecksumSize = 1;
constexpr size_t ipmbChecksum2StartOffset = 3;
constexpr size_t ipmbMinFrameLength = 7;
constexpr size_t ipmbMaxFrameLength =
    ipmbPktLenSize + ipmbConnectionHeaderLength + ipmbResponseDataHeaderLength +
    ipmbChecksumSize + ipmbMaxDataSize;

/**
 * @brief Ipmb misc
 */
constexpr uint8_t ipmbNetFnResponseMask = 0x01;
constexpr uint8_t ipmbLunMask = 0x03;
constexpr uint8_t ipmbSeqMask = 0x3F;
constexpr uint8_t ipmbRsLun = 0x0;

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

constexpr uint8_t ipmbAddressTo7BitSet(uint8_t address)
{
    return address >> 1;
}

constexpr uint8_t ipmbRespNetFn(uint8_t netFn)
{
    return netFn |= 1;
}

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
 * @brief Ipmb checkers
 */
constexpr bool ipmbIsResponse(IPMB_HEADER *ipmbHeader)
{
    return ipmbNetFnGet(ipmbHeader->Header.Resp.rqNetFnLUN) &
           ipmbNetFnResponseMask;
}

/**
 * @brief Ipmb request state
 */
enum class ipmbRequestState
{
    invalid,
    valid,
    matched,
};

/**
 * @brief Channel types
 */
enum class ipmbChannelType
{
    ipmb = 0,
    me = 1
};

/**
 * @brief IpmbResponse declaration
 */
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

    IpmbResponse();

    IpmbResponse(uint8_t address, uint8_t netFn, uint8_t rqLun, uint8_t rsSA,
                 uint8_t seq, uint8_t rsLun, uint8_t cmd,
                 uint8_t completionCode, const std::vector<uint8_t> &inputData);

    void i2cToIpmbConstruct(IPMB_HEADER *ipmbBuffer, size_t bufferLength);

    std::shared_ptr<std::vector<uint8_t>> ipmbToi2cConstruct();
};

/**
 * @brief IpmbRequest declaration
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

    size_t dataLength;
    ipmbRequestState state;
    std::optional<boost::asio::steady_timer> timer;
    std::unique_ptr<IpmbResponse> matchedResponse;

    // creates empty request with empty timer object
    IpmbRequest();

    IpmbRequest(uint8_t address, uint8_t netFn, uint8_t rsLun, uint8_t rqSA,
                uint8_t seq, uint8_t rqLun, uint8_t cmd,
                const std::vector<uint8_t> &inputData);

    IpmbRequest(const IpmbRequest &) = delete;
    IpmbRequest &operator=(IpmbRequest const &) = delete;

    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
        returnMatchedResponse();

    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
        returnStatusResponse(int status);

    void i2cToIpmbConstruct(IPMB_HEADER *ipmbBuffer, size_t bufferLength);

    int ipmbToi2cConstruct(std::vector<uint8_t> &buffer);
};

/**
 * @brief Command filtering class declaration
 *
 * This feature provides simple mechanism for filtering out commands - which are
 * not implemented in IPMI - on IPMB level, in order to reduce DBus traffic
 */
class IpmbCommandFilter
{
  public:
    // function checking if netFn & cmd combination exist in blocked command
    // list
    bool isBlocked(const uint8_t reqNetFn, const uint8_t cmd);
    // function adding netfFn & cmd combination to the blocked command list
    void addFilter(const uint8_t reqNetFn, const uint8_t cmd);

  private:
    boost::container::flat_set<std::pair<uint8_t, uint8_t>> unhandledCommands;
};

/**
 * @brief Command filtering defines
 */

constexpr uint8_t ipmbIpmiInvalidCmd = 0xC1;
constexpr uint8_t ipmbIpmiCmdRespNotProvided = 0xCE;

constexpr uint8_t ipmbReqNetFnFromRespNetFn(uint8_t reqNetFn)
{
    return reqNetFn & ~ipmbNetFnResponseMask;
}

/**
 * @brief IpmbChannel class declaration
 */
class IpmbChannel
{
  public:
    IpmbChannel(boost::asio::io_service &io, uint8_t ipmbBmcSlaveAddress,
                uint8_t ipmbRqSlaveAddress, ipmbChannelType type,
                std::shared_ptr<IpmbCommandFilter> commandFilter);

    IpmbChannel(const IpmbChannel &) = delete;
    IpmbChannel &operator=(IpmbChannel const &) = delete;

    int ipmbChannelInit(const char *ipmbI2cSlave);

    int ipmbChannelUpdateSlaveAddress(const uint8_t newBmcSlaveAddr);

    bool seqNumGet(uint8_t &seq);

    ipmbChannelType getChannelType();

    uint8_t getBusId();

    uint8_t getBmcSlaveAddress();

    uint8_t getRqSlaveAddress();

    void addFilter(const uint8_t respNetFn, const uint8_t cmd);

    void processI2cEvent();

    void ipmbSendI2cFrame(std::shared_ptr<std::vector<uint8_t>> buffer,
                          size_t retriesAttempted);

    std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
        requestAdd(boost::asio::yield_context &yield,
                   std::shared_ptr<IpmbRequest> requestToSend);

  private:
    boost::asio::posix::stream_descriptor i2cSlaveDescriptor;

    int ipmbi2cSlaveFd;

    uint8_t ipmbBmcSlaveAddress;
    uint8_t ipmbRqSlaveAddress;
    uint8_t ipmbBusId;

    ipmbChannelType type;

    std::shared_ptr<IpmbCommandFilter> commandFilter;

    // array storing outstanding requests
    std::array<std::shared_ptr<IpmbRequest>, ipmbMaxOutstandingRequestsCount>
        outstandingRequests;

    void requestTimerCallback(std::shared_ptr<IpmbRequest> request,
                              std::shared_ptr<std::vector<uint8_t>> buffer);

    void responseMatch(std::unique_ptr<IpmbResponse> &response);

    void makeRequestInvalid(IpmbRequest &request);

    void makeRequestValid(std::shared_ptr<IpmbRequest> request);
};

/**
 * @brief ioWrite class declaration
 */
class ioWrite
{
  public:
    ioWrite(std::vector<uint8_t> &buffer)
    {
        i2cmsg[0].addr = ipmbAddressTo7BitSet(buffer[0]);
        i2cmsg[0].len = buffer.size() - ipmbAddressSize;
        i2cmsg[0].buf = buffer.data() + ipmbAddressSize;

        msgRdwr.msgs = i2cmsg;
        msgRdwr.nmsgs = 1;
    };

    int name()
    {
        return static_cast<int>(I2C_RDWR);
    }

    void *data()
    {
        return &msgRdwr;
    }

  private:
    int myname;
    i2c_rdwr_ioctl_data msgRdwr = {0};
    i2c_msg i2cmsg[1] = {0};
};

#endif
