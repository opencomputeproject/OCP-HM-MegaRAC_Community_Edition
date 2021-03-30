#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{
using ::testing::ElementsAreArray;
using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobWriteMetaTest, ManagerReturnsFailureReturnsFailure)
{
    // This verifies a failure from the manager is passed back.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobWriteMetaTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteMetaTx) + sizeof(expectedBytes);

    EXPECT_CALL(
        mgr, writeMeta(req->sessionId, req->offset,
                       ElementsAreArray(expectedBytes, sizeof(expectedBytes))))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              writeMeta(&mgr, request, reply, &dataLen));
}

TEST(BlobWriteMetaTest, ManagerReturnsTrueWriteSucceeds)
{
    // The case where everything works.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobWriteMetaTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteMetaTx) + sizeof(expectedBytes);

    EXPECT_CALL(
        mgr, writeMeta(req->sessionId, req->offset,
                       ElementsAreArray(expectedBytes, sizeof(expectedBytes))))
        .WillOnce(Return(true));

    EXPECT_EQ(IPMI_CC_OK, writeMeta(&mgr, request, reply, &dataLen));
}
} // namespace blobs
