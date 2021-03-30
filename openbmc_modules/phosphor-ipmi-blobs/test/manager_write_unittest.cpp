#include "blob_mock.hpp"
#include "manager.hpp"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

namespace blobs
{

TEST(ManagerWriteTest, WriteNoSessionReturnsFalse)
{
    // Calling Write on a session that doesn't exist should return false.

    BlobManager mgr;
    uint16_t sess = 1;
    uint32_t ofs = 0x54;
    std::vector<uint8_t> data = {0x11, 0x22};

    EXPECT_FALSE(mgr.write(sess, ofs, data));
}

TEST(ManagerWriteTest, WriteSessionFoundButHandlerReturnsFalse)
{
    // The handler was found but it returned failure.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::write, sess;
    std::string path = "/asdf/asdf";
    uint32_t ofs = 0x54;
    std::vector<uint8_t> data = {0x11, 0x22};

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    EXPECT_CALL(*m1ptr, write(sess, ofs, data)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.write(sess, ofs, data));
}

TEST(ManagerWriteTest, WriteFailsBecauseFileOpenedReadOnly)
{
    // The manager will not route a write call to a file opened read-only.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";
    uint32_t ofs = 0x54;
    std::vector<uint8_t> data = {0x11, 0x22};

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    EXPECT_FALSE(mgr.write(sess, ofs, data));
}

TEST(ManagerWriteTest, WriteSessionFoundAndHandlerReturnsSuccess)
{
    // The handler was found and returned success.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::write, sess;
    std::string path = "/asdf/asdf";
    uint32_t ofs = 0x54;
    std::vector<uint8_t> data = {0x11, 0x22};

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    EXPECT_CALL(*m1ptr, write(sess, ofs, data)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.write(sess, ofs, data));
}
} // namespace blobs
