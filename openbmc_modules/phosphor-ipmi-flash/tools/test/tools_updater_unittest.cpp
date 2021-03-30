#include "data_interface_mock.hpp"
#include "flags.hpp"
#include "status.hpp"
#include "tool_errors.hpp"
#include "updater.hpp"
#include "updater_mock.hpp"
#include "util.hpp"

#include <ipmiblob/test/blob_interface_mock.hpp>

#include <string>

#include <gtest/gtest.h>

namespace host_tool
{

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;
using ::testing::TypedEq;

class UpdateHandlerTest : public ::testing::Test
{
  protected:
    const std::uint16_t session = 0xbeef;

    DataInterfaceMock handlerMock;
    ipmiblob::BlobInterfaceMock blobMock;
    UpdateHandler updater{&blobMock, &handlerMock};
};

TEST_F(UpdateHandlerTest, CheckAvailableSuccess)
{
    EXPECT_CALL(blobMock, getBlobList())
        .WillOnce(
            Return(std::vector<std::string>({ipmi_flash::staticLayoutBlobId})));

    EXPECT_TRUE(updater.checkAvailable(ipmi_flash::staticLayoutBlobId));
}

TEST_F(UpdateHandlerTest, SendFileSuccess)
{
    /* Call sendFile to verify it does what we expect. */
    std::string firmwareImage = "image.bin";

    std::uint16_t supported =
        static_cast<std::uint16_t>(
            ipmi_flash::FirmwareFlags::UpdateFlags::lpc) |
        static_cast<std::uint16_t>(
            ipmi_flash::FirmwareFlags::UpdateFlags::openWrite);

    EXPECT_CALL(handlerMock, supportedType())
        .WillOnce(Return(ipmi_flash::FirmwareFlags::UpdateFlags::lpc));

    EXPECT_CALL(blobMock, openBlob(ipmi_flash::staticLayoutBlobId, supported))
        .WillOnce(Return(session));

    EXPECT_CALL(handlerMock, sendContents(firmwareImage, session))
        .WillOnce(Return(true));

    EXPECT_CALL(blobMock, closeBlob(session)).Times(1);

    updater.sendFile(ipmi_flash::staticLayoutBlobId, firmwareImage);
}

TEST_F(UpdateHandlerTest, VerifyFileHandleReturnsTrueOnSuccess)
{
    EXPECT_CALL(blobMock, openBlob(ipmi_flash::verifyBlobId, _))
        .WillOnce(Return(session));
    EXPECT_CALL(blobMock, commit(session, _)).WillOnce(Return());
    ipmiblob::StatResponse verificationResponse = {};
    /* the other details of the response are ignored, and should be. */
    verificationResponse.metadata.push_back(
        static_cast<std::uint8_t>(ipmi_flash::ActionStatus::success));

    EXPECT_CALL(blobMock, getStat(TypedEq<std::uint16_t>(session)))
        .WillOnce(Return(verificationResponse));
    EXPECT_CALL(blobMock, closeBlob(session)).WillOnce(Return());

    EXPECT_TRUE(updater.verifyFile(ipmi_flash::verifyBlobId, false));
}

class UpdaterTest : public ::testing::Test
{
  protected:
    ipmiblob::BlobInterfaceMock blobMock;
    std::uint16_t session = 0xbeef;
    bool defaultIgnore = false;
};

TEST_F(UpdaterTest, UpdateMainReturnsSuccessIfAllSuccess)
{
    const std::string image = "image.bin";
    const std::string signature = "signature.bin";
    UpdateHandlerMock handler;

    EXPECT_CALL(handler, checkAvailable(_)).WillOnce(Return(true));
    EXPECT_CALL(handler, sendFile(_, image)).WillOnce(Return());
    EXPECT_CALL(handler, sendFile(_, signature)).WillOnce(Return());
    EXPECT_CALL(handler, verifyFile(ipmi_flash::verifyBlobId, defaultIgnore))
        .WillOnce(Return(true));
    EXPECT_CALL(handler, verifyFile(ipmi_flash::updateBlobId, defaultIgnore))
        .WillOnce(Return(true));

    updaterMain(&handler, image, signature, "static", defaultIgnore);
}

TEST_F(UpdaterTest, UpdateMainReturnsSuccessWithIgnoreUpdate)
{
    const std::string image = "image.bin";
    const std::string signature = "signature.bin";
    UpdateHandlerMock handler;
    bool updateIgnore = true;

    EXPECT_CALL(handler, checkAvailable(_)).WillOnce(Return(true));
    EXPECT_CALL(handler, sendFile(_, image)).WillOnce(Return());
    EXPECT_CALL(handler, sendFile(_, signature)).WillOnce(Return());
    EXPECT_CALL(handler, verifyFile(ipmi_flash::verifyBlobId, defaultIgnore))
        .WillOnce(Return(true));
    EXPECT_CALL(handler, verifyFile(ipmi_flash::updateBlobId, updateIgnore))
        .WillOnce(Return(true));

    updaterMain(&handler, image, signature, "static", updateIgnore);
}

TEST_F(UpdaterTest, UpdateMainCleansUpOnFailure)
{
    const std::string image = "image.bin";
    const std::string signature = "signature.bin";
    UpdateHandlerMock handler;

    EXPECT_CALL(handler, checkAvailable(_)).WillOnce(Return(true));
    EXPECT_CALL(handler, sendFile(_, image)).WillOnce(Return());
    EXPECT_CALL(handler, sendFile(_, signature)).WillOnce(Return());
    EXPECT_CALL(handler, verifyFile(ipmi_flash::verifyBlobId, defaultIgnore))
        .WillOnce(Return(false));
    EXPECT_CALL(handler, cleanArtifacts()).WillOnce(Return());

    EXPECT_THROW(
        updaterMain(&handler, image, signature, "static", defaultIgnore),
        ToolException);
}

} // namespace host_tool
