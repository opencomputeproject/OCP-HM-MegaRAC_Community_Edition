#include "ethstats.hpp"
#include "handler_mock.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#define MAX_IPMI_BUFFER 64

using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

namespace ethstats
{

TEST(EthStatsTest, InvalidStatReturnsFailure)
{
    // Verify that the enum of valid statistic IDs is checked.

    std::string ifName = "eth0";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::TX_WINDOW_ERRORS + 1;
    requestStruct.if_name_len = ifName.length();

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    size_t dataLen = request.size();
    std::uint8_t reply[MAX_IPMI_BUFFER];

    // Using StrictMock to ensure it isn't called.
    StrictMock<HandlerMock> hMock;

    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));
}

TEST(EthStatsTest, InvalidIpmiPacketSize)
{
    // An IPMI packet for this command has a minimum length.

    std::string ifName = "e";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::RX_BYTES;
    requestStruct.if_name_len = ifName.length();

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    // The minimum length is a 1-byte ifname - this gives one, but dataLen is
    // set to smaller.
    size_t dataLen = request.size() - 1;
    std::uint8_t reply[MAX_IPMI_BUFFER];

    // Using StrictMock to ensure it isn't called.
    StrictMock<HandlerMock> hMock;

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));
}

TEST(EthStatsTest, InvalidIpmiPacketContents)
{
    // The packet has a name length and name contents, if the name length is
    // longer than the packet size, it should fail.

    std::string ifName = "eth0";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::RX_BYTES;
    requestStruct.if_name_len = ifName.length() + 1;

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    size_t dataLen = request.size();
    std::uint8_t reply[MAX_IPMI_BUFFER];

    // Using StrictMock to ensure it isn't called.
    StrictMock<HandlerMock> hMock;

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));
}

TEST(EthStatsTest, NameHasIllegalCharacters)
{
    // The interface name cannot have slashes.
    std::string ifName = "et/h0";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::RX_BYTES;
    requestStruct.if_name_len = ifName.length();

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    size_t dataLen = request.size();
    std::uint8_t reply[MAX_IPMI_BUFFER];

    // Using StrictMock to ensure it isn't called.
    StrictMock<HandlerMock> hMock;

    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));
}

TEST(EthStatsTest, InvalidNameOrField)
{
    // The handler returns failure on the input validity check.
    std::string ifName = "eth0";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::RX_BYTES;
    requestStruct.if_name_len = ifName.length();

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    size_t dataLen = request.size();
    std::uint8_t reply[MAX_IPMI_BUFFER];

    std::string expectedPath = buildPath(ifName, "rx_bytes");

    HandlerMock hMock;
    EXPECT_CALL(hMock, validIfNameAndField(StrEq(expectedPath)))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));
}

TEST(EthStatsTest, EverythingHappy)
{
    std::string ifName = "eth0";
    struct EthStatRequest requestStruct;
    requestStruct.statId = EthernetStatisticsIds::RX_BYTES;
    requestStruct.if_name_len = ifName.length();

    std::vector<std::uint8_t> request(sizeof(requestStruct) + ifName.length());
    std::memcpy(request.data(), &requestStruct, sizeof(requestStruct));
    std::memcpy(&request[sizeof(requestStruct)], ifName.c_str(),
                ifName.length());

    size_t dataLen = request.size();
    std::uint8_t reply[MAX_IPMI_BUFFER];

    std::string expectedPath = buildPath(ifName, "rx_bytes");

    HandlerMock hMock;
    EXPECT_CALL(hMock, validIfNameAndField(StrEq(expectedPath)))
        .WillOnce(Return(true));
    EXPECT_CALL(hMock, readStatistic(StrEq(expectedPath))).WillOnce(Return(1));

    EXPECT_EQ(IPMI_CC_OK,
              handleEthStatCommand(request.data(), reply, &dataLen, &hMock));

    struct EthStatReply expectedReply, realReply;
    expectedReply.statId = EthernetStatisticsIds::RX_BYTES;
    expectedReply.value = 1;

    std::memcpy(&realReply, reply, sizeof(realReply));
    EXPECT_EQ(0, std::memcmp(&expectedReply, &realReply, sizeof(realReply)));
}

} // namespace ethstats
