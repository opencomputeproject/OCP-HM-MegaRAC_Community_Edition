#include "blob_mock.hpp"
#include "manager.hpp"

#include <string>

#include <gtest/gtest.h>

namespace blobs
{

using namespace std::chrono_literals;

using ::testing::_;
using ::testing::Return;

TEST(ManagerExpireTest, OpenWithLongTimeoutSucceeds)
{
    // With a long timeout, open should succeed without calling expire.
    BlobManager mgr(2min);
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillOnce(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillOnce(Return(true));
    EXPECT_TRUE(mgr.open(flags, path, &sess));
    // Do not expect the open session to expire
    EXPECT_CALL(*m1ptr, expire(sess)).Times(0);
}

TEST(ManagerExpireTest, ZeroTimeoutWillCauseExpiration)
{
    // With timeout being zero, every open will cause all previous opened
    // sessions to expire.
    BlobManager mgr(0min);
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    uint16_t flags = OpenFlags::read, sess;
    std::string path = "/asdf/asdf";
    const int testIterations = 10;

    EXPECT_CALL(*m1ptr, canHandleBlob(path)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m1ptr, open(_, flags, path)).WillRepeatedly(Return(true));
    for (int i = 0; i < testIterations; ++i)
    {
        if (i != 0)
        {
            // Here 'sess' is the session ID obtained in previous loop
            EXPECT_CALL(*m1ptr, expire(sess)).WillOnce(Return(true));
        }
        EXPECT_TRUE(mgr.open(flags, path, &sess));
    }
}
} // namespace blobs
