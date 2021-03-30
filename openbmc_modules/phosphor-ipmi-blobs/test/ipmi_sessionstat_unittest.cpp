#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::Matcher;
using ::testing::NotNull;
using ::testing::Return;

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(BlobSessionStatTest, RequestRejectedByManagerReturnsFailure)
{
    // If the session ID is invalid, the request must fail.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobSessionStatTx*>(request);
    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat);
    req->crc = 0;
    req->sessionId = 0x54;

    dataLen = sizeof(struct BmcBlobSessionStatTx);

    EXPECT_CALL(mgr,
                stat(Matcher<uint16_t>(req->sessionId), Matcher<BlobMeta*>(_)))
        .WillOnce(Return(false));

    EXPECT_EQ(IPMI_CC_UNSPECIFIED_ERROR,
              sessionStatBlob(&mgr, request, reply, &dataLen));
}

TEST(BlobSessionStatTest, RequestSucceedsNoMetadata)
{
    // Stat request succeeeds but there were no metadata bytes.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobSessionStatTx*>(request);
    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat);
    req->crc = 0;
    req->sessionId = 0x54;

    dataLen = sizeof(struct BmcBlobSessionStatTx);

    struct BmcBlobStatRx rep;
    rep.crc = 0x00;
    rep.blobState = 0x01;
    rep.size = 0x100;
    rep.metadataLen = 0x00;

    EXPECT_CALL(mgr, stat(Matcher<uint16_t>(req->sessionId),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](uint16_t session, BlobMeta* meta) {
            meta->blobState = rep.blobState;
            meta->size = rep.size;
            return true;
        }));

    EXPECT_EQ(IPMI_CC_OK, sessionStatBlob(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
}

TEST(BlobSessionStatTest, RequestSucceedsWithMetadata)
{
    // Stat request succeeds and there were metadata bytes.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobSessionStatTx*>(request);
    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobSessionStat);
    req->crc = 0;
    req->sessionId = 0x54;

    dataLen = sizeof(struct BmcBlobSessionStatTx);

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

    EXPECT_CALL(mgr, stat(Matcher<uint16_t>(req->sessionId),
                          Matcher<BlobMeta*>(NotNull())))
        .WillOnce(Invoke([&](uint16_t session, BlobMeta* meta) {
            (*meta) = lmeta;
            return true;
        }));

    EXPECT_EQ(IPMI_CC_OK, sessionStatBlob(&mgr, request, reply, &dataLen));

    EXPECT_EQ(sizeof(rep) + lmeta.metadata.size(), dataLen);
    EXPECT_EQ(0, std::memcmp(reply, &rep, sizeof(rep)));
    EXPECT_EQ(0, std::memcmp(reply + sizeof(rep), lmeta.metadata.data(),
                             lmeta.metadata.size()));
}
} // namespace blobs
