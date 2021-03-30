/* The goal of these tests is to verify the behavior of all blob commands given
 * the current state is updateStarted.  This state is achieved as an exit from
 * updatePending.
 */
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "status.hpp"
#include "util.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::IsEmpty;
using ::testing::Return;
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

class FirmwareHandlerUpdateStartedTest : public IpmiOnlyFirmwareStaticTest
{};

/*
 * open(blob)
 */
TEST_F(FirmwareHandlerUpdateStartedTest, AttemptToOpenFilesReturnsFailure)
{
    /* In state updateStarted a file is open, which means no others can be. */
    getToUpdateStarted();

    auto blobsToOpen = handler->getBlobIds();
    for (const auto& blob : blobsToOpen)
    {
        EXPECT_FALSE(handler->open(session + 1, flags, blob));
    }
}

/* canHandleBlob(blob)
 * getBlobIds
 */
TEST_F(FirmwareHandlerUpdateStartedTest, VerifyListOfBlobs)
{
    getToUpdateStarted();

    EXPECT_THAT(
        handler->getBlobIds(),
        UnorderedElementsAreArray(
            {updateBlobId, hashBlobId, activeImageBlobId, staticLayoutBlobId}));
}

/*
 * deleteBlob(blob)
 */
TEST_F(FirmwareHandlerUpdateStartedTest, DeleteBlobReturnsFalse)
{
    /* Try deleting all blobs, it doesn't really matter which though because you
     * cannot close out an open session, therefore you must fail to delete
     * anything unless everything is closed.
     */
    getToUpdateStarted();
    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerUpdateStartedTest, StatOnActiveImageReturnsFailure)
{
    getToUpdateStarted();

    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerUpdateStartedTest, StatOnUpdateBlobReturnsFailure)
{
    getToUpdateStarted();

    ASSERT_TRUE(handler->canHandleBlob(updateBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(updateBlobId, &meta));
}

TEST_F(FirmwareHandlerUpdateStartedTest, StatOnNormalBlobsReturnsSuccess)
{
    getToUpdateStarted();

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
TEST_F(FirmwareHandlerUpdateStartedTest, WriteMetaToUpdateBlobReturnsFailure)
{
    getToUpdateStarted();
    EXPECT_FALSE(handler->writeMeta(session, 0, {0x01}));
}

/*
 * write(session)
 */
TEST_F(FirmwareHandlerUpdateStartedTest, WriteToUpdateBlobReturnsFailure)
{
    getToUpdateStarted();
    EXPECT_FALSE(handler->write(session, 0, {0x01}));
}

/*
 * read(session)
 */
TEST_F(FirmwareHandlerUpdateStartedTest, ReadFromUpdateBlobReturnsEmpty)
{
    getToUpdateStarted();
    EXPECT_THAT(handler->read(session, 0, 1), IsEmpty());
}

/*
 * commit(session)
 */
TEST_F(FirmwareHandlerUpdateStartedTest,
       CallingCommitShouldReturnTrueAndHaveNoEffect)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, trigger()).Times(0);

    EXPECT_TRUE(handler->commit(session, {}));
    expectedState(FirmwareBlobHandler::UpdateState::updateStarted);
}

/*
 * stat(session) - this will trigger a check, and the state may change.
 */
TEST_F(FirmwareHandlerUpdateStartedTest,
       CallStatChecksActionStatusReturnsRunningDoesNotChangeState)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, status())
        .WillOnce(Return(ActionStatus::running));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committing;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::running));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::updateStarted);
}

TEST_F(FirmwareHandlerUpdateStartedTest,
       CallStatChecksActionStatusReturnsFailedChangesStateToCompleted)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, status())
        .WillOnce(Return(ActionStatus::failed));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::commit_error;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::failed));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::updateCompleted);
}

TEST_F(FirmwareHandlerUpdateStartedTest,
       CallStatChecksActionStatusReturnsSuccessChangesStateToCompleted)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, status())
        .WillOnce(Return(ActionStatus::success));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committed;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::success));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::updateCompleted);
}

/*
 * close(session) - this will abort.
 */
TEST_F(FirmwareHandlerUpdateStartedTest, CloseOnUpdateDuringUpdateAbortsProcess)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, abort()).Times(1);

    EXPECT_TRUE(handler->close(session));

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerUpdateStartedTest,
       ExpireOnUpdateDuringUpdateAbortsProcess)
{
    getToUpdateStarted();
    EXPECT_CALL(*updateMockPtr, abort()).Times(0);

    ASSERT_TRUE(handler->expire(session));

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

} // namespace
} // namespace ipmi_flash
