#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobStatTest, InvalidRequestLengthReturnsFailure)
{
    // There is a minimum blobId length of one character, this test verifies
    // we check that.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobStatTx*>(request);
    std::string blobId = "abc";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat);
    req->crc = 0;
    // length() doesn't include the nul-terminator.
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobStatTx) + blobId.length();

    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              statBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobStatTest, RequestRejectedReturnsFailure)
{
    // The blobId is rejected for any reason.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobStatTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat);
    req->crc = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobStatTx) + blobId.length() + 1;

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(_)))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              statBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobStatTest, RequestSucceedsNoMetadata)
{
    // Stat request succeeeds but there were no metadata bytes.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobStatTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat);
    req->crc = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobStatTx) + blobId.length() + 1;

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = 0x01;
    rep.size = 0x100;
    rep.metadataLen = 0x00;

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](const std::string& path, BlobMeta* meta) {
            meta->blobState = rep.blobState;
            meta->size = rep.size;
            return true;
        }));

    EXPECT_EQ(IPMI_CC_OK, statBlob(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
}

TEST(BlobStatTest, RequestSucceedsWithMetadata)
{
    // Stat request succeeds and there were metadata bytes.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobStatTx*>(request);
    std::string blobId = "a";

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobStat);
    req->crc = 0;
    // length() doesn't include the nul-terminator, request buff is initialized
    // to 0s
    std::memcpy(req->blobId, blobId.c_str(), blobId.length());

    dataLen = sizeof(struct BmcBlobStatTx) + blobId.length() + 1;

    BlobMeta lmeta;
    lmeta.blobState = 0x01;
    lmeta.size = 0x100;
    lmeta.metadata.push_back(0x01);
    lmeta.metadata.push_back(0x02);
    lmeta.metadata.push_back(0x03);
    lmeta.metadata.push_back(0x04);

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = lmeta.blobState;
    rep.size = lmeta.size;
    rep.metadataLen = lmeta.metadata.size();

    EXPECT_CALL(mgr, stat(Matcher<const std::string&>(StrEq(blobId)),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](const std::string& path, BlobMeta* meta) {
            (*meta) = lmeta;
            return true;
        }));

    EXPECT_EQ(IPMI_CC_OK, statBlob(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep) + lmeta.metadata.size(), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
    EXPECT_EQ(0, std::memcmp(reply + sizeof(rep), lmeta.metadata.data(),
                             lmeta.metadata.size()));
}
} // namespace blobs
