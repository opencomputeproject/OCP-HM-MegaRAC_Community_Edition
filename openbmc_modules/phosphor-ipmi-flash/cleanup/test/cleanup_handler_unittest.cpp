#include "cleanup.hpp"
#include "filesystem_mock.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::Return;
using ::testing::UnorderedElementsAreArray;

class CleanupHandlerTest : public ::testing::Test
{
  protected:
    CleanupHandlerTest() : mock(std::make_unique<FileSystemMock>())
    {
        mock_ptr = mock.get();
        handler = std::make_unique<FileCleanupHandler>(cleanupBlobId, blobs,
                                                       std::move(mock));
    }

    std::vector<std::string> blobs = {"abcd", "efgh"};
    std::unique_ptr<FileSystemMock> mock;
    FileSystemMock* mock_ptr;
    std::unique_ptr<FileCleanupHandler> handler;
};

TEST_F(CleanupHandlerTest, GetBlobListReturnsExpectedList)
{
    EXPECT_TRUE(handler->canHandleBlob(cleanupBlobId));
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray({cleanupBlobId}));
}

TEST_F(CleanupHandlerTest, CommitShouldDeleteFiles)
{
    EXPECT_CALL(*mock_ptr, remove("abcd")).WillOnce(Return());
    EXPECT_CALL(*mock_ptr, remove("efgh")).WillOnce(Return());

    EXPECT_TRUE(handler->commit(1, {}));
}

} // namespace
} // namespace ipmi_flash
