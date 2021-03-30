/* The goal of these tests is to verify the behavior of all blob commands given
 * the current state is UpdateCompleted.  This state is achieved as an exit from
 * updateStarted.
 *
 * This can be reached with success or failure from an update, and is reached
 * via a stat() call from updatedStarted.
 */
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "status.hpp"
#include "util.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAreArray;

/*
 * There are the following calls (parameters may vary):
 * canHandleBlob(blob)
 * getBlobIds
 * deleteBlob(blob)
 * stat(blob)
 * stat(session)
 * open(blob)
 * close(session)
 * writemeta(session)
 * write(session)
 * read(session)
 * commit(session)
 */

class FirmwareHandlerUpdateCompletedTest : public IpmiOnlyFirmwareStaticTest
{};

/*
 * open(blob)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest,
       AttemptToOpenFilesReturnsFailureAfterSuccess)
{
    /* In state updateCompleted a file is open, which means no others can be. */
    getToUpdateCompleted(ActionStatus::success);

    auto blobsToOpen = handler->getBlobIds();
    for (const auto& blob : blobsToOpen)
    {
        EXPECT_FALSE(handler->open(session + 1, flags, blob));
    }
}

/*
 * stat(session)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest,
       CallingStatSessionAfterCompletedSuccessReturnsStateWithoutRechecking)
{
    getToUpdateCompleted(ActionStatus::success);
    EXPECT_CALL(*updateMockPtr, status()).Times(0);

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committed;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::success));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::updateCompleted);
}

TEST_F(FirmwareHandlerUpdateCompletedTest,
       CallingStatSessionAfterCompletedFailureReturnsStateWithoutRechecking)
{
    getToUpdateCompleted(ActionStatus::failed);
    EXPECT_CALL(*updateMockPtr, status()).Times(0);

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::commit_error;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::failed));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::updateCompleted);
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, StatOnActiveImageReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::success);

    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerUpdateCompletedTest, StatOnUpdateBlobReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::success);

    ASSERT_TRUE(handler->canHandleBlob(updateBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(updateBlobId, &meta));
}

TEST_F(FirmwareHandlerUpdateCompletedTest, StatOnNormalBlobsReturnsSuccess)
{
    getToUpdateCompleted(ActionStatus::success);

    std::vector<std::string> testBlobs = {staticLayoutBlobId, hashBlobId};
    for (const auto& blob : testBlobs)
    {
        ASSERT_TRUE(handler->canHandleBlob(blob));

        blobs::BlobMeta meta = {};
        EXPECT_TRUE(handler->stat(blob, &meta));
        EXPECT_EQ(expectedIdleMeta, meta);
    }
}

/*
 * writemeta(session)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, WriteMetaToUpdateBlobReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::success);

    EXPECT_FALSE(handler->writeMeta(session, 0, {0x01}));
}

/*
 * write(session)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, WriteToUpdateBlobReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::success);

    EXPECT_FALSE(handler->write(session, 0, {0x01}));
}

/*
 * commit(session) - returns failure
 */
TEST_F(FirmwareHandlerUpdateCompletedTest,
       CommitOnUpdateBlobAfterSuccessReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::success);

    EXPECT_CALL(*updateMockPtr, trigger()).Times(0);
    EXPECT_FALSE(handler->commit(session, {}));
}

TEST_F(FirmwareHandlerUpdateCompletedTest,
       CommitOnUpdateBlobAfterFailureReturnsFailure)
{
    getToUpdateCompleted(ActionStatus::failed);

    EXPECT_CALL(*updateMockPtr, trigger()).Times(0);
    EXPECT_FALSE(handler->commit(session, {}));
}

/*
 * read(session) - nothing to read here.
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, ReadingFromUpdateBlobReturnsNothing)
{
    getToUpdateCompleted(ActionStatus::success);

    EXPECT_THAT(handler->read(session, 0, 1), IsEmpty());
}

/*
 * getBlobIds
 * canHandleBlob(blob)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, GetBlobListProvidesExpectedBlobs)
{
    getToUpdateCompleted(ActionStatus::success);

    EXPECT_THAT(
        handler->getBlobIds(),
        UnorderedElementsAreArray(
            {updateBlobId, hashBlobId, activeImageBlobId, staticLayoutBlobId}));
}

/*
 * close(session) - closes everything out and returns to state not-yet-started.
 * It doesn't go and clean out any update artifacts that may be present on the
 * system.  It's up to the update implementation to deal with this before
 * marking complete.
 */
TEST_F(FirmwareHandlerUpdateCompletedTest,
       ClosingOnUpdateBlobIdAfterSuccessReturnsToNotYetStartedAndCleansBlobList)
{
    getToUpdateCompleted(ActionStatus::success);

    handler->close(session);
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));
}

TEST_F(FirmwareHandlerUpdateCompletedTest,
       ClosingOnUpdateBlobIdAfterFailureReturnsToNotYetStartedAndCleansBlobList)
{
    getToUpdateCompleted(ActionStatus::failed);

    handler->close(session);
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));
}

/*
 * deleteBlob(blob)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, DeleteBlobReturnsFalse)
{
    /* Try deleting all blobs, it doesn't really matter which though because you
     * cannot close out an open session, therefore you must fail to delete
     * anything unless everything is closed.
     */
    getToUpdateCompleted(ActionStatus::success);

    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerUpdateCompletedTest, ExpireOnUpdateCompletedAbortsProcess)
{
    getToUpdateCompleted(ActionStatus::success);

    ASSERT_TRUE(handler->expire(session));
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

} // namespace
} // namespace ipmi_flash
