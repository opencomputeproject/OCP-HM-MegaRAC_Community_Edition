#include "remote_logging_tests.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

namespace phosphor
{
namespace logging
{
namespace test
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;

TEST_F(TestRemoteLogging, testGoodAddress)
{
    config->address("1.1.1.1");
    EXPECT_EQ(config->address(), "1.1.1.1");
}

TEST_F(TestRemoteLogging, testBadAddress)
{
    EXPECT_THROW(config->address("this is not_an_IP!"), InvalidArgument);
}

} // namespace test
} // namespace logging
} // namespace phosphor
