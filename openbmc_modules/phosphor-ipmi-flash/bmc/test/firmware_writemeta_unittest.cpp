#include "data_mock.hpp"
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
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

using ::testing::Eq;
using ::testing::Return;

class FirmwareHandlerWriteMetaTest : public FakeLpcFirmwareTest
{};

TEST_F(FirmwareHandlerWriteMetaTest, WriteConfigParametersFailIfOverIPMI)
{
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi, "asdf"));

    std::vector<std::uint8_t> bytes = {0xaa, 0x55};

    EXPECT_FALSE(handler->writeMeta(0, 0, bytes));
}

TEST_F(FirmwareHandlerWriteMetaTest, WriteConfigParametersPassedThrough)
{
    EXPECT_CALL(dataMock, open()).WillOnce(Return(true));
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc, "asdf"));

    std::vector<std::uint8_t> bytes = {0x01, 0x02, 0x03, 0x04};

    EXPECT_CALL(dataMock, writeMeta(Eq(bytes))).WillOnce(Return(true));
    EXPECT_TRUE(handler->writeMeta(0, 0, bytes));
}

} // namespace
} // namespace ipmi_flash
