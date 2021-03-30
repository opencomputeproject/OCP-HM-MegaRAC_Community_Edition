#include "firmware_handler.hpp"
#include "flags.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <memory>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

/* This test ensures the stat() method preserves compatibility with older host
 * tools by reporting that all transports are supported. */
TEST(FirmwareHandlerStatTest, StatOnInactiveBlobIDReturnsAllTransports)
{
    /* Test that the metadata information returned matches expectations for this
     * case.
     *
     * canHandle has already been called at this point, so we don't need to test
     * the input for this function.
     */

    std::vector<HandlerPack> blobs;
    blobs.push_back(std::move(
        HandlerPack(hashBlobId, std::make_unique<ImageHandlerMock>())));
    blobs.push_back(
        std::move(HandlerPack("asdf", std::make_unique<ImageHandlerMock>())));

    std::vector<DataHandlerPack> data = {
        {FirmwareFlags::UpdateFlags::ipmi, nullptr},
    };

    auto handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
        std::move(blobs), data, std::move(CreateActionMap("asdf")));

    blobs::BlobMeta meta;
    EXPECT_TRUE(handler->stat("asdf", &meta));
    /* All transport flags are set */
    EXPECT_EQ(0xff00, meta.blobState);
}

} // namespace
} // namespace ipmi_flash
