/**
 * The goal of these tests is to verify the behavior of all blob commands given
 * the current state is uploadInProgress.  This state is achieved when an image
 * or hash blob is opened and the handler is expected to receive bytes.
 */
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::ContainerEq;
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
class FirmwareHandlerUploadInProgressTest : public IpmiOnlyFirmwareStaticTest
{};

TEST_F(FirmwareHandlerUploadInProgressTest, GetBlobIdsVerifyOutputActiveImage)
{
    /* Opening the image file will add the active image blob id */
    openToInProgress(staticLayoutBlobId);

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(
                    {staticLayoutBlobId, hashBlobId, activeImageBlobId}));
}

TEST_F(FirmwareHandlerUploadInProgressTest, GetBlobIdsVerifyOutputActiveHash)
{
    /* Opening the image file will add the active image blob id */
    openToInProgress(hashBlobId);

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(
                    {staticLayoutBlobId, hashBlobId, activeHashBlobId}));
}

/*
 * stat(blob)
 */
TEST_F(FirmwareHandlerUploadInProgressTest, StatOnActiveImageReturnsFailure)
{
    /* you cannot call stat() on the active image or the active hash or the
     * verify blob.
     */
    openToInProgress(staticLayoutBlobId);
    ASSERT_TRUE(handler->canHandleBlob(activeImageBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeImageBlobId, &meta));
}

TEST_F(FirmwareHandlerUploadInProgressTest, StatOnActiveHashReturnsFailure)
{
    /* this test is separate from the active image one so that the state doesn't
     * change from close.
     */
    openToInProgress(hashBlobId);
    ASSERT_TRUE(handler->canHandleBlob(activeHashBlobId));

    blobs::BlobMeta meta;
    EXPECT_FALSE(handler->stat(activeHashBlobId, &meta));
}

TEST_F(FirmwareHandlerUploadInProgressTest, StatOnNormalBlobsReturnsSuccess)
{
    /* Calling stat() on the normal blobs (not the active) ones will work and
     * return the same information as in the notYetStarted state.
     */
    openToInProgress(staticLayoutBlobId);

    std::vector<std::string> testBlobs = {staticLayoutBlobId, hashBlobId};
    for (const auto& blob : testBlobs)
    {
        blobs::BlobMeta meta = {};
        EXPECT_TRUE(handler->stat(blob, &meta));
        EXPECT_EQ(expectedIdleMeta, meta);
    }
}

/*
 * stat(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest,
       CallingStatOnActiveImageOrHashSessionReturnsDetails)
{
    /* This test will verify that the underlying image handler is called with
     * this stat, in addition to the normal information.
     */
    openToInProgress(staticLayoutBlobId);

    EXPECT_CALL(*imageMock2, getSize()).WillOnce(Return(32));

    blobs::BlobMeta meta, expectedMeta = {};
    expectedMeta.size = 32;
    expectedMeta.blobState =
        blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi;
    EXPECT_TRUE(handler->stat(session, &meta));
    EXPECT_EQ(expectedMeta, meta);
}

/*
 * open(blob) - While any blob is open, all other fail.
 *
 * The fullBlobsList is all the blob_ids present if both /flash/image and
 * /flash/hash are opened, and one is left open (so there's no verify blob). if
 * left closed, we'd be in verificationPending, not uploadInProgress.
 */
const std::vector<std::string> fullBlobsList = {
    activeHashBlobId, activeImageBlobId, hashBlobId, staticLayoutBlobId};

TEST_F(FirmwareHandlerUploadInProgressTest, OpeningHashFileWhileImageOpenFails)
{
    /* To be in this state, something must be open (and specifically either an
     * active image (or tarball) or the hash file. Also verifies you can't just
     * re-open the currently open file.
     */
    openToInProgress(staticLayoutBlobId);

    for (const auto& blob : fullBlobsList)
    {
        EXPECT_FALSE(handler->open(2, flags, blob));
    }
}

TEST_F(FirmwareHandlerUploadInProgressTest, OpeningImageFileWhileHashOpenFails)
{
    openToInProgress(hashBlobId);

    for (const auto& blob : fullBlobsList)
    {
        EXPECT_FALSE(handler->open(2, flags, blob));
    }
}

/*
 * close(session) - closing the hash or image will trigger a state transition to
 * verificationPending.
 *
 * NOTE: Re-opening /flash/image will transition back to uploadInProgress, but
 * that is verified in the verificationPending::open tests.
 */
TEST_F(FirmwareHandlerUploadInProgressTest,
       ClosingImageFileTransitionsToVerificationPending)
{
    EXPECT_FALSE(handler->canHandleBlob(verifyBlobId));
    openToInProgress(staticLayoutBlobId);

    handler->close(session);
    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);

    EXPECT_TRUE(handler->canHandleBlob(verifyBlobId));
}

TEST_F(FirmwareHandlerUploadInProgressTest,
       ClosingHashFileTransitionsToVerificationPending)
{
    EXPECT_FALSE(handler->canHandleBlob(verifyBlobId));
    openToInProgress(hashBlobId);

    handler->close(session);
    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);

    EXPECT_FALSE(handler->canHandleBlob(verifyBlobId));
}

/*
 * writemeta(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest,
       WriteMetaAgainstImageReturnsFailureIfNoDataHandler)
{
    /* Calling write/read/writeMeta are uninteresting against the open blob in
     * this case because the blob will just pass the call along.  Whereas
     * calling against the verify or update blob may be more interesting.
     */
    openToInProgress(staticLayoutBlobId);

    /* TODO: Consider adding a test that has a data handler, but that test
     * already exists under the general writeMeta test suite.
     */
    /* Note: with IPMI as the transport there's no data handler, so this should
     * fail nicely. */
    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_FALSE(handler->writeMeta(session, 0, bytes));
}

/*
 * write(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest, WriteToImageReturnsSuccess)
{
    openToInProgress(staticLayoutBlobId);
    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_CALL(*imageMock2, write(0, ContainerEq(bytes)))
        .WillOnce(Return(true));
    EXPECT_TRUE(handler->write(session, 0, bytes));
}

TEST_F(FirmwareHandlerUploadInProgressTest, WriteToHashReturnsSuccess)
{
    openToInProgress(hashBlobId);
    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    EXPECT_CALL(*hashImageMock, write(0, ContainerEq(bytes)))
        .WillOnce(Return(true));
    EXPECT_TRUE(handler->write(session, 0, bytes));
}

/*
 * read(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest, ReadImageFileReturnsFailure)
{
    /* Read is not supported. */
    openToInProgress(staticLayoutBlobId);
    EXPECT_THAT(handler->read(session, 0, 32), IsEmpty());
}

/*
 * commit(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest,
       CommitAgainstImageFileReturnsFailure)
{
    /* Commit is only valid against specific blobs. */
    openToInProgress(staticLayoutBlobId);
    EXPECT_FALSE(handler->commit(session, {}));
}

TEST_F(FirmwareHandlerUploadInProgressTest, CommitAgainstHashFileReturnsFailure)
{
    openToInProgress(hashBlobId);
    EXPECT_FALSE(handler->commit(session, {}));
}

/*
 * deleteBlob(blob)
 */
TEST_F(FirmwareHandlerUploadInProgressTest, DeleteBlobReturnsFalse)
{
    /* Try deleting all blobs, it doesn't really matter which though because you
     * cannot close out an open session, therefore you must fail to delete
     * anything unless everything is closed.
     */
    openToInProgress(staticLayoutBlobId);
    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerUploadInProgressTest, ExpireAbortsProcess)
{
    openToInProgress(staticLayoutBlobId);

    ASSERT_TRUE(handler->expire(session));
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));
    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

} // namespace
} // namespace ipmi_flash
