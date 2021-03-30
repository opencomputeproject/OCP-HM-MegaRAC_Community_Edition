#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobDeleteTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobDeleteTx*>(request);
    std::string blobId = "abc";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobDelete);
    req->crc = 0;
    // length() doesn't include the nul-terminator.
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobDeleteTx) + blobId.length();

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              deleteBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobDeleteTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobDeleteTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobDelete);
    req->crc = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobDeleteTx) + blobId.length() + 1;

    EXPECT_CALL(mgr, deleteBlob(StrEq(blobId))).WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              deleteBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobDeleteTest, BlobDeleteReturnsOk)
{
    // The boring case where the blobId is deleted.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobDeleteTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobDelete);
    req->crc = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobDeleteTx) + blobId.length() + 1;

    EXPECT_CALL(mgr, deleteBlob(StrEq(blobId))).WillOnce(Return(true));

    EXPECT_EQ(IPMI_CC_OK, deleteBlob(&mgr, request, reply, &dataLen));
}
} // namespace blobs
