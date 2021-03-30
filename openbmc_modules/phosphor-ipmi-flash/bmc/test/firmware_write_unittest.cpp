#include "data.hpp"
#include "data_mock.hpp"
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::Eq;
using ::testing::Return;

class FirmwareHandlerWriteTestIpmiOnly : public IpmiOnlyFirmwareTest
{};

class FirmwareHandlerWriteTestLpc : public FakeLpcFirmwareTest
{};

TEST_F(FirmwareHandlerWriteTestIpmiOnly, DataTypeIpmiWriteSuccess)
{
    /* Verify if data type ipmi, it calls write with the bytes. */
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi, "asdf"));

    std::vector<std::uint8_t> bytes = {0xaa, 0x55};

    EXPECT_CALL(*imageMock, write(0, Eq(bytes))).WillOnce(Return(true));
    EXPECT_TRUE(handler->write(0, 0, bytes));
}

TEST_F(FirmwareHandlerWriteTestLpc, DataTypeNonIpmiWriteSuccess)
{
    /* Verify if data type non-ipmi, it calls write with the length. */
    EXPECT_CALL(dataMock, open()).WillOnce(Return(true));
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc, "asdf"));

    struct ExtChunkHdr request;
    request.length = 4; /* number of bytes to read. */
    std::vector<std::uint8_t> ipmiRequest;
    ipmiRequest.resize(sizeof(request));
    std::memcpy(ipmiRequest.data(), &request, sizeof(request));

    std::vector<std::uint8_t> bytes = {0x01, 0x02, 0x03, 0x04};

    EXPECT_CALL(dataMock, copyFrom(request.length)).WillOnce(Return(bytes));
    EXPECT_CALL(*imageMock, write(0, Eq(bytes))).WillOnce(Return(true));
    EXPECT_TRUE(handler->write(0, 0, ipmiRequest));
}

TEST_F(FirmwareHandlerWriteTestLpc, DataTypeNonIpmiWriteFailsBadRequest)
{
    /* Verify the data type non-ipmi, if the request's structure doesn't match,
     * return failure. */
    EXPECT_CALL(dataMock, open()).WillOnce(Return(true));
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc, "asdf"));

    struct ExtChunkHdr request;
    request.length = 4; /* number of bytes to read. */

    std::vector<std::uint8_t> ipmiRequest;
    ipmiRequest.resize(sizeof(request));
    std::memcpy(ipmiRequest.data(), &request, sizeof(request));
    ipmiRequest.push_back(1);

    /* ipmiRequest is too large by one byte. */
    EXPECT_FALSE(handler->write(0, 0, ipmiRequest));
}

} // namespace
} // namespace ipmi_flash
