#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobCloseTest, ManagerRejectsCloseReturnsFailure)
{
    // The session manager returned failure to close, which we need to pass on.

    ManagerMock mgr;
    uint16_t sessionId = 0x54;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobCloseTx req;

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobClose);
    req.crc = 0;
    req.sessionId = sessionId;

    dataLen = sizeof(req);

    std::memcpy(request, &req, sizeof(req));

    EXPECT_CALL(mgr, close(sessionId)).WillOnce(Return(false));
    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              closeBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobCloseTest, BlobClosedReturnsSuccess)
{
    // Verify that if all goes right, success is returned.

    ManagerMock mgr;
    uint16_t sessionId = 0x54;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobCloseTx req;

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobClose);
    req.crc = 0;
    req.sessionId = sessionId;

    dataLen = sizeof(req);

    std::memcpy(request, &req, sizeof(req));

    EXPECT_CALL(mgr, close(sessionId)).WillOnce(Return(true));
    EXPECT_EQ(IPMI_CC_OK, closeBlob(&mgr, request, reply, &dataLen));
}
} // namespace blobs
