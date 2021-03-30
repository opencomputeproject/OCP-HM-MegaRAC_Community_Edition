#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

// the request here is only the subcommand byte and therefore there's no invalid
// length check, etc to handle within the method.

TEST(BlobCountTest, ReturnsZeroBlobs)
{
    // Calling BmcBlobGetCount if there are no handlers registered should just
    // return that there are 0 blobs.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobCountTx req;
    struct BmcBlobCountRx rep;
    uint8_t* request = reinterpret_cast<uint8_t*>(&req);

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount);
    dataLen = sizeof(req);

    rep.crc = 0;
    rep.blobCount = 0;

    EXPECT_CALL(mgr, buildBlobList()).WillOnce(Return(0));

    EXPECT_EQ(IPMI_CC_OK, getBlobCount(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
}

TEST(BlobCountTest, ReturnsTwoBlobs)
{
    // Calling BmcBlobGetCount with one handler registered that knows of two
    // blobs will return that it found two blobs.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobCountTx req;
    struct BmcBlobCountRx rep;
    uint8_t* request = reinterpret_cast<uint8_t*>(&req);

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobGetCount);
    dataLen = sizeof(req);

    rep.crc = 0;
    rep.blobCount = 2;

    EXPECT_CALL(mgr, buildBlobList()).WillOnce(Return(2));

    EXPECT_EQ(IPMI_CC_OK, getBlobCount(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
}
} // namespace blobs
