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

TEST(BlobWriteTest, ManagerReturnsFailureReturnsFailure)
{
    // This verifies a failure from the manager is passed back.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    EXPECT_CALL(mgr,
                write(req->sessionId, req->offset,
                      ElementsAreArray(expectedBytes, sizeof(expectedBytes))))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              writeBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobWriteTest, ManagerReturnsTrueWriteSucceeds)
{
    // The case where everything works.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobWriteTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobWrite);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;

    uint8_t expectedBytes[2] = {0x66, 0x67};
    std::memcpy(req->data, &expectedBytes[0], sizeof(expectedBytes));

    dataLen = sizeof(struct BmcBlobWriteTx) + sizeof(expectedBytes);

    EXPECT_CALL(mgr,
                write(req->sessionId, req->offset,
                      ElementsAreArray(expectedBytes, sizeof(expectedBytes))))
        .WillOnce(Return(true));

    EXPECT_EQ(IPMI_CC_OK, writeBlob(&mgr, request, reply, &dataLen));
}
} // namespace blobs
