#include "blob_mock.hpp"
#include "manager.hpp"

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::_;
using ::testing::Return;

TEST(ManagerDeleteTest, FileIsOpenReturnsFailure)
{
    // The blob manager maintains a naive list of open files and will
    // return failure if you try to delete an open file.

    // Open the file.
    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));

    // Try to delete the file.
    EXPECT_FALSE(mgr.deleteBlob(path));
}

TEST(ManagerDeleteTest, FileHasNoHandler)
{
    // The blob manager cannot find any handler.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(false));

    // Try to delete the file.
    EXPECT_FALSE(mgr.deleteBlob(path));
}

TEST(ManagerDeleteTest, FileIsNotOpenButHandlerDeleteFails)
{
    // The Blob manager finds the handler but the handler returns failure
    // on delete.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, deleteBlob(path)).WillOnce(Return(false));

    // Try to delete the file.
    EXPECT_FALSE(mgr.deleteBlob(path));
}

TEST(ManagerDeleteTest, FileIsNotOpenAndHandlerSucceeds)
{
    // The Blob manager finds the handler and the handler returns success.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, deleteBlob(path)).WillOnce(Return(true));

    // Try to delete the file.
    EXPECT_TRUE(mgr.deleteBlob(path));
}

TEST(ManagerDeleteTest, DeleteWorksAfterOpenClose)
{
    // The Blob manager is able to decrement the ref count and delete.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, close(_)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, deleteBlob(path)).WillOnce(Return(true));

    EXPECT_TRUE(mgr.open(flags, path, &sess));
    EXPECT_TRUE(mgr.close(sess));

    // Try to delete the file.
    EXPECT_TRUE(mgr.deleteBlob(path));
}
} // namespace blobs
