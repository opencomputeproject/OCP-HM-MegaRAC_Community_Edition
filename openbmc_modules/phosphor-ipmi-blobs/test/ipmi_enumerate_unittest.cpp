#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobEnumerateTest, VerifyIfRequestByIdInvalidReturnsFailure)
{
    // This tests to verify that if the index is invalid, it'll return failure.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobEnumerateTx req;
    uint8_t* request = reinterpret_cast<uint8_t*>(&req);

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobEnumerate);
    req.blobIdx = 0;
    dataLen = sizeof(struct BmcBlobEnumerateTx);

    EXPECT_CALL(mgr, getBlobId(req.blobIdx)).WillOnce(Return(""));

    EXPECT_EQ(IPMI_CC_INVALID_FIELD_REQUEST,
              enumerateBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobEnumerateTest, BoringRequestByIdAndReceive)
{
    // This tests that if an index into the blob_id cache is valid, the command
    // will return the blobId.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    struct BmcBlobEnumerateTx req;
    struct BmcBlobEnumerateRx* rep;
    uint8_t* request = reinterpret_cast<uint8_t*>(&req);
    std::string blobId = "/asdf";

    req.cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobEnumerate);
    req.blobIdx = 0;
    dataLen = sizeof(struct BmcBlobEnumerateTx);

    EXPECT_CALL(mgr, getBlobId(req.blobIdx)).WillOnce(Return(blobId));

    EXPECT_EQ(IPMI_CC_OK, enumerateBlob(&mgr, request, reply, &dataLen));

    // We're expecting this as a response.
    // blobId.length + 1 + sizeof(uint16_t);
    EXPECT_EQ(blobId.length() + 1 + sizeof(uint16_t), dataLen);

    rep = reinterpret_cast<struct BmcBlobEnumerateRx*>(reply);
    EXPECT_EQ(0, std::memcmp(rep->blobId, blobId.c_str(), blobId.length() + 1));
}
} // namespace blobs
