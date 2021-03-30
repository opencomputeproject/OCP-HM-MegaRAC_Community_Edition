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

TEST(BlobReadTest, ManagerReturnsNoData)
{
    // Verify that if no data is returned the IPMI command reply has no
    // payload.  The manager, in all failures, will just return 0 bytes.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobReadTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobRead);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;
    req->requestedSize = 0x10;

    dataLen = sizeof(struct BmcBlobReadTx);

    std::vector<uint8_t> data;

    EXPECT_CALL(mgr, read(req->sessionId, req->offset, req->requestedSize))
        .WillOnce(Return(data));

    EXPECT_EQ(IPMI_CC_OK, readBlob(&mgr, request, reply, &dataLen));
    EXPECT_EQ(sizeof(struct BmcBlobReadRx), dataLen);
}

TEST(BlobReadTest, ManagerReturnsData)
{
    // Verify that if data is returned, it's placed in the expected location.

    ManagerMock mgr;
    size_t dataLen;
    uint8_t request[MAX_IPMI_BUFFER] = {0};
    uint8_t reply[MAX_IPMI_BUFFER] = {0};
    auto req = reinterpret_cast<struct BmcBlobReadTx*>(request);

    req->cmd = static_cast<std::uint8_t>(BlobOEMCommands::bmcBlobRead);
    req->crc = 0;
    req->sessionId = 0x54;
    req->offset = 0x100;
    req->requestedSize = 0x10;

    dataLen = sizeof(struct BmcBlobReadTx);

    std::vector<uint8_t> data = {0x02, 0x03, 0x05, 0x06};

    EXPECT_CALL(mgr, read(req->sessionId, req->offset, req->requestedSize))
        .WillOnce(Return(data));

    EXPECT_EQ(IPMI_CC_OK, readBlob(&mgr, request, reply, &dataLen));
    EXPECT_EQ(sizeof(struct BmcBlobReadRx) + data.size(), dataLen);
    EXPECT_EQ(0, std::memcmp(&reply[sizeof(struct BmcBlobReadRx)], data.data(),
                             data.size()));
}

/* TODO(venture): We need a test that handles other checks such as if the size
 * requested won't fit into a packet response.
 */
} // namespace blobs
