#include <ipmiblob/ipmi_errors.hpp>

#include <gtest/gtest.h>

namespace ipmiblob
{

TEST(IpmiExceptionTest, VerifyTimedOutIsString)
{
    /* Verify that throwing the exception with the cc code for timed out gets
     * converted to the human readable string.
     */
    bool verified = false;

    try
    {
        throw IpmiException(0xc3);
    }
    catch (const IpmiException& i)
    {
        EXPECT_STREQ("Received IPMI_CC: timeout", i.what());
        verified = true;
    }

    EXPECT_TRUE(verified);
}

} // namespace ipmiblob
