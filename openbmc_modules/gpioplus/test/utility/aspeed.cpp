#include <gpioplus/utility/aspeed.hpp>
#include <gtest/gtest.h>

namespace gpioplus
{
namespace utility
{
namespace aspeed
{
namespace
{

TEST(AspeedTest, NameToOffset)
{
    EXPECT_EQ(5, nameToOffset("A5"));
    EXPECT_EQ(33, nameToOffset("E1"));
    EXPECT_EQ(202, nameToOffset("Z2"));
    EXPECT_EQ(208, nameToOffset("AA0"));
    EXPECT_EQ(223, nameToOffset("AB7"));
}

TEST(AspeedTest, NameToOffsetShort)
{
    EXPECT_THROW(nameToOffset(""), std::logic_error);
    EXPECT_THROW(nameToOffset("A"), std::logic_error);
    EXPECT_THROW(nameToOffset("0"), std::logic_error);
}

TEST(AspeedTest, NameToOffsetBad)
{
    EXPECT_THROW(nameToOffset("00"), std::logic_error);
    EXPECT_THROW(nameToOffset("AB"), std::logic_error);
    EXPECT_THROW(nameToOffset(".1"), std::logic_error);
    EXPECT_THROW(nameToOffset("A#"), std::logic_error);
}

TEST(AspeedTest, NameToOffsetMaybeBad)
{
    EXPECT_THROW(nameToOffset("BA0"), std::logic_error);
    EXPECT_THROW(nameToOffset("AAA0"), std::logic_error);
}

} // namespace
} // namespace aspeed
} // namespace utility
} // namespace gpioplus
