/**
 * The goal of these tests is to verify the behavior of all blob commands given
 * the current state is verificationCompleted.  This state is achieved as a out
 * of verificationStarted.
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
 *
 * Like the state verificationStarted, there is a file open in
 * verificationCompleted.  This state is transitioned to after a stat() command
 * indicates a successful verification.
 */

class FirmwareHandlerVerificationCompletedTest :
    public IpmiOnlyFirmwareStaticTest
{};

/*
 * deleteBlob(blob)
 */
TEST_F(FirmwareHandlerVerificationCompletedTest, DeleteBlobReturnsFalse)
{
    /* Try deleting all blobs, it doesn't really matter which though because you
     * cannot close out an open session, therefore you must fail to delete
     * anything unless everything is closed.
     */
    getToVerificationCompleted(ActionStatus::success);
    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/*
 * canHandleBlob
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       OnVerificationCompleteSuccessUpdateBlobIdNotPresent)
{
    /* the uploadBlobId is only added on close() of the verifyBlobId.  This is a
     * consistent behavior with verifyBlobId only added when closing the image
     * or hash.
     */
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_FALSE(handler->canHandleBlob(updateBlobId));
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       OnVerificationCompleteFailureUpdateBlobIdNotPresent)
{
    getToVerificationCompleted(ActionStatus::failed);
    EXPECT_FALSE(handler->canHandleBlob(updateBlobId));
}

/*
 * getBlobIds
 */
TEST_F(FirmwareHandlerVerificationCompletedTest, GetBlobIdsReturnsExpectedList)
{
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_THAT(
        handler->getBlobIds(),
        UnorderedElementsAreArray(
            {verifyBlobId, hashBlobId, activeImageBlobId, staticLayoutBlobId}));
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       StatOnActiveImageReturnsFailure)
{
    getToVerificationCompleted(ActionStatus::success);
    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       VerifyActiveHashIdMissingInThisCase)
{
    /* The path taken to get to this state never opened the hash blob Id, which
     * is fine.  But let's verify it behaved as intended.
     */
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_FALSE(handler->canHandleBlob(activeHashBlobId));
}

/* TODO: Add sufficient warning that you can get to verificationCompleted
 * without ever opening the image blob id (or the tarball one).
 *
 * Although in this case, it's expected that any verification triggered would
 * certainly fail.  So, although it's possible, it's uninteresting.
 */

TEST_F(FirmwareHandlerVerificationCompletedTest, StatOnVerifyBlobReturnsFailure)
{
    getToVerificationCompleted(ActionStatus::success);
    ASSERT_TRUE(handler->canHandleBlob(verifyBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(verifyBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       StatOnNormalBlobsReturnsSuccess)
{
    getToVerificationCompleted(ActionStatus::success);

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
 * stat(session) - the verify blobid is open in this state, so stat on that once
 * completed should have no effect.
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       SessionStatOnVerifyAfterSuccessDoesNothing)
{
    /* Every time you stat() once it's triggered, it checks the state again
     * until it's completed.
     */
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_CALL(*verifyMockPtr, status()).Times(0);

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committed;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::success));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::verificationCompleted);
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       SessionStatOnVerifyAfterFailureDoesNothing)
{
    getToVerificationCompleted(ActionStatus::failed);
    EXPECT_CALL(*verifyMockPtr, status()).Times(0);

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::commit_error;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::failed));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::verificationCompleted);
}

/*
 * open(blob) - all open should fail
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       OpeningAnyBlobAvailableFailsAfterSuccess)
{
    getToVerificationCompleted(ActionStatus::success);

    auto blobs = handler->getBlobIds();
    for (const auto& blob : blobs)
    {
        EXPECT_FALSE(handler->open(session + 1, flags, blob));
    }
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       OpeningAnyBlobAvailableFailsAfterFailure)
{
    getToVerificationCompleted(ActionStatus::failed);

    auto blobs = handler->getBlobIds();
    for (const auto& blob : blobs)
    {
        EXPECT_FALSE(handler->open(session + 1, flags, blob));
    }
}

/*
 * writemeta(session) - write meta should fail.
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       WriteMetaToVerifyBlobReturnsFailure)
{
    getToVerificationCompleted(ActionStatus::success);

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->writeMeta(session, 0, bytes));
}

/*
 * write(session) - write should fail.
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       WriteToVerifyBlobReturnsFailure)
{
    getToVerificationCompleted(ActionStatus::success);

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->write(session, 0, bytes));
}

/*
 * read(session) - read returns empty.
 */
TEST_F(FirmwareHandlerVerificationCompletedTest, ReadOfVerifyBlobReturnsEmpty)
{
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_THAT(handler->read(session, 0, 1), IsEmpty());
}

/*
 * commit(session) - returns failure
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       CommitOnVerifyBlobAfterSuccessReturnsFailure)
{
    /* If you've started this'll return success, but if it's finished, it won't
     * let you try-again.
     */
    getToVerificationCompleted(ActionStatus::success);
    EXPECT_CALL(*verifyMockPtr, trigger()).Times(0);

    EXPECT_FALSE(handler->commit(session, {}));
}

TEST_F(FirmwareHandlerVerificationCompletedTest,
       CommitOnVerifyBlobAfterFailureReturnsFailure)
{
    getToVerificationCompleted(ActionStatus::failed);
    EXPECT_CALL(*verifyMockPtr, trigger()).Times(0);

    EXPECT_FALSE(handler->commit(session, {}));
}

/*
 * close(session) - close on the verify blobid:
 *   1. if successful adds update blob id, changes state to UpdatePending
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       CloseAfterSuccessChangesStateAddsUpdateBlob)
{
    getToVerificationCompleted(ActionStatus::success);
    ASSERT_FALSE(handler->canHandleBlob(updateBlobId));

    handler->close(session);
    EXPECT_TRUE(handler->canHandleBlob(updateBlobId));
    expectedState(FirmwareBlobHandler::UpdateState::updatePending);
}

/*
 * close(session) - close on the verify blobid:
 *   2. if unsuccessful it aborts.
 */
TEST_F(FirmwareHandlerVerificationCompletedTest, CloseAfterFailureAborts)
{
    getToVerificationCompleted(ActionStatus::failed);
    ASSERT_FALSE(handler->canHandleBlob(updateBlobId));

    handler->close(session);
    ASSERT_FALSE(handler->canHandleBlob(updateBlobId));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerVerificationCompletedTest,
       ExpireAfterVerificationCompletedAborts)
{
    getToVerificationCompleted(ActionStatus::failed);

    ASSERT_TRUE(handler->expire(session));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));
}

} // namespace
} // namespace ipmi_flash
