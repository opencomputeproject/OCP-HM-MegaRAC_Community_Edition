#include <gtest/gtest.h>
#include <stdplus/util/string.hpp>
#include <string>
#include <string_view>

namespace stdplus
{
namespace util
{
namespace
{

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST(StrCat, NoStr)
{
    EXPECT_EQ("", strCat());
}

TEST(StrCat, SingleStr)
{
    EXPECT_EQ("func", strCat("func"));
}

TEST(StrCat, Multi)
{
    EXPECT_EQ("func world test", strCat("func", " world"sv, " test"s));
}

TEST(StrCat, MoveStr)
{
    EXPECT_EQ("func", strCat("func"s));
    EXPECT_EQ("func world", strCat("func"s, " world"));
}

} // namespace
} // namespace util
} // namespace stdplus
