#include "firmware_handler.hpp"
#include "flags.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

class FirmwareOpenFailTest : public ::testing::TestWithParam<std::uint16_t>
{};

TEST_P(FirmwareOpenFailTest, WithFlags)
{
    std::vector<DataHandlerPack> data = {
        {FirmwareFlags::UpdateFlags::ipmi, nullptr},
        {FirmwareFlags::UpdateFlags::p2a, nullptr},
        {FirmwareFlags::UpdateFlags::lpc, nullptr},
    };

    std::vector<HandlerPack> blobs;
    blobs.push_back(std::move(
        HandlerPack(hashBlobId, std::make_unique<ImageHandlerMock>())));
    blobs.push_back(
        std::move(HandlerPack("asdf", std::make_unique<ImageHandlerMock>())));

    auto handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
        std::move(blobs), data, std::move(CreateActionMap("asdf")));

    EXPECT_FALSE(handler->open(0, GetParam(), "asdf"));
}

const std::vector<std::uint16_t> OpenFailParams{
    /* These first 4 fail because they don't have the "write" flag */
    0b000 << 8,
    0b110 << 8,
    0b101 << 8,
    0b011 << 8,
    /* Next 1 doesn't specify any transport */
    blobs::OpenFlags::write | 0b000 << 8,
    /* Next 3 specify 2 reserved transport bits at the same time. This isn't
     * allowed because older code expects these first 3 bits to be mutually
     * exclusive.
     */
    blobs::OpenFlags::write | 0b110 << 8,
    blobs::OpenFlags::write | 0b101 << 8,
    blobs::OpenFlags::write | 0b011 << 8,
};

INSTANTIATE_TEST_CASE_P(WithFlags, FirmwareOpenFailTest,
                        ::testing::ValuesIn(OpenFailParams));

} // namespace
} // namespace ipmi_flash
