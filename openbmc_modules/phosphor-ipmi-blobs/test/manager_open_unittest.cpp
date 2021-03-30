#include "blob_mock.hpp"
#include "manager.hpp"

#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerOpenTest, OpenButNoHandler)
{
    // No handler claims to be able to open the file.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(false));
    EXPECT_FALSE(mgr.open(flags, path, &sess));
}

TEST(ManagerOpenTest, OpenButHandlerFailsOpen)
{
    // The handler is found but Open fails.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(false));
    EXPECT_FALSE(mgr.open(flags, path, &sess));
}

TEST(ManagerOpenTest, OpenFailsMustSupplyAtLeastReadOrWriteFlag)
{
    // One must supply either read or write in the flags for the session to
    // open.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = 0, sess;
    std::string path = "/asdf/asdf";

    /* It checks if someone can handle the blob before it checks the flags. */
    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));

    EXPECT_FALSE(mgr.open(flags, path, &sess));
}

TEST(ManagerOpenTest, OpenSucceeds)
{
    // The handler is found and Open succeeds.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    // TODO(venture): Need a way to verify the session is associated with it,
    // maybe just call Read() or SessionStat()
}
} // namespace blobs
