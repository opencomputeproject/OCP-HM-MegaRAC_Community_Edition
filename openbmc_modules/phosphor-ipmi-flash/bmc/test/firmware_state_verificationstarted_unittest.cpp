/* The goal of these tests is to verify the behavior of all blob commands given
 * the current state is verificationStarted.  This state is achieved as a out of
 * verificationPending.
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
 * Testing canHandleBlob is uninteresting in this state.  Getting the BlobIDs
 * will inform what canHandleBlob will return.
 */

class FirmwareHandlerVerificationStartedTest : public IpmiOnlyFirmwareStaticTest
{};

/*
 * canHandleBlob(blob)
 * getBlobIds()
 */
TEST_F(FirmwareHandlerVerificationStartedTest, GetBlobIdsReturnsExpectedList)
{
    getToVerificationStarted(staticLayoutBlobId);

    auto blobs = handler->getBlobIds();
    EXPECT_THAT(
        blobs, UnorderedElementsAreArray({activeImageBlobId, staticLayoutBlobId,
                                          hashBlobId, verifyBlobId}));

    for (const auto& blob : blobs)
    {
        EXPECT_TRUE(handler->canHandleBlob(blob));
    }
}

/*
 * stat(session)
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       StatOnVerifyBlobIdAfterCommitChecksStateAndReturnsRunning)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, status())
        .WillOnce(Return(ActionStatus::running));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committing;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::running));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
}

TEST_F(FirmwareHandlerVerificationStartedTest,
       StatOnVerifyBlobIdAfterCommitChecksStateAndReturnsOther)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, status())
        .WillOnce(Return(ActionStatus::unknown));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committing;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::unknown));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
}

TEST_F(FirmwareHandlerVerificationStartedTest,
       StatOnVerifyBlobIdAfterCommitCheckStateAndReturnsFailed)
{
    /* If the returned state from the verification handler is failed, it sets
     * commit_error and transitions to verificationCompleted.
     */
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, status())
        .WillOnce(Return(ActionStatus::failed));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::commit_error;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::failed));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::verificationCompleted);
}

TEST_F(FirmwareHandlerVerificationStartedTest,
       StatOnVerifyBlobIdAfterCommitCheckStateAndReturnsSuccess)
{
    /* If the returned state from the verification handler is success, it sets
     * committed and transitions to verificationCompleted.
     */
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, status())
        .WillOnce(Return(ActionStatus::success));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags | blobs::StateFlags::committed;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::success));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
    expectedState(FirmwareBlobHandler::UpdateState::verificationCompleted);
}

/*
 * deleteBlob(blob)
 */
TEST_F(FirmwareHandlerVerificationStartedTest, DeleteBlobReturnsFalse)
{
    /* Try deleting all blobs, it doesn't really matter which though because you
     * cannot close out an open session, therefore you must fail to delete
     * anything unless everything is closed.
     */
    getToVerificationStarted(staticLayoutBlobId);
    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerVerificationStartedTest, StatOnActiveImageReturnsFailure)
{
    getToVerificationStarted(staticLayoutBlobId);
    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationStartedTest, StatOnActiveHashReturnsFailure)
{
    getToVerificationStartedWitHashBlob();
    ASSERT_TRUE(handler->canHandleBlob(activeHashBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeHashBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationStartedTest, StatOnVerifyBlobReturnsFailure)
{
    /* the verifyBlobId is available starting at verificationPending. */
    getToVerificationStarted(staticLayoutBlobId);
    ASSERT_TRUE(handler->canHandleBlob(verifyBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(verifyBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationStartedTest, StatOnNormalBlobsReturnsSuccess)
{
    getToVerificationStarted(staticLayoutBlobId);

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
TEST_F(FirmwareHandlerVerificationStartedTest,
       WriteMetaOnVerifySessionReturnsFailure)
{
    getToVerificationStarted(staticLayoutBlobId);

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->writeMeta(session, 0, bytes));
}

/*
 * write(session)
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       WriteOnVerifySessionReturnsFailure)
{
    getToVerificationStarted(staticLayoutBlobId);

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->write(session, 0, bytes));
}

/*
 * open(blob) - there is nothing you can open, this state has an open file.
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       AttemptToOpenImageFileReturnsFailure)
{
    /* Attempt to open a file one normally can open, however, as there is
     * already a file open, this will fail.
     */
    getToVerificationStarted(staticLayoutBlobId);

    auto blobsToOpen = handler->getBlobIds();
    for (const auto& blob : blobsToOpen)
    {
        EXPECT_FALSE(handler->open(session + 1, flags, blob));
    }
}

/*
 * read(session)
 */
TEST_F(FirmwareHandlerVerificationStartedTest, ReadOfVerifyBlobReturnsEmpty)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_THAT(handler->read(session, 0, 1), IsEmpty());
}

/*
 * commit(session)
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       CommitOnVerifyDuringVerificationHasNoImpact)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_TRUE(handler->commit(session, {}));
    expectedState(FirmwareBlobHandler::UpdateState::verificationStarted);
}

/*
 * close(session) - close while state if verificationStarted without calling
 * stat first will abort.
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       CloseOnVerifyDuringVerificationAbortsProcess)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, abort()).Times(1);

    EXPECT_TRUE(handler->close(session));

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerVerificationStartedTest,
       ExpireOnSessionDuringVerificationAbortsProcess)
{
    getToVerificationStarted(staticLayoutBlobId);
    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    EXPECT_TRUE(handler->expire(session));

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

} // namespace
} // namespace ipmi_flash
