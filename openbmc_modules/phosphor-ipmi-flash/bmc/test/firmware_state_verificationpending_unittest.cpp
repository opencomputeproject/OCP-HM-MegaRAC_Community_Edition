/**
 * The goal of these tests is to verify the behavior of all blob commands given
 * the current state is verificationPending.  This state is achieved as a
 * transition out of uploadInProgress.
 */
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "status.hpp"
#include "util.hpp"

#include <algorithm>
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

class FirmwareHandlerVerificationPendingTest : public IpmiOnlyFirmwareStaticTest
{};

/*
 * getBlobIds
 */
TEST_F(FirmwareHandlerVerificationPendingTest, VerifyBlobIdAvailableInState)
{
    /* Only in the verificationPending state (and later), should the
     * verifyBlobId be present.
     */
    EXPECT_FALSE(handler->canHandleBlob(verifyBlobId));

    getToVerificationPending(staticLayoutBlobId);

    EXPECT_TRUE(handler->canHandleBlob(verifyBlobId));
    EXPECT_TRUE(handler->canHandleBlob(activeImageBlobId));
    EXPECT_FALSE(handler->canHandleBlob(updateBlobId));
}

/*
 * delete(blob)
 */
TEST_F(FirmwareHandlerVerificationPendingTest, DeleteVerifyPendingAbortsProcess)
{
    /* It doesn't matter what blob id is used to delete in the design, so just
     * delete the verify blob id
     */
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    ASSERT_TRUE(handler->canHandleBlob(verifyBlobId));
    EXPECT_TRUE(handler->deleteBlob(verifyBlobId));

    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId};
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

TEST_F(FirmwareHandlerVerificationPendingTest, DeleteActiveImageAbortsProcess)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));
    EXPECT_TRUE(handler->deleteBlob(activeImageBlobId));

    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId};
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

TEST_F(FirmwareHandlerVerificationPendingTest, DeleteStaticLayoutAbortsProcess)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    ASSERT_TRUE(handler->canHandleBlob(staticLayoutBlobId));
    EXPECT_TRUE(handler->deleteBlob(staticLayoutBlobId));

    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId};
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

TEST_F(FirmwareHandlerVerificationPendingTest, DeleteHashAbortsProcess)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    ASSERT_TRUE(handler->canHandleBlob(hashBlobId));
    EXPECT_TRUE(handler->deleteBlob(hashBlobId));

    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId};
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest,
       ExpireVerificationPendingAbortsProcess)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_CALL(*verifyMockPtr, abort()).Times(0);

    EXPECT_TRUE(handler->expire(session));

    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId};
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerVerificationPendingTest, StatOnActiveImageReturnsFailure)
{
    getToVerificationPending(staticLayoutBlobId);
    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationPendingTest, StatOnActiveHashReturnsFailure)
{
    getToVerificationPending(hashBlobId);
    ASSERT_TRUE(handler->canHandleBlob(activeHashBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeHashBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationPendingTest,
       StatOnVerificationBlobReturnsFailure)
{
    getToVerificationPending(staticLayoutBlobId);
    ASSERT_TRUE(handler->canHandleBlob(verifyBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(verifyBlobId, &meta));
}

TEST_F(FirmwareHandlerVerificationPendingTest,
       VerificationBlobNotFoundWithoutStaticDataAsWell)
{
    /* If you only ever open the hash blob id, and never the firmware blob id,
     * the verify blob isn't added.
     */
    getToVerificationPending(hashBlobId);
    EXPECT_FALSE(handler->canHandleBlob(verifyBlobId));
}

TEST_F(FirmwareHandlerVerificationPendingTest, StatOnNormalBlobsReturnsSuccess)
{
    getToVerificationPending(staticLayoutBlobId);

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
 * open(blob)
 */
TEST_F(FirmwareHandlerVerificationPendingTest, OpenVerifyBlobSucceeds)
{
    getToVerificationPending(staticLayoutBlobId);

    /* the session is safe because it was already closed to get to this state.
     */
    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
}

TEST_F(FirmwareHandlerVerificationPendingTest, OpenActiveBlobsFail)
{
    /* Try opening the active blob Id.  This test is equivalent to trying to
     * open the active hash blob id, in that neither are ever allowed.
     */
    getToVerificationPending(staticLayoutBlobId);
    EXPECT_FALSE(handler->open(session, flags, activeImageBlobId));
    EXPECT_FALSE(handler->open(session, flags, activeHashBlobId));
}

TEST_F(FirmwareHandlerVerificationPendingTest,
       OpenImageBlobTransitionsToUploadInProgress)
{
    getToVerificationPending(staticLayoutBlobId);

    /* Verify the active blob for the image is in the list once to start.
     * Note: This is truly tested under the notYetStarted::open() test.
     */
    std::vector<std::string> expectedBlobs = {staticLayoutBlobId, hashBlobId,
                                              verifyBlobId, activeImageBlobId};

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));

    /* Verifies it isn't triggered again. */
    EXPECT_CALL(*prepareMockPtr, trigger()).Times(0);

    EXPECT_CALL(*imageMock2, open(staticLayoutBlobId)).WillOnce(Return(true));
    EXPECT_TRUE(handler->open(session, flags, staticLayoutBlobId));
    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    expectedBlobs.erase(
        std::remove(expectedBlobs.begin(), expectedBlobs.end(), verifyBlobId),
        expectedBlobs.end());

    /* Verify the active blob ID was not added to the list twice and
     * verifyBlobId is removed
     */
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(expectedBlobs));
}

/*
 * close(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest,
       ClosingVerifyBlobWithoutCommitDoesNotChangeState)
{
    /* commit() will change the state, closing post-commit is part of
     * verificationStarted testing.
     */
    getToVerificationPending(staticLayoutBlobId);
    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);

    handler->close(session);
    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);
}

/*
 * commit(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest,
       CommitOnVerifyBlobTriggersVerificationAndStateTransition)
{
    getToVerificationPending(staticLayoutBlobId);
    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
    EXPECT_CALL(*verifyMockPtr, trigger()).WillOnce(Return(true));

    EXPECT_TRUE(handler->commit(session, {}));
    expectedState(FirmwareBlobHandler::UpdateState::verificationStarted);
}

/*
 * stat(session) - in this state, you can only open(verifyBlobId) without
 * changing state.
 */
TEST_F(FirmwareHandlerVerificationPendingTest, StatOnVerifyBlobIdReturnsState)
{
    /* If this is called before commit(), it's still verificationPending, so it
     * just returns the state is other
     */
    getToVerificationPending(staticLayoutBlobId);
    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
    EXPECT_CALL(*verifyMockPtr, trigger()).Times(0);
    EXPECT_CALL(*verifyMockPtr, status()).Times(0);

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 0;
    expectedMeta.blobState = flags;
    expectedMeta.metadata.push_back(
        static_cast<std::uint8_t>(ActionStatus::unknown));

    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
}

/*
 * writemeta(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest, WriteMetaAgainstVerifyFails)
{
    /* The verifyBlobId has no data handler, which means write meta fails. */
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->writeMeta(session, 0, bytes));
}

/*
 * write(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest, WriteAgainstVerifyBlobIdFails)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->write(session, 0, bytes));
}

/*
 * read(session)
 */
TEST_F(FirmwareHandlerVerificationPendingTest,
       ReadAgainstVerifyBlobIdReturnsEmpty)
{
    getToVerificationPending(staticLayoutBlobId);

    EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
    EXPECT_THAT(handler->read(session, 0, 1), IsEmpty());
}

} // namespace
} // namespace ipmi_flash
