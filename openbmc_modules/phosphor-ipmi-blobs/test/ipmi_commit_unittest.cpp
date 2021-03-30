#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::ElementsAreArray;
using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobCommitTest, InvalidCommitDataLengthReturnsFailure)
{
    // The commit command supports an optional commit blob.  This test verifies
    // we sanity check the length of that blob.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobCommitTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobCommit);
    req->crc = 0;
    req->sessionId = 0x54;
    req->commitDataLen =
        1; // It's one byte, but that's more than the packet size.

    dataLen = sizeof(struct BmcBlobCommitTx);

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              commitBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobCommitTest, ValidCommitNoDataHandlerRejectsReturnsFailure)
{
    // The commit packet is valid and the manager's commit call returns failure.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobCommitTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobCommit);
    req->crc = 0;
    req->sessionId = 0x54;
    req->commitDataLen = 0;

    dataLen = sizeof(struct BmcBlobCommitTx);

    EXPECT_CALL(mgr, commit(req->sessionId, _)).WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              commitBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobCommitTest, ValidCommitNoDataHandlerAcceptsReturnsSuccess)
{
    // Commit called with no data and everything returns success.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobCommitTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobCommit);
    req->crc = 0;
    req->sessionId = 0x54;
    req->commitDataLen = 0;

    dataLen = sizeof(struct BmcBlobCommitTx);

    EXPECT_CALL(mgr, commit(req->sessionId, _)).WillOnce(Return(true));

    EXPECT_EQ(IPMI_CC_OK, commitBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobCommitTest, ValidCommitWithDataHandlerAcceptsReturnsSuccess)
{
    // Commit called with extra data and everything returns success.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobCommitTx*>(request);

    uint8_t expectedBlob[4] = {0x25, 0x33, 0x45, 0x67};

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobCommit);
    req->crc = 0;
    req->sessionId = 0x54;
    req->commitDataLen = sizeof(expectedBlob);
    std::memcpy(req->commitData, &expectedBlob[0], sizeof(expectedBlob));

    dataLen = sizeof(struct BmcBlobCommitTx) + sizeof(expectedBlob);

    EXPECT_CALL(mgr,
                commit(req->sessionId,
                       ElementsAreArray(expectedBlob, sizeof(expectedBlob))))
        .WillOnce(Return(true));

    EXPECT_EQ(IPMI_CC_OK, commitBlob(&mgr, request, reply, &dataLen));
}
} // namespace blobs
