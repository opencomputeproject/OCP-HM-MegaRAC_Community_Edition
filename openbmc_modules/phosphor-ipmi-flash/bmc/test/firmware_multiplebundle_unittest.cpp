/* The goal of these tests is to verify that once a host-client has started the
 * process with one blob bundle, they cannot pivot to upload data to another.
 *
 * This prevent someone from starting to upload a BMC firmware, and then midway
 * through start uploading a BIOS image.
 */
#include "firmware_handler.hpp"
#include "flags.hpp"
#include "image_mock.hpp"
#include "status.hpp"
#include "triggerable_mock.hpp"
#include "util.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::Return;

class IpmiOnlyTwoFirmwaresTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        std::unique_ptr<ImageHandlerInterface> image =
            std::make_unique<ImageHandlerMock>();
        hashImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack(hashBlobId, std::move(image))));

        image = std::make_unique<ImageHandlerMock>();
        staticImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(
            std::move(HandlerPack(staticLayoutBlobId, std::move(image))));

        image = std::make_unique<ImageHandlerMock>();
        biosImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack(biosBlobId, std::move(image))));

        std::unique_ptr<TriggerableActionInterface> bmcPrepareMock =
            std::make_unique<TriggerMock>();
        bmcPrepareMockPtr =
            reinterpret_cast<TriggerMock*>(bmcPrepareMock.get());

        std::unique_ptr<TriggerableActionInterface> bmcVerifyMock =
            std::make_unique<TriggerMock>();
        bmcVerifyMockPtr = reinterpret_cast<TriggerMock*>(bmcVerifyMock.get());

        std::unique_ptr<TriggerableActionInterface> bmcUpdateMock =
            std::make_unique<TriggerMock>();
        bmcUpdateMockPtr = reinterpret_cast<TriggerMock*>(bmcUpdateMock.get());

        std::unique_ptr<TriggerableActionInterface> biosPrepareMock =
            std::make_unique<TriggerMock>();
        biosPrepareMockPtr =
            reinterpret_cast<TriggerMock*>(biosPrepareMock.get());

        std::unique_ptr<TriggerableActionInterface> biosVerifyMock =
            std::make_unique<TriggerMock>();
        biosVerifyMockPtr =
            reinterpret_cast<TriggerMock*>(biosVerifyMock.get());

        std::unique_ptr<TriggerableActionInterface> biosUpdateMock =
            std::make_unique<TriggerMock>();
        biosUpdateMockPtr =
            reinterpret_cast<TriggerMock*>(biosUpdateMock.get());

        ActionMap packs;

        std::unique_ptr<ActionPack> bmcPack = std::make_unique<ActionPack>();
        bmcPack->preparation = std::move(bmcPrepareMock);
        bmcPack->verification = std::move(bmcVerifyMock);
        bmcPack->update = std::move(bmcUpdateMock);

        std::unique_ptr<ActionPack> biosPack = std::make_unique<ActionPack>();
        biosPack->preparation = std::move(biosPrepareMock);
        biosPack->verification = std::move(biosVerifyMock);
        biosPack->update = std::move(biosUpdateMock);

        packs[staticLayoutBlobId] = std::move(bmcPack);
        packs[biosBlobId] = std::move(biosPack);

        handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
            std::move(blobs), data, std::move(packs));
    }

    void expectedState(FirmwareBlobHandler::UpdateState state)
    {
        auto realHandler = dynamic_cast<FirmwareBlobHandler*>(handler.get());
        EXPECT_EQ(state, realHandler->getCurrentState());
    }

    ImageHandlerMock *hashImageMock, *staticImageMock, *biosImageMock;

    std::vector<HandlerPack> blobs;
    std::vector<DataHandlerPack> data = {
        {FirmwareFlags::UpdateFlags::ipmi, nullptr}};

    std::unique_ptr<blobs::GenericBlobInterface> handler;

    TriggerMock *bmcPrepareMockPtr, *bmcVerifyMockPtr, *bmcUpdateMockPtr;
    TriggerMock *biosPrepareMockPtr, *biosVerifyMockPtr, *biosUpdateMockPtr;

    std::uint16_t session = 1;
    std::uint16_t flags =
        blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi;
};

TEST_F(IpmiOnlyTwoFirmwaresTest, OpeningBiosAfterBlobFails)
{
    /* You can only have one file open at a time, and the first firmware file
     * you open locks it down
     */
    EXPECT_CALL(*staticImageMock, open(staticLayoutBlobId))
        .WillOnce(Return(true));
    EXPECT_CALL(*bmcPrepareMockPtr, trigger()).WillOnce(Return(true));

    EXPECT_TRUE(handler->open(session, flags, staticLayoutBlobId));
    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    EXPECT_CALL(*staticImageMock, close()).WillOnce(Return());
    handler->close(session);

    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);

    EXPECT_CALL(*biosImageMock, open(biosBlobId)).Times(0);
    EXPECT_FALSE(handler->open(session, flags, biosBlobId));
}

TEST_F(IpmiOnlyTwoFirmwaresTest, OpeningHashBeforeBiosSucceeds)
{
    /* Opening the hash blob does nothing special in this regard. */
    EXPECT_CALL(*hashImageMock, open(hashBlobId)).WillOnce(Return(true));
    EXPECT_TRUE(handler->open(session, flags, hashBlobId));

    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    EXPECT_CALL(*hashImageMock, close()).WillOnce(Return());
    handler->close(session);

    expectedState(FirmwareBlobHandler::UpdateState::verificationPending);
    ASSERT_FALSE(handler->canHandleBlob(verifyBlobId));

    EXPECT_CALL(*biosImageMock, open(biosBlobId)).WillOnce(Return(true));
    EXPECT_TRUE(handler->open(session, flags, biosBlobId));

    expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);

    EXPECT_CALL(*biosImageMock, close()).WillOnce(Return());
    handler->close(session);
}

} // namespace
} // namespace ipmi_flash
