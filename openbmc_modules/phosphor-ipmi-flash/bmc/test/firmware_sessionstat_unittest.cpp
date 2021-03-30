#include "data_mock.hpp"
#include "firmware_handler.hpp"
#include "firmware_unittest.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
using ::testing::Eq;
using ::testing::Return;

class FirmwareSessionStateTestIpmiOnly : public IpmiOnlyFirmwareTest
{};

class FirmwareSessionStateTestLpc : public FakeLpcFirmwareTest
{};

TEST_F(FirmwareSessionStateTestIpmiOnly, DataTypeIpmiNoMetadata)
{
    /* Verifying running stat if the type of data session is IPMI returns no
     * metadata.
     */
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi, "asdf"));

    int size = 512;
    EXPECT_CALL(*imageMock, getSize()).WillOnce(Return(size));

    blobs::BlobMeta meta;
    EXPECT_TRUE(handler->stat(0, &meta));
    EXPECT_EQ(meta.blobState,
              blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi);
    EXPECT_EQ(meta.size, size);
    EXPECT_EQ(meta.metadata.size(), 0);
}

TEST_F(FirmwareSessionStateTestLpc, DataTypeP2AReturnsMetadata)
{
    /* Really any type that isn't IPMI can return metadata, but we only expect
     * P2A to for now.  Later, LPC may have reason to provide data, and can by
     * simply implementing read().
     */
    EXPECT_CALL(dataMock, open()).WillOnce(Return(true));
    EXPECT_CALL(*imageMock, open("asdf")).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(
        0, blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc, "asdf"));

    int size = 512;
    EXPECT_CALL(*imageMock, getSize()).WillOnce(Return(size));
    std::vector<std::uint8_t> mBytes = {0x01, 0x02};
    EXPECT_CALL(dataMock, readMeta()).WillOnce(Return(mBytes));

    blobs::BlobMeta meta;
    EXPECT_TRUE(handler->stat(0, &meta));
    EXPECT_EQ(meta.blobState,
              blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::lpc);
    EXPECT_EQ(meta.size, size);
    EXPECT_EQ(meta.metadata.size(), mBytes.size());
    EXPECT_EQ(meta.metadata[0], mBytes[0]);
    EXPECT_EQ(meta.metadata[1], mBytes[1]);
}

} // namespace ipmi_flash
