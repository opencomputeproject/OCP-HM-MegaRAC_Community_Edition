#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobOpenTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobOpenTx*>(request);
    std::string blobId = "abc";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobOpen);
    req->crc = 0;
    req->flags = 0;
    // length() doesn't include the nul-terminator.
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobOpenTx) + blobId.length();

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              openBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobOpenTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobOpenTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobOpen);
    req->crc = 0;
    req->flags = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobOpenTx) + blobId.length() + 1;

    EXPECT_CALL(mgr, open(req->flags, StrEq(blobId), _))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              openBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobOpenTest, BlobOpenReturnsOk)
{
    // The boring case where the blobId opens.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobOpenTx*>(request);
    struct BmcBlobOpenRx rep;
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobOpen);
    req->crc = 0;
    req->flags = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobOpenTx) + blobId.length() + 1;
    uint16_t returnedSession = 0x54;

    EXPECT_CALL(mgr, open(req->flags, StrEq(blobId), NotNull()))
        .WillOnce(Invoke(
            [&](uint16_t flags, const std::string& path, uint16_t* session) {
                (*session) = returnedSession;
                return true;
            }));

    EXPECT_EQ(IPMI_CC_OK, openBlob(&mgr, request, reply, &dataLen));

    rep.crc = 0;
    rep.sessionId = returnedSession;

    EXPECT_EQ(sizeof(rep), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
}
} // namespace blobs
