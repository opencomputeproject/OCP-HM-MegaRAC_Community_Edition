#include "helper.hpp"
#include "status.hpp"

#include <ipmiblob/test/blob_interface_mock.hpp>

#include <cstdint>

#include <gtest/gtest.h>

namespace host_tool
{
using ::testing::Return;
using ::testing::TypedEq;

class UpdaterTest : public ::testing::Test
{
  protected:
    ipmiblob::BlobInterfaceMock blobMock;
    std::uint16_t session = 0xbeef;
};

TEST_F(UpdaterTest, PollStatusReturnsAfterSuccess)
{
    ipmiblob::StatResponse verificationResponse = {};
    /* the other details of the response are ignored, and should be. */
    verificationResponse.metadata.push_back(
        static_cast<std::uint8_t>(ipmi_flash::ActionStatus::success));

    EXPECT_CALL(blobMock, getStat(TypedEq<std::uint16_t>(session)))
        .WillOnce(Return(verificationResponse));

    EXPECT_TRUE(pollStatus(session, &blobMock));
}

TEST_F(UpdaterTest, PollStatusReturnsAfterFailure)
{
    ipmiblob::StatResponse verificationResponse = {};
    /* the other details of the response are ignored, and should be. */
    verificationResponse.metadata.push_back(
        static_cast<std::uint8_t>(ipmi_flash::ActionStatus::failed));

    EXPECT_CALL(blobMock, getStat(TypedEq<std::uint16_t>(session)))
        .WillOnce(Return(verificationResponse));

    EXPECT_FALSE(pollStatus(session, &blobMock));
}

} // namespace host_tool
