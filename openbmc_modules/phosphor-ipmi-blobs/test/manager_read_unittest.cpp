#include "blob_mock.hpp"
#include "manager.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerReadTest, ReadNoSessionReturnsFalse)
{
    // Calling Read on a session that doesn't exist should return false.

    BlobManager mgr;
    uint16_t sess = 1;
    uint32_t ofs = 0x54;
    uint32_t requested = 0x100;

    std::vector<uint8_t> result = mgr.read(sess, ofs, requested);
    EXPECT_EQ(0, result.size());
}

TEST(ManagerReadTest, ReadFromWriteOnlyFails)
{
    // The session manager will not route a Read call to a blob if the session
    // was opened as write-only.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t sess = 1;
    uint32_t ofs = 0x54;
    uint32_t requested = 0x100;
    uint16_t flags = OpenFlags::write;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    std::vector<uint8_t> result = mgr.read(sess, ofs, requested);
    EXPECT_EQ(0, result.size());
}

TEST(ManagerReadTest, ReadFromHandlerReturnsData)
{
    // There is no logic in this as it's just as a pass-thru command, however
    // we want to verify this behavior doesn't change.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t sess = 1;
    uint32_t ofs = 0x54;
    uint32_t requested = 0x10;
    uint16_t flags = OpenFlags::read;
    std::string path = "/asdf/asdf";
    std::vector<uint8_t> data = {0x12, 0x14, 0x15, 0x16};

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    EXPECT_CALL(*m1ptr, read(sess, ofs, requested)).WillOnce(Return(data));

    std::vector<uint8_t> result = mgr.read(sess, ofs, requested);
    EXPECT_EQ(data.size(), result.size());
    EXPECT_EQ(result, data);
}

TEST(ManagerReadTest, ReadTooManyBytesTruncates)
{
    // For now, the hard-coded maximum transfer size is 64 bytes on read
    // commands, due to a hard-coded buffer in ipmid among other future
    // challenges.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t sess = 1;
    uint32_t ofs = 0x54;
    uint32_t requested = 0x100;
    uint16_t flags = OpenFlags::read;
    std::string path = "/asdf/asdf";
    std::vector<uint8_t> data = {0x12, 0x14, 0x15, 0x16};

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    EXPECT_CALL(*m1ptr, read(sess, ofs, maximumReadSize))
        .WillOnce(Return(data));

    std::vector<uint8_t> result = mgr.read(sess, ofs, requested);
    EXPECT_EQ(data.size(), result.size());
    EXPECT_EQ(result, data);
}

} // namespace blobs
