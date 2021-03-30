#include "blob_mock.hpp"
#include "manager.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerCommitTest, CommitNoSessionReturnsFalse)
{
    // Calling Commit on a session that doesn't exist should return false.

    BlobManager mgr;
    uint16_t sess = 1;
    std::vector<uint8_t> data;

    EXPECT_FALSE(mgr.commit(sess, data));
}

TEST(ManagerCommitTest, CommitSessionFoundButHandlerReturnsFalse)
{
    // The handler was found but it returned failure.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::write, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    std::vector<uint8_t> data;
    EXPECT_CALL(*m1ptr, commit(sess, data)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.commit(sess, data));
}

TEST(ManagerCommitTest, CommitSessionFoundAndHandlerReturnsSuccess)
{
    // The handler was found and returned success.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::write, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    std::vector<uint8_t> data;
    EXPECT_CALL(*m1ptr, commit(sess, data)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.commit(sess, data));
}
} // namespace blobs
