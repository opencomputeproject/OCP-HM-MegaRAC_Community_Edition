#include "manager.hpp"

#include <gtest/gtest.h>

namespace blobs
{

TEST(ManagerGetSessionTest, NextSessionReturned)
{
    // This test verifies the next session ID is returned.
    BlobManager mgr;

    uint16_t first, second;
    EXPECT_TRUE(mgr.getSession(&first));
    EXPECT_TRUE(mgr.getSession(&second));
    EXPECT_FALSE(first == second);
}

TEST(ManagerGetSessionTest, SessionsCheckedAgainstList)
{
    // TODO(venture): Need a test that verifies the session ids are checked
    // against open sessions.
}
} // namespace blobs
