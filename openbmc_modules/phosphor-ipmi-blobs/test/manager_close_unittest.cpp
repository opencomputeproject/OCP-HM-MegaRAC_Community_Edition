#include "blob_mock.hpp"
#include "manager.hpp"

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerCloseTest, CloseNoSessionReturnsFalse)
{
    // Calling Close on a session that doesn't exist should return false.

    BlobManager mgr;
    uint16_t sess = 1;

    EXPECT_FALSE(mgr.close(sess));
}

TEST(ManagerCloseTest, CloseSessionFoundButHandlerReturnsFalse)
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

    EXPECT_CALL(*m1ptr, close(sess)).WillOnce(Return(false));

    EXPECT_FALSE(mgr.close(sess));

    // TODO(venture): The session wasn't closed, need to verify.  Could call
    // public GetHandler method.
}

TEST(ManagerCloseTest, CloseSessionFoundAndHandlerReturnsSuccess)
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

    EXPECT_CALL(*m1ptr, close(sess)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.close(sess));
}
} // namespace blobs
