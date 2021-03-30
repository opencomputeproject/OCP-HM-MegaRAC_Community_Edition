#include "remote_logging_tests.hpp"

namespace phosphor
{
namespace logging
{
namespace test
{

TEST_F(TestRemoteLogging, testGoodPort)
{
    config->port(100);
    EXPECT_EQ(config->port(), 100);
}

} // namespace test
} // namespace logging
} // namespace phosphor
