#include "data_mock.hpp"
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::Eq;
using ::testing::Return;
using ::testing::StrEq;

class FirmwareHandlerCloseTest : public FakeLpcFirmwareTest
{};

TEST_F(FirmwareHandlerCloseTest, CloseSucceedsWithDataHandler)
{
    /* Boring test where you open a blob_id, then verify that when it's closed
     * everything looks right.
     */
    EXPECT_CALL(dataMock, open()).WillOnce(Return(true));
    EXPECT_CALL(*hashImageMock, open(StrEq(hashBlobId))).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc,
        hashBlobId));

    /* The active hash blob_id was added. */
    auto currentBlobs = handler->getBlobIds();
    EXPECT_EQ(3, currentBlobs.size());
    EXPECT_EQ(1, std::count(currentBlobs.begin(), currentBlobs.end(),
                            activeHashBlobId));

    /* Set up close() expectations. */
    EXPECT_CALL(dataMock, close());
    EXPECT_CALL(*hashImageMock, close());
    EXPECT_TRUE(handler->close(0));

    /* Close does not delete the active blob id.  This indicates that there is
     * data queued.
     */
}

TEST_F(FirmwareHandlerCloseTest, CloseSucceedsWithoutDataHandler)
{
    /* Boring test where you open a blob_id using ipmi, so there's no data
     * handler, and it's closed and everything looks right.
     */
    EXPECT_CALL(*hashImageMock, open(StrEq(hashBlobId))).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi,
        hashBlobId));

    /* The active hash blob_id was added. */
    auto currentBlobs = handler->getBlobIds();
    EXPECT_EQ(3, currentBlobs.size());
    EXPECT_EQ(1, std::count(currentBlobs.begin(), currentBlobs.end(),
                            activeHashBlobId));

    /* Set up close() expectations. */
    EXPECT_CALL(*hashImageMock, close());
    EXPECT_TRUE(handler->close(0));
}

} // namespace
} // namespace ipmi_flash
