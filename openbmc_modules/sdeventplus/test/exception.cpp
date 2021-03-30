#include <gtest/gtest.h>
#include <sdeventplus/exception.hpp>
#include <string>
#include <system_error>

namespace sdeventplus
{
namespace
{

TEST(ExceptionTest, Construct)
{
    const int code = EINTR;
    const char* const prefix = "construct_test";

    std::system_error expected(code, std::generic_category(), prefix);
    SdEventError err(code, prefix);

    EXPECT_EQ(std::string{expected.what()}, err.what());
    EXPECT_EQ(code, err.code().value());
    EXPECT_EQ(std::generic_category(), err.code().category());
}

} // namespace
} // namespace sdeventplus
