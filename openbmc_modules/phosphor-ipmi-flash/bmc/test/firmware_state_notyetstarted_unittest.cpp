/**
 * The goal of these tests is to verify the behavior of all blob commands given
 * the current state is notYetStarted.  The initial state.
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

using ::testing::Return;
using ::testing::UnorderedElementsAreArray;

class FirmwareHandlerNotYetStartedTest : public IpmiOnlyFirmwareStaticTest
{};

/*
 * There are the following calls (parameters may vary):
 * Note: you cannot have a session yet, so only commands that don't take a
 * session parameter are valid. Once you open() from this state, we will vary
 * you transition out of this state (ensuring the above is true). Technically
 * the firmware handler receives the session number with open(), but the blob
 * manager is providing this normally.
 *
 * canHandleBlob
 * getBlobIds
 * deleteBlob
 * stat
 * open
 *
 * canHandleBlob is just a count check (or something similar) against what is
 * returned by getBlobIds.  It is tested in firmware_canhandle_unittest
 */

/*
 * deleteBlob()
 */
TEST_F(FirmwareHandlerNotYetStartedTest, DeleteBlobInStateReturnsFalse)
{
    auto blobs = handler->getBlobIds();
    for (const auto& b : blobs)
    {
        EXPECT_FALSE(handler->deleteBlob(b));
    }
}

/* canHandleBlob, getBlobIds */
TEST_F(FirmwareHandlerNotYetStartedTest, GetBlobListValidateListContents)
{
    /* By only checking that the hash and static blob ids are present to start
     * with, we're also verifying others aren't.
     */
    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    /* Verify canHandleBlob is reading from the same list (basically) */
    for (const auto& blob : startingBlobs)
    {
        EXPECT_TRUE(handler->canHandleBlob(blob));
    }
}

/* stat(blob_id) */
TEST_F(FirmwareHandlerNotYetStartedTest, StatEachBlobIdVerifyResults)
{
    /* In this original state, calling stat() on the blob ids will return the
     * idle status
     */

    auto blobs = handler->getBlobIds();
    for (const auto& blob : blobs)
    {
        blobs::BlobMeta meta = {};
        EXPECT_TRUE(handler->stat(blob, &meta));
        EXPECT_EQ(expectedIdleMeta, meta);
    }
}

/* open(each blob id) (verifyblobid will no longer be available at this state.
 */
TEST_F(FirmwareHandlerNotYetStartedTest, OpenStaticImageFileVerifyStateChange)
{
    EXPECT_CALL(*imageMock2, open(staticLayoutBlobId)).WillOnce(Return(true));
    EXPECT_CALL(*prepareMockPtr, trigger()).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(session, flags, staticLayoutBlobId));

    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    EXPECT_TRUE(handler->canHandleBlob(activeImageBlobId));
}

TEST_F(FirmwareHandlerNotYetStartedTest, OpenHashFileVerifyStateChange)
{
    EXPECT_CALL(*hashImageMock, open(hashBlobId)).WillOnce(Return(true));
    /* Opening the hash blob id doesn't trigger a preparation, only a firmware
     * blob.
     */
    EXPECT_CALL(*prepareMockPtr, trigger()).Times(0);

    EXPECT_TRUE(handler->open(session, flags, hashBlobId));

    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    EXPECT_TRUE(handler->canHandleBlob(activeHashBlobId));
}

/*
 * expire(session)
 */
TEST_F(FirmwareHandlerNotYetStartedTest, ExpireOnNotYetStartedAbortsProcess)
{
    ASSERT_TRUE(handler->expire(session));

    EXPECT_THAT(handler->getBlobIds(),
                UnorderedElementsAreArray(startingBlobs));

    expectedState(FirmwareBlobHandler::UpdateState::notYetStarted);
}

} // namespace
} // namespace ipmi_flash
