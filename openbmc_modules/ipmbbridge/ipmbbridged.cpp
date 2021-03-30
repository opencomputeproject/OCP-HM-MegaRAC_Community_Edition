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

#include "ipmbbridged.hpp"

#include "ipmbdefines.hpp"
#include "ipmbutils.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <phosphor-logging/log.hpp>
#include <tuple>
#include <unordered_map>

/**
 * @brief Dbus
 */
static constexpr const char *ipmbBus = "xyz.openbmc_project.Ipmi.Channel.Ipmb";
static constexpr const char *ipmbObj = "/xyz/openbmc_project/Ipmi/Channel/Ipmb";
static constexpr const char *ipmbDbusIntf = "org.openbmc.Ipmb";

boost::asio::io_service io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

static std::list<IpmbChannel> ipmbChannels;
static const std::unordered_map<std::string, ipmbChannelType>
    ipmbChannelTypeMap = {{"me", ipmbChannelType::me},
                          {"ipmb", ipmbChannelType::ipmb}};

/**
 * @brief Ipmb request class methods
 */
IpmbRequest::IpmbRequest()
{
    data.reserve(ipmbMaxDataSize);
}

IpmbRequest::IpmbRequest(uint8_t address, uint8_t netFn, uint8_t rsLun,
                         uint8_t rqSA, uint8_t seq, uint8_t rqLun, uint8_t cmd,
                         const std::vector<uint8_t> &inputData) :
    address(address),
    netFn(netFn), rsLun(rsLun), rqSA(rqSA), seq(seq), rqLun(rqLun), cmd(cmd),
    timer(io)
{
    data.reserve(ipmbMaxDataSize);
    state = ipmbRequestState::invalid;

    if (inputData.size() > 0)
    {
        data = std::move(inputData);
    }
}

void IpmbRequest::i2cToIpmbConstruct(IPMB_HEADER *ipmbBuffer,
                                     size_t bufferLength)
{
    // constructing ipmb request from i2c buffer
    netFn = ipmbNetFnGet(ipmbBuffer->Header.Req.rsNetFnLUN);
    rsLun = ipmbLunFromNetFnLunGet(ipmbBuffer->Header.Req.rsNetFnLUN);
    rqSA = ipmbBuffer->Header.Req.rqSA;
    seq = ipmbSeqGet(ipmbBuffer->Header.Req.rqSeqLUN);
    rqLun = ipmbLunFromSeqLunGet(ipmbBuffer->Header.Req.rqSeqLUN);
    cmd = ipmbBuffer->Header.Req.cmd;

    size_t dataLength =
        bufferLength - (ipmbConnectionHeaderLength +
                        ipmbRequestDataHeaderLength + ipmbChecksumSize);

    if (dataLength > 0)
    {
        data.insert(data.end(), ipmbBuffer->Header.Req.data,
                    &ipmbBuffer->Header.Req.data[dataLength]);
    }
}

int IpmbRequest::ipmbToi2cConstruct(std::vector<uint8_t> &buffer)
{
    /* Add one byte for length byte as per required by driver */
    size_t bufferLength = 1 + data.size() + ipmbRequestDataHeaderLength +
                          ipmbConnectionHeaderLength + ipmbChecksumSize;

    if (bufferLength > ipmbMaxFrameLength)
    {
        return -1;
    }

    buffer.resize(bufferLength);
    static_assert(ipmbMaxFrameLength >= sizeof(IPMB_HEADER));
    IPMB_PKT *ipmbPkt = reinterpret_cast<IPMB_PKT *>(buffer.data());
    ipmbPkt->len = bufferLength - 1;
    IPMB_HEADER *ipmbBuffer = &(ipmbPkt->hdr);

    // constructing buffer from ipmb request
    ipmbBuffer->Header.Req.address = address;
    ipmbBuffer->Header.Req.rsNetFnLUN = ipmbNetFnLunSet(netFn, rsLun);
    ipmbBuffer->Header.Req.rqSA = rqSA;
    ipmbBuffer->Header.Req.rqSeqLUN = ipmbSeqLunSet(seq, rqLun);
    ipmbBuffer->Header.Req.cmd = cmd;

    ipmbBuffer->Header.Req.checksum1 = ipmbChecksumCompute(
        (uint8_t *)ipmbBuffer, ipmbConnectionHeaderLength - ipmbChecksumSize);

    if (data.size() > 0)
    {
        std::copy(data.begin(), data.end(), ipmbBuffer->Header.Req.data);
    }

    buffer[bufferLength - ipmbChecksumSize] =
        ipmbChecksumCompute((uint8_t *)ipmbBuffer + ipmbChecksum2StartOffset,
                            (ipmbRequestDataHeaderLength + data.size()));

    return 0;
}

std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
    IpmbRequest::returnMatchedResponse()
{
    return std::make_tuple(
        static_cast<int>(ipmbResponseStatus::success), matchedResponse->netFn,
        matchedResponse->rsLun, matchedResponse->cmd,
        matchedResponse->completionCode, matchedResponse->data);
}

static std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
    returnStatus(ipmbResponseStatus status)
{
    // we only want to send status here, other fields are not relevant
    return std::make_tuple(static_cast<int>(status), 0, 0, 0, 0,
                           std::vector<uint8_t>(0));
}

/**
 * @brief Ipmb response class methods
 */
IpmbResponse::IpmbResponse()
{
    data.reserve(ipmbMaxDataSize);
}

IpmbResponse::IpmbResponse(uint8_t address, uint8_t netFn, uint8_t rqLun,
                           uint8_t rsSA, uint8_t seq, uint8_t rsLun,
                           uint8_t cmd, uint8_t completionCode,
                           const std::vector<uint8_t> &inputData) :
    address(address),
    netFn(netFn), rqLun(rqLun), rsSA(rsSA), seq(seq), rsLun(rsLun), cmd(cmd),
    completionCode(completionCode)
{
    data.reserve(ipmbMaxDataSize);

    if (inputData.size() > 0)
    {
        data = std::move(inputData);
    }
}

void IpmbResponse::i2cToIpmbConstruct(IPMB_HEADER *ipmbBuffer,
                                      size_t bufferLength)
{
    netFn = ipmbNetFnGet(ipmbBuffer->Header.Resp.rqNetFnLUN);
    rqLun = ipmbLunFromNetFnLunGet(ipmbBuffer->Header.Resp.rqNetFnLUN);
    rsSA = ipmbBuffer->Header.Resp.rsSA;
    seq = ipmbSeqGet(ipmbBuffer->Header.Resp.rsSeqLUN);
    rsLun = ipmbLunFromSeqLunGet(ipmbBuffer->Header.Resp.rsSeqLUN);
    cmd = ipmbBuffer->Header.Resp.cmd;
    completionCode = ipmbBuffer->Header.Resp.completionCode;

    size_t dataLength =
        bufferLength - (ipmbConnectionHeaderLength +
                        ipmbResponseDataHeaderLength + ipmbChecksumSize);

    if (dataLength > 0)
    {
        data.insert(data.end(), ipmbBuffer->Header.Resp.data,
                    &ipmbBuffer->Header.Resp.data[dataLength]);
    }
}

std::shared_ptr<std::vector<uint8_t>> IpmbResponse::ipmbToi2cConstruct()
{
    /* Add one byte for length byte as per required by driver */
    size_t bufferLength = 1 + data.size() + ipmbResponseDataHeaderLength +
                          ipmbConnectionHeaderLength + ipmbChecksumSize;

    if (bufferLength > ipmbMaxFrameLength)
    {
        return nullptr;
    }

    std::shared_ptr<std::vector<uint8_t>> buffer =
        std::make_shared<std::vector<uint8_t>>(bufferLength);

    IPMB_PKT *ipmbPkt = reinterpret_cast<IPMB_PKT *>(buffer->data());
    ipmbPkt->len = bufferLength - 1;
    IPMB_HEADER *ipmbBuffer = &(ipmbPkt->hdr);

    ipmbBuffer->Header.Resp.address = address;
    ipmbBuffer->Header.Resp.rqNetFnLUN = ipmbNetFnLunSet(netFn, rqLun);
    ipmbBuffer->Header.Resp.rsSA = rsSA;
    ipmbBuffer->Header.Resp.rsSeqLUN = ipmbSeqLunSet(seq, rsLun);
    ipmbBuffer->Header.Resp.cmd = cmd;
    ipmbBuffer->Header.Resp.completionCode = completionCode;

    ipmbBuffer->Header.Resp.checksum1 = ipmbChecksumCompute(
        (uint8_t *)ipmbBuffer, ipmbConnectionHeaderLength - ipmbChecksumSize);

    if (data.size() > 0)
    {
        std::copy(data.begin(), data.end(), ipmbBuffer->Header.Resp.data);
    }

    (*buffer)[bufferLength - ipmbChecksumSize] =
        ipmbChecksumCompute((uint8_t *)ipmbBuffer + ipmbChecksum2StartOffset,
                            (ipmbResponseDataHeaderLength + data.size()));

    return buffer;
}

bool IpmbCommandFilter::isBlocked(const uint8_t reqNetFn, const uint8_t cmd)
{
    auto blockedCmd = unhandledCommands.find({reqNetFn, cmd});

    if (blockedCmd != unhandledCommands.end())
    {
        return true;
    }

    return false;
}

void IpmbCommandFilter::addFilter(const uint8_t reqNetFn, const uint8_t cmd)
{
    if (unhandledCommands.insert({reqNetFn, cmd}).second)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "addFilter: added command to filter",
            phosphor::logging::entry("netFn = %d", reqNetFn),
            phosphor::logging::entry("cmd = %d", cmd));
    }
}

/**
 * @brief Ipmb channel
 */
void IpmbChannel::ipmbSendI2cFrame(std::shared_ptr<std::vector<uint8_t>> buffer,
                                   size_t retriesAttempted = 0)
{
    IPMB_PKT *ipmbPkt = reinterpret_cast<IPMB_PKT *>(buffer->data());
    uint8_t targetAddr = ipmbIsResponse(&(ipmbPkt->hdr))
                             ? ipmbPkt->hdr.Header.Resp.address
                             : ipmbPkt->hdr.Header.Req.address;
    boost::asio::async_write(
        i2cSlaveDescriptor, boost::asio::buffer(*buffer),
        [this, buffer, retriesAttempted,
         targetAddr](const boost::system::error_code &ec, size_t bytesSent) {
            if (ec)
            {
                size_t currentRetryCnt = retriesAttempted;

                if (currentRetryCnt > ipmbI2cNumberOfRetries)
                {
                    std::string msgToLog =
                        "ipmbSendI2cFrame: send to I2C failed after retries."
                        " busId=" +
                        std::to_string(ipmbBusId) +
                        ", targetAddr=" + std::to_string(targetAddr) +
                        ", error=" + ec.message();
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        msgToLog.c_str());
                    return;
                }
                currentRetryCnt++;
                ipmbSendI2cFrame(buffer, currentRetryCnt);
            }
        });
}

/**
 * @brief Ipmb Outstanding Requests
 */
void IpmbChannel::makeRequestInvalid(IpmbRequest &request)
{
    // change request state to invalid and remove it from outstanding requests
    // list
    request.state = ipmbRequestState::invalid;
    outstandingRequests[request.seq] = nullptr;
}

void IpmbChannel::makeRequestValid(std::shared_ptr<IpmbRequest> request)
{
    // change request state to valid and add it to outstanding requests list
    request->state = ipmbRequestState::valid;
    outstandingRequests[request->seq] = request;
}

bool IpmbChannel::seqNumGet(uint8_t &seq)
{
    static uint8_t seqNum = 0;

    for (int i = 0; i < ipmbMaxOutstandingRequestsCount; i++)
    {
        seqNum = ++seqNum & ipmbSeqMask;
        if (seqNum == ipmbMaxOutstandingRequestsCount)
        {
            seqNum = 0;
        }

        if (outstandingRequests[seqNum] == nullptr)
        {
            seq = seqNum;
            return true;
        }
    }

    return false;
}

void IpmbChannel::responseMatch(std::unique_ptr<IpmbResponse> &response)
{
    std::shared_ptr<IpmbRequest> request = outstandingRequests[response->seq];

    if (request != nullptr)
    {
        if (((ipmbRespNetFn(request->netFn)) == (response->netFn)) &&
            ((request->rqLun) == (response->rqLun)) &&
            ((request->rsLun) == (response->rsLun)) &&
            ((request->cmd) == (response->cmd)))
        {
            // match, response is corresponding to previously sent request
            request->state = ipmbRequestState::matched;
            request->timer->cancel();
            request->matchedResponse = std::move(response);
        }
    }
}

void IpmbChannel::processI2cEvent()
{
    std::array<uint8_t, ipmbMaxFrameLength> buffer{};
    IPMB_PKT *ipmbPkt = reinterpret_cast<IPMB_PKT *>(buffer.data());
    IPMB_HEADER *ipmbFrame = &(ipmbPkt->hdr);

    lseek(ipmbi2cSlaveFd, 0, SEEK_SET);
    int r = read(ipmbi2cSlaveFd, buffer.data(), ipmbMaxFrameLength);

    /* Substract first byte len size from total frame length */
    r--;

    if ((r < ipmbMinFrameLength) || (r > ipmbMaxFrameLength))
    {
        goto end;
    }

    // valiate the frame
    if (!isFrameValid(ipmbFrame, r))
    {
        goto end;
    }

    // if it is broadcast message from ipmb channel, send out dbus signal
    if (ipmbFrame->Header.Req.address == broadcastAddress &&
        getChannelType() == ipmbChannelType::ipmb)
    {
        auto ipmbMessageReceived = IpmbRequest();
        ipmbMessageReceived.i2cToIpmbConstruct(ipmbFrame, r);
        sdbusplus::message::message msg =
            conn->new_signal(ipmbObj, ipmbDbusIntf, "receiveBroadcast");
        msg.append(ipmbMessageReceived.netFn, ipmbMessageReceived.cmd,
                   ipmbMessageReceived.data);
        msg.signal_send();
    }

    // copy frame to ipmib message buffer
    else if (ipmbIsResponse(ipmbFrame))
    {
        std::unique_ptr<IpmbResponse> ipmbMessageReceived =
            std::make_unique<IpmbResponse>();

        ipmbMessageReceived->i2cToIpmbConstruct(ipmbFrame, r);

        // try to match response with outstanding request
        responseMatch(ipmbMessageReceived);
    }
    else
    {
        // if command is blocked - respond with 'invalid command'
        // completion code
        if (commandFilter)
        {
            uint8_t netFn = ipmbNetFnGet(ipmbFrame->Header.Req.rsNetFnLUN);
            uint8_t cmd = ipmbFrame->Header.Req.cmd;
            uint8_t rqSA = ipmbFrame->Header.Req.rqSA;

            if (commandFilter->isBlocked(netFn, cmd))
            {
                uint8_t seq = ipmbSeqGet(ipmbFrame->Header.Req.rqSeqLUN);
                uint8_t lun =
                    ipmbLunFromSeqLunGet(ipmbFrame->Header.Req.rqSeqLUN);

                // prepare generic response
                auto ipmbResponse = IpmbResponse(
                    rqSA, ipmbRespNetFn(netFn), lun, ipmbBmcSlaveAddress, seq,
                    ipmbRsLun, cmd, ipmbIpmiInvalidCmd, {});

                auto buffer = ipmbResponse.ipmbToi2cConstruct();
                if (buffer)
                {
                    ipmbSendI2cFrame(buffer);
                }

                goto end;
            }
        }

        auto ipmbMessageReceived = IpmbRequest();
        ipmbMessageReceived.i2cToIpmbConstruct(ipmbFrame, r);

        std::map<std::string, std::variant<int>> options{
            {"rqSA", ipmbAddressTo7BitSet(ipmbMessageReceived.rqSA)}};
        using IpmiDbusRspType = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,
                                           std::vector<uint8_t>>;
        conn->async_method_call(
            [this, rqLun{ipmbMessageReceived.rqLun},
             seq{ipmbMessageReceived.seq}, address{ipmbMessageReceived.rqSA}](
                const boost::system::error_code &ec,
                const IpmiDbusRspType &response) {
                const auto &[netfn, lun, cmd, cc, payload] = response;
                if (ec)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "processI2cEvent: error getting response from IPMI");
                    return;
                }

                uint8_t bmcSlaveAddress = getBmcSlaveAddress();

                if (payload.size() > ipmbMaxDataSize)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "processI2cEvent: response exceeding maximum size");

                    // prepare generic response
                    auto ipmbResponse = IpmbResponse(
                        address, netfn, rqLun, bmcSlaveAddress, seq, ipmbRsLun,
                        cmd, ipmbIpmiCmdRespNotProvided, {});

                    auto buffer = ipmbResponse.ipmbToi2cConstruct();
                    if (buffer)
                    {
                        ipmbSendI2cFrame(buffer);
                    }

                    return;
                }

                if (!(netfn & ipmbNetFnResponseMask))
                {
                    // we are not expecting request here
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "processI2cEvent: got a request instead of response");
                    return;
                }

                // if command is not supported, add it to filter
                if (cc == ipmbIpmiInvalidCmd)
                {
                    addFilter(ipmbReqNetFnFromRespNetFn(netfn), cmd);
                }

                // payload is empty after constructor invocation
                auto ipmbResponse =
                    IpmbResponse(address, netfn, rqLun, bmcSlaveAddress, seq,
                                 lun, cmd, cc, payload);

                auto buffer = ipmbResponse.ipmbToi2cConstruct();
                if (!buffer)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "processI2cEvent: error constructing a request");
                    return;
                }

                ipmbSendI2cFrame(buffer);
            },
            "xyz.openbmc_project.Ipmi.Host", "/xyz/openbmc_project/Ipmi",
            "xyz.openbmc_project.Ipmi.Server", "execute",
            ipmbMessageReceived.netFn, ipmbMessageReceived.rsLun,
            ipmbMessageReceived.cmd, ipmbMessageReceived.data, options);
    }

end:
    i2cSlaveDescriptor.async_wait(
        boost::asio::posix::descriptor_base::wait_read,
        [this](const boost::system::error_code &ec) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error: processI2cEvent()");
                return;
            }

            processI2cEvent();
        });
}

IpmbChannel::IpmbChannel(boost::asio::io_service &io,
                         uint8_t ipmbBmcSlaveAddress,
                         uint8_t ipmbRqSlaveAddress, ipmbChannelType type,
                         std::shared_ptr<IpmbCommandFilter> commandFilter) :
    i2cSlaveDescriptor(io),
    ipmbBmcSlaveAddress(ipmbBmcSlaveAddress),
    ipmbRqSlaveAddress(ipmbRqSlaveAddress), type(type),
    commandFilter(commandFilter)
{
}

int IpmbChannel::ipmbChannelInit(const char *ipmbI2cSlave)
{
    // extract bus id from slave path and save
    std::string ipmbI2cSlaveStr(ipmbI2cSlave);
    auto findHyphen = ipmbI2cSlaveStr.find("-");
    std::string busStr = ipmbI2cSlaveStr.substr(findHyphen + 1);
    try
    {
        ipmbBusId = std::stoi(busStr);
    }
    catch (std::invalid_argument)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbChannelInit: invalid bus id in slave-path config");
        return -1;
    }

    // Check if sysfs has device. If not, enable I2C slave driver by command
    // echo "ipmb-dev 0x1010" > /sys/bus/i2c/devices/i2c-0/new_device
    bool hasSysfs = std::filesystem::exists(ipmbI2cSlave);
    if (!hasSysfs)
    {
        std::string deviceFileName =
            "/sys/bus/i2c/devices/i2c-" + busStr + "/new_device";
        std::string para = "ipmb-dev 0x1010"; // init with BMC addr 0x20
        std::fstream deviceFile;
        deviceFile.open(deviceFileName, std::ios::out);
        if (!deviceFile.good())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "ipmbChannelInit: error opening deviceFile");
            return -1;
        }
        deviceFile << para;
        deviceFile.close();
    }

    // open fd to i2c slave device for read write
    ipmbi2cSlaveFd = open(ipmbI2cSlave, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (ipmbi2cSlaveFd < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbChannelInit: error opening ipmbI2cSlave");
        return -1;
    }

    i2cSlaveDescriptor.assign(ipmbi2cSlaveFd);

    i2cSlaveDescriptor.async_wait(
        boost::asio::posix::descriptor_base::wait_read,
        [this](const boost::system::error_code &ec) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error: processI2cEvent()");
                return;
            }

            processI2cEvent();
        });

    return 0;
}

int IpmbChannel::ipmbChannelUpdateSlaveAddress(const uint8_t newBmcSlaveAddr)
{
    if (ipmbi2cSlaveFd > 0)
    {
        i2cSlaveDescriptor.close();
        close(ipmbi2cSlaveFd);
        ipmbi2cSlaveFd = 0;
    }

    // disable old I2C slave driver by command:
    //     echo "0x1010" > /sys/bus/i2c/devices/i2c-0/delete_device
    std::string deviceFileName;
    std::string para;
    std::fstream deviceFile;
    deviceFileName = "/sys/bus/i2c/devices/i2c-" + std::to_string(ipmbBusId) +
                     "/delete_device";
    para = "0x1010"; // align with removed ipmb0 definition in dts file
    deviceFile.open(deviceFileName, std::ios::out);
    if (!deviceFile.good())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbChannelUpdateSlaveAddress: error opening deviceFile to delete "
            "sysfs");
        return -1;
    }
    deviceFile << para;
    deviceFile.close();

    // enable new I2C slave driver by command:
    //      echo "ipmb-dev 0x1012" > /sys/bus/i2c/devices/i2c-0/new_device
    deviceFileName =
        "/sys/bus/i2c/devices/i2c-" + std::to_string(ipmbBusId) + "/new_device";
    std::ostringstream hex;
    uint16_t addr = 0x1000 + (newBmcSlaveAddr >> 1);
    hex << std::hex << static_cast<uint16_t>(addr);
    const std::string &addressHexStr = hex.str();
    para = "ipmb-dev 0x" + addressHexStr;
    deviceFile.open(deviceFileName, std::ios::out);
    if (!deviceFile.good())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbChannelUpdateSlaveAddress: error opening deviceFile to create "
            "sysfs");
        return -1;
    }
    deviceFile << para;
    deviceFile.close();

    // open fd to i2c slave device
    std::string ipmbI2cSlaveStr = "/dev/ipmb-" + std::to_string(ipmbBusId);
    ipmbi2cSlaveFd = open(ipmbI2cSlaveStr.c_str(), O_RDWR | O_NONBLOCK);
    if (ipmbi2cSlaveFd < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbChannelInit: error opening ipmbI2cSlave");
        return -1;
    }

    // start to receive i2c data as slave
    i2cSlaveDescriptor.assign(ipmbi2cSlaveFd);
    i2cSlaveDescriptor.async_wait(
        boost::asio::posix::descriptor_base::wait_read,
        [this](const boost::system::error_code &ec) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Error: processI2cEvent()");
                return;
            }

            processI2cEvent();
        });

    ipmbBmcSlaveAddress = newBmcSlaveAddr;

    return 0;
}

uint8_t IpmbChannel::getBusId()
{
    return ipmbBusId;
}

uint8_t IpmbChannel::getBmcSlaveAddress()
{
    return ipmbBmcSlaveAddress;
}

uint8_t IpmbChannel::getRqSlaveAddress()
{
    return ipmbRqSlaveAddress;
}

ipmbChannelType IpmbChannel::getChannelType()
{
    return type;
}

void IpmbChannel::addFilter(const uint8_t respNetFn, const uint8_t cmd)
{
    if (commandFilter)
    {
        commandFilter->addFilter(respNetFn, cmd);
    }
}

std::tuple<int, uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
    IpmbChannel::requestAdd(boost::asio::yield_context &yield,
                            std::shared_ptr<IpmbRequest> request)
{
    makeRequestValid(request);

    std::vector<uint8_t> buffer(0);
    if (request->ipmbToi2cConstruct(buffer) != 0)
    {
        return returnStatus(ipmbResponseStatus::error);
    }

    for (int i = 0; i < ipmbNumberOfTries; i++)
    {
        boost::system::error_code ec;
        int i2cRetryCnt = 0;

        for (; i2cRetryCnt < ipmbI2cNumberOfRetries; i2cRetryCnt++)
        {
            boost::asio::async_write(i2cSlaveDescriptor,
                                     boost::asio::buffer(buffer), yield[ec]);

            if (ec)
            {
                continue; // retry
            }
            break;
        }

        if (i2cRetryCnt == ipmbI2cNumberOfRetries)
        {
            std::string msgToLog =
                "requestAdd: Sent to I2C failed after retries."
                " busId=" +
                std::to_string(ipmbBusId) + ", error=" + ec.message();
            phosphor::logging::log<phosphor::logging::level::INFO>(
                msgToLog.c_str());
        }

        request->timer->expires_after(
            std::chrono::milliseconds(ipmbRequestRetryTimeout));
        request->timer->async_wait(yield[ec]);

        if (ec && ec != boost::asio::error::operation_aborted)
        {
            // unexpected error - invalidate request and return generic error
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "requestAdd: async_wait error");
            makeRequestInvalid(*request);
            return returnStatus(ipmbResponseStatus::error);
        }

        if (request->state == ipmbRequestState::matched)
        {
            // matched response, send it to client application
            makeRequestInvalid(*request);
            return request->returnMatchedResponse();
        }
    }

    makeRequestInvalid(*request);
    return returnStatus(ipmbResponseStatus::timeout);
}

static IpmbChannel *getChannel(ipmbChannelType channelType)
{
    auto channel =
        std::find_if(ipmbChannels.begin(), ipmbChannels.end(),
                     [channelType](IpmbChannel &channel) {
                         return channel.getChannelType() == channelType;
                     });
    if (channel != ipmbChannels.end())
    {
        return &(*channel);
    }

    return nullptr;
}

static int initializeChannels()
{
    std::shared_ptr<IpmbCommandFilter> commandFilter =
        std::make_shared<IpmbCommandFilter>();

    constexpr const char *configFilePath =
        "/usr/share/ipmbbridge/ipmb-channels.json";
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "initializeChannels: Cannot open config path");
        return -1;
    }
    try
    {
        auto data = nlohmann::json::parse(configFile, nullptr);
        for (const auto &channelConfig : data["channels"])
        {
            const std::string &typeConfig = channelConfig["type"];
            const std::string &slavePath = channelConfig["slave-path"];
            uint8_t bmcAddr = channelConfig["bmc-addr"];
            uint8_t reqAddr = channelConfig["remote-addr"];
            ipmbChannelType type = ipmbChannelTypeMap.at(typeConfig);

            auto channel = ipmbChannels.emplace(ipmbChannels.end(), io, bmcAddr,
                                                reqAddr, type, commandFilter);
            if (channel->ipmbChannelInit(slavePath.c_str()) < 0)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "initializeChannels: channel initialization failed");
                return -1;
            }
        }
    }
    catch (nlohmann::json::exception &e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "initializeChannels: Error parsing config file");
        return -1;
    }
    catch (std::out_of_range &e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "initializeChannels: Error invalid type");
        return -1;
    }
    return 0;
}

auto ipmbHandleRequest = [](boost::asio::yield_context yield,
                            uint8_t reqChannel, uint8_t netfn, uint8_t lun,
                            uint8_t cmd, std::vector<uint8_t> dataReceived) {
    IpmbChannel *channel = getChannel(static_cast<ipmbChannelType>(reqChannel));
    if (channel == nullptr)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbHandleRequest: requested channel does not exist");
        return returnStatus(ipmbResponseStatus::invalid_param);
    }

    // check outstanding request list for valid sequence number
    uint8_t seqNum = 0;
    bool seqValid = channel->seqNumGet(seqNum);
    if (!seqValid)
    {
        phosphor::logging::log<phosphor::logging::level::WARNING>(
            "ipmbHandleRequest: cannot add more requests to the list");
        return returnStatus(ipmbResponseStatus::busy);
    }

    uint8_t bmcSlaveAddress = channel->getBmcSlaveAddress();
    uint8_t rqSlaveAddress = channel->getRqSlaveAddress();

    // construct the request to add it to outstanding request list
    std::shared_ptr<IpmbRequest> request = std::make_shared<IpmbRequest>(
        rqSlaveAddress, netfn, ipmbRsLun, bmcSlaveAddress, seqNum, lun, cmd,
        dataReceived);

    if (!request->timer)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "ipmbHandleRequest: timer object does not exist");
        return returnStatus(ipmbResponseStatus::error);
    }

    return channel->requestAdd(yield, request);
};

void addUpdateSlaveAddrHandler()
{
    // callback to handle dbus signal of updating slave addr
    std::function<void(sdbusplus::message::message &)> updateSlaveAddrHandler =
        [](sdbusplus::message::message &message) {
            uint8_t reqChannel, busId, slaveAddr;

            // valid source of signal, check whether from multi-node manager
            std::string pathName = message.get_path();
            if (pathName != "/xyz/openbmc_project/MultiNode/Status")
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "addUpdateSlaveAddrHandler: invalid obj path");
                return;
            }

            message.read(reqChannel, busId, slaveAddr);

            IpmbChannel *channel =
                getChannel(static_cast<ipmbChannelType>(reqChannel));
            if (channel == nullptr ||
                reqChannel != static_cast<uint8_t>(ipmbChannelType::ipmb))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "addUpdateSlaveAddrHandler: invalid channel");
                return;
            }
            if (busId != channel->getBusId())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "addUpdateSlaveAddrHandler: invalid busId");
                return;
            }
            if (channel->getBmcSlaveAddress() == slaveAddr)
            {
                phosphor::logging::log<phosphor::logging::level::INFO>(
                    "addUpdateSlaveAddrHandler: channel bmc slave addr is "
                    "unchanged, do nothing");
                return;
            }

            channel->ipmbChannelUpdateSlaveAddress(slaveAddr);
        };

    static auto match = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus &>(*conn),
        "type='signal',member='updateBmcSlaveAddr',", updateSlaveAddrHandler);
}

void addSendBroadcastHandler()
{
    // callback to handle dbus signal of sending broadcast message
    std::function<void(sdbusplus::message::message &)> sendBroadcastHandler =
        [](sdbusplus::message::message &message) {
            uint8_t reqChannel, netFn, lun, cmd;
            std::vector<uint8_t> dataReceived;
            message.read(reqChannel, netFn, lun, cmd, dataReceived);

            IpmbChannel *channel =
                getChannel(static_cast<ipmbChannelType>(reqChannel));
            if (channel == nullptr)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "addSendBroadcastMsgHandler: requested channel does not "
                    "exist");
                return;
            }

            uint8_t bmcSlaveAddress = channel->getBmcSlaveAddress();
            uint8_t seqNum = 0; // seqNum is not used in broadcast msg
            uint8_t targetAddr = broadcastAddress;

            std::shared_ptr<IpmbRequest> request =
                std::make_shared<IpmbRequest>(targetAddr, netFn, ipmbRsLun,
                                              bmcSlaveAddress, seqNum, lun, cmd,
                                              dataReceived);

            std::shared_ptr<std::vector<uint8_t>> buffer =
                std::make_shared<std::vector<uint8_t>>();

            if (request->ipmbToi2cConstruct(*buffer) != 0)
            {
                return;
            }

            channel->ipmbSendI2cFrame(buffer);
        };

    static auto match = std::make_unique<sdbusplus::bus::match::match>(
        static_cast<sdbusplus::bus::bus &>(*conn),
        "type='signal',member='sendBroadcast',", sendBroadcastHandler);
}

/**
 * @brief Main
 */
int main(int argc, char *argv[])
{
    conn->request_name(ipmbBus);

    auto server = sdbusplus::asio::object_server(conn);

    std::shared_ptr<sdbusplus::asio::dbus_interface> ipmbIface =
        server.add_interface(ipmbObj, ipmbDbusIntf);

    ipmbIface->register_method("sendRequest", std::move(ipmbHandleRequest));
    ipmbIface->initialize();

    if (initializeChannels() < 0)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Error initializeChannels");
        return -1;
    }

    addUpdateSlaveAddrHandler();

    addSendBroadcastHandler();

    io.run();
    return 0;
}
