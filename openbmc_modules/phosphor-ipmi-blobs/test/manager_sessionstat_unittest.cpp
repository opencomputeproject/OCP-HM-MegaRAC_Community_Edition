#include "blob_mock.hpp"
#include "manager.hpp"

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerSessionStatTest, StatNoSessionReturnsFalse)
{
    // Calling Stat on a session that doesn't exist should return false.

    BlobManager mgr;
    BlobMeta meta;
    uint16_t sess = 1;

    EXPECT_FALSE(mgr.stat(sess, &meta));
}

TEST(ManagerSessionStatTest, StatSessionFoundButHandlerReturnsFalse)
{
    // The handler was found but it returned failure.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    BlobMeta meta;
    EXPECT_CALL(*m1ptr, stat(sess, &meta)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.stat(sess, &meta));
}

TEST(ManagerSessionStatTest, StatSessionFoundAndHandlerReturnsSuccess)
{
    // The handler was found and returned success.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    BlobMeta meta;
    EXPECT_CALL(*m1ptr, stat(sess, &meta)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.stat(sess, &meta));
}
} // namespace blobs
