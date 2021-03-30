#include "ipmi.hpp"
#include "manager_mock.hpp"
#include "process.hpp"

#include <cstring>
#include <ipmiblob/test/crc_mock.hpp>

#include <gtest/gtest.h>

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace ipmiblob
{
CrcInterface* crcIntf = nullptr;

std::uint16_t generateCrc(const std::vector<std::uint8_t>& data)
{
    return (crcIntf) ? crcIntf->generateCrc(data) : 0x00;
}
} // namespace ipmiblob

namespace blobs
{
namespace
{

void EqualFunctions(IpmiBlobHandler lhs, IpmiBlobHandler rhs)
{
    EXPECT_FALSE(lhs == nullptr);
    EXPECT_FALSE(rhs == nullptr);

    ipmi_ret_t (*const* lPtr)(ManagerInterface*, const uint8_t*, uint8_t*,
                              size_t*) =
        lhs.target<ipmi_ret_t (*)(ManagerInterface*, const uint8_t*, uint8_t*,
                                  size_t*)>();

    ipmi_ret_t (*const* rPtr)(ManagerInterface*, const uint8_t*, uint8_t*,
                              size_t*) =
        rhs.target<ipmi_ret_t (*)(ManagerInterface*, const uint8_t*, uint8_t*,
                                  size_t*)>();

    EXPECT_TRUE(lPtr);
    EXPECT_TRUE(rPtr);
    EXPECT_EQ(*lPtr, *rPtr);
    return;
}

} // namespace

class ValidateBlobCommandTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ipmiblob::crcIntf = &crcMock;
    }

    ipmiblob::CrcMock crcMock;
};

TEST_F(ValidateBlobCommandTest, InvalidCommandReturnsFailure)
{
    // Verify we handle an invalid command.

    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = 0xff;         // There is no command 0xff.
    dataLen = sizeof(uint8_t); // There is no payload for CRC.
    ipmi_ret_t rc;

    EXPECT_EQ(nullptr, validateBlobCommand(request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST, rc);
}

TEST_F(ValidateBlobCommandTest, ValidCommandWithoutPayload)
{
    // Verify we handle a valid command that doesn't have a payload.

    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount);
    dataLen = sizeof(uint8_t); // There is no payload for CRC.
    ipmi_ret_t rc;

    IpmiBlobHandler res = validateBlobCommand(request, reply, &dataLen, &rc);
    EXPECT_FALSE(res == nullptr);
    EqualFunctions(getBlobCount, res);
}

TEST_F(ValidateBlobCommandTest, WithPayloadMinimumLengthIs3VerifyChecks)
{
    // Verify that if there's a payload, it's at least one command byte and
    // two bytes for the crc16 and then one data byte.

    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    request[0] = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount);
    dataLen = sizeof(uint8_t) + sizeof(uint16_t);
    // There is a payload, but there are insufficient bytes.
    ipmi_ret_t rc;

    EXPECT_EQ(nullptr, validateBlobCommand(request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID, rc);
}

TEST_F(ValidateBlobCommandTest, WithPayloadAndInvalidCrc)
{
    // Verify that the CRC is checked, and failure is reported.

    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);
    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0x34;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    // skip over cmd and crc.
    std::vector<std::uint8_t> bytes(&request[3], request + dataLen);
    EXPECT_CALL(crcMock, generateCrc(Eq(bytes))).WillOnce(Return(0x1234));

    ipmi_ret_t rc;

    EXPECT_EQ(nullptr, validateBlobCommand(request, reply, &dataLen, &rc));
    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR, rc);
}

TEST_F(ValidateBlobCommandTest, WithPayloadAndValidCrc)
{
    // Verify the CRC is checked and if it matches, return the handler.

    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);
    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0x3412;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    // skip over cmd and crc.
    std::vector<std::uint8_t> bytes(&request[3], request + dataLen);
    EXPECT_CALL(crcMock, generateCrc(Eq(bytes))).WillOnce(Return(0x3412));

    ipmi_ret_t rc;

    IpmiBlobHandler res = validateBlobCommand(request, reply, &dataLen, &rc);
    EXPECT_FALSE(res == nullptr);
    EqualFunctions(writeBlob, res);
}

class ProcessBlobCommandTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ipmiblob::crcIntf = &crcMock;
    }

    ipmiblob::CrcMock crcMock;
};

TEST_F(ProcessBlobCommandTest, CommandReturnsNotOk)
{
    // Verify that if the IPMI command handler returns not OK that this is
    // noticed and returned.

    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf,
                           size_t* dataLen) { return IPMI_CC_INVALID; };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_INVALID,
              processBlobCommand(h, &manager, request, reply, &dataLen));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithNoPayload)
{
    // Verify that if the IPMI command handler returns OK but without a payload
    // it doesn't try to compute a CRC.

    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = 0;
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_OK,
              processBlobCommand(h, &manager, request, reply, &dataLen));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithInvalidPayloadLength)
{
    // There is a minimum payload length of 2 bytes (the CRC only, no data, for
    // read), this returns 1.

    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};

    IpmiBlobHandler h = [](ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = sizeof(uint8_t);
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              processBlobCommand(h, &manager, request, reply, &dataLen));
}

TEST_F(ProcessBlobCommandTest, CommandReturnsOkWithValidPayloadLength)
{
    // There is a minimum payload length of 3 bytes, this command returns a
    // payload of 3 bytes and the crc code is called to process the payload.

    StrictMock<ManagerMock> manager;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    uint32_t payloadLen = sizeof(uint16_t) + sizeof(uint8_t);

    IpmiBlobHandler h = [payloadLen](ManagerInterface* mgr,
                                     const uint8_t* reqBuf,
                                     uint8_t* replyCmdBuf, size_t* dataLen) {
        (*dataLen) = payloadLen;
        replyCmdBuf[2] = 0x56;
        return IPMI_CC_OK;
    };

    dataLen = sizeof(request);

    EXPECT_CALL(crcMock, generateCrc(_)).WillOnce(Return(0x3412));

    EXPECT_EQ(IPMI_CC_OK,
              processBlobCommand(h, &manager, request, reply, &dataLen));
    EXPECT_EQ(dataLen, payloadLen);

    uint8_t expectedBytes[3] = {0x12, 0x34, 0x56};
    EXPECT_EQ(0, std::memcmp(expectedBytes, reply, sizeof(expectedBytes)));
}
} // namespace blobs
