#pragma once

#include "data_mock.hpp"
#include "firmware_handler.hpp"
#include "flags.hpp"
#include "image_mock.hpp"
#include "triggerable_mock.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ipmi_flash
{
namespace
{

using ::testing::Return;

class IpmiOnlyFirmwareStaticTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        /* Unfortunately, since the FirmwareHandler object ends up owning the
         * handlers, we can't just share handlers.
         */
        std::unique_ptr<ImageHandlerInterface> image =
            std::make_unique<ImageHandlerMock>();
        hashImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack(hashBlobId, std::move(image))));

        image = std::make_unique<ImageHandlerMock>();
        imageMock2 = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(
            std::move(HandlerPack(staticLayoutBlobId, std::move(image))));

        std::unique_ptr<TriggerableActionInterface> prepareMock =
            std::make_unique<TriggerMock>();
        prepareMockPtr = reinterpret_cast<TriggerMock*>(prepareMock.get());

        std::unique_ptr<TriggerableActionInterface> verifyMock =
            std::make_unique<TriggerMock>();
        verifyMockPtr = reinterpret_cast<TriggerMock*>(verifyMock.get());

        std::unique_ptr<TriggerableActionInterface> updateMock =
            std::make_unique<TriggerMock>();
        updateMockPtr = reinterpret_cast<TriggerMock*>(updateMock.get());

        std::unique_ptr<ActionPack> actionPack = std::make_unique<ActionPack>();
        actionPack->preparation = std::move(prepareMock);
        actionPack->verification = std::move(verifyMock);
        actionPack->update = std::move(updateMock);

        ActionMap packs;
        packs[staticLayoutBlobId] = std::move(actionPack);

        handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
            std::move(blobs), data, std::move(packs));
    }

    void expectedState(FirmwareBlobHandler::UpdateState state)
    {
        auto realHandler = dynamic_cast<FirmwareBlobHandler*>(handler.get());
        EXPECT_EQ(state, realHandler->getCurrentState());
    }

    void openToInProgress(const std::string& blobId)
    {
        if (blobId == hashBlobId)
        {
            EXPECT_CALL(*hashImageMock, open(blobId)).WillOnce(Return(true));
        }
        else
        {
            EXPECT_CALL(*imageMock2, open(blobId)).WillOnce(Return(true));
        }

        if (blobId != hashBlobId)
        {
            EXPECT_CALL(*prepareMockPtr, trigger()).WillOnce(Return(true));
        }
        EXPECT_TRUE(handler->open(session, flags, blobId));
        expectedState(FirmwareBlobHandler::UpdateState::uploadInProgress);
    }

    void getToVerificationPending(const std::string& blobId)
    {
        openToInProgress(blobId);

        if (blobId == hashBlobId)
        {
            EXPECT_CALL(*hashImageMock, close()).WillRepeatedly(Return());
        }
        else
        {
            EXPECT_CALL(*imageMock2, close()).WillRepeatedly(Return());
        }
        handler->close(session);
        expectedState(FirmwareBlobHandler::UpdateState::verificationPending);
    }

    void getToVerificationStarted(const std::string& blobId)
    {
        getToVerificationPending(blobId);

        EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
        EXPECT_CALL(*verifyMockPtr, trigger()).WillOnce(Return(true));

        EXPECT_TRUE(handler->commit(session, {}));
        expectedState(FirmwareBlobHandler::UpdateState::verificationStarted);
    }

    void getToVerificationStartedWitHashBlob()
    {
        /* Open both static and hash to check for activeHashBlobId. */
        getToVerificationPending(staticLayoutBlobId);

        openToInProgress(hashBlobId);
        EXPECT_CALL(*hashImageMock, close()).WillRepeatedly(Return());
        handler->close(session);
        expectedState(FirmwareBlobHandler::UpdateState::verificationPending);

        /* Now the hash is active AND the static image is active. */
        EXPECT_TRUE(handler->open(session, flags, verifyBlobId));
        EXPECT_CALL(*verifyMockPtr, trigger()).WillOnce(Return(true));

        EXPECT_TRUE(handler->commit(session, {}));
        expectedState(FirmwareBlobHandler::UpdateState::verificationStarted);
    }

    void getToVerificationCompleted(ActionStatus checkResponse)
    {
        getToVerificationStarted(staticLayoutBlobId);

        EXPECT_CALL(*verifyMockPtr, status()).WillOnce(Return(checkResponse));
        blobs::BlobMeta meta;
        EXPECT_TRUE(handler->stat(session, &meta));
        expectedState(FirmwareBlobHandler::UpdateState::verificationCompleted);
    }

    void getToUpdatePending()
    {
        getToVerificationCompleted(ActionStatus::success);

        handler->close(session);
        expectedState(FirmwareBlobHandler::UpdateState::updatePending);
    }

    void getToUpdateStarted()
    {
        getToUpdatePending();
        EXPECT_TRUE(handler->open(session, flags, updateBlobId));

        EXPECT_CALL(*updateMockPtr, trigger()).WillOnce(Return(true));
        EXPECT_TRUE(handler->commit(session, {}));
        expectedState(FirmwareBlobHandler::UpdateState::updateStarted);
    }

    void getToUpdateCompleted(ActionStatus result)
    {
        getToUpdateStarted();
        EXPECT_CALL(*updateMockPtr, status()).WillOnce(Return(result));

        blobs::BlobMeta meta;
        EXPECT_TRUE(handler->stat(session, &meta));
        expectedState(FirmwareBlobHandler::UpdateState::updateCompleted);
    }

    ImageHandlerMock *hashImageMock, *imageMock2;

    std::vector<HandlerPack> blobs;
    std::vector<DataHandlerPack> data = {
        {FirmwareFlags::UpdateFlags::ipmi, nullptr}};

    std::unique_ptr<blobs::GenericBlobInterface> handler;

    TriggerMock* prepareMockPtr;
    TriggerMock* verifyMockPtr;
    TriggerMock* updateMockPtr;

    std::uint16_t session = 1;
    std::uint16_t flags =
        blobs::OpenFlags::write | FirmwareFlags::UpdateFlags::ipmi;

    blobs::BlobMeta expectedIdleMeta = {0xff00, 0, {}};

    std::vector<std::string> startingBlobs = {staticLayoutBlobId, hashBlobId};
};

class IpmiOnlyFirmwareTest : public ::testing::Test
{
  protected:
    ImageHandlerMock *hashImageMock, *imageMock;
    std::vector<HandlerPack> blobs;
    std::vector<DataHandlerPack> data = {
        {FirmwareFlags::UpdateFlags::ipmi, nullptr}};
    std::unique_ptr<blobs::GenericBlobInterface> handler;

    void SetUp() override
    {
        std::unique_ptr<ImageHandlerInterface> image =
            std::make_unique<ImageHandlerMock>();
        hashImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack(hashBlobId, std::move(image))));

        image = std::make_unique<ImageHandlerMock>();
        imageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack("asdf", std::move(image))));

        handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
            std::move(blobs), data, std::move(CreateActionMap("asdf")));
    }
};

class FakeLpcFirmwareTest : public ::testing::Test
{
  protected:
    DataHandlerMock dataMock;
    ImageHandlerMock *hashImageMock, *imageMock;
    std::vector<HandlerPack> blobs;
    std::vector<DataHandlerPack> data;
    std::unique_ptr<blobs::GenericBlobInterface> handler;

    void SetUp() override
    {
        std::unique_ptr<ImageHandlerInterface> image =
            std::make_unique<ImageHandlerMock>();
        hashImageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack(hashBlobId, std::move(image))));

        image = std::make_unique<ImageHandlerMock>();
        imageMock = reinterpret_cast<ImageHandlerMock*>(image.get());
        blobs.push_back(std::move(HandlerPack("asdf", std::move(image))));

        data = {
            {FirmwareFlags::UpdateFlags::ipmi, nullptr},
            {FirmwareFlags::UpdateFlags::lpc, &dataMock},
        };
        handler = FirmwareBlobHandler::CreateFirmwareBlobHandler(
            std::move(blobs), data, std::move(CreateActionMap("asdf")));
    }
};

} // namespace
} // namespace ipmi_flash
