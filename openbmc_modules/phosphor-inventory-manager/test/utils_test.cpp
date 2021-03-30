#include "../utils.hpp"

#include <gtest/gtest.h>

using namespace phosphor::inventory::manager;
using namespace std::string_literals;

TEST(UtilsTest, TestVariantVisitor)
{
    std::variant<int, std::string> ib1(100);
    auto converted1 = convertVariant<std::variant<int>>(ib1);
    EXPECT_TRUE(std::get<int>(converted1) == 100);

    std::variant<int, std::string> ib2(100);
    EXPECT_THROW(convertVariant<std::variant<std::string>>(ib2),
                 std::runtime_error);
}

TEST(UtilsTest, TestCompareFirst)
{
    auto c = compareFirst(std::less<int>());
    auto p1 = std::make_pair(1, 2);
    auto p2 = std::make_pair(3, 4);

    EXPECT_TRUE(c(p1, p2));
    EXPECT_TRUE(c(1, p2));
    EXPECT_TRUE(c(p1, 2));
    EXPECT_FALSE(c(p2, p1));
    EXPECT_FALSE(c(p2, 1));
    EXPECT_FALSE(c(2, p1));

    auto p3 = std::make_pair(1, 100);
    auto p4 = std::make_pair(1, 200);

    EXPECT_FALSE(c(p3, p4));
    EXPECT_FALSE(c(1, p4));
    EXPECT_TRUE(c(p3, 2));
    EXPECT_FALSE(c(p4, p3));
    EXPECT_FALSE(c(p4, 1));
    EXPECT_FALSE(c(2, p3));
}

TEST(UtilsTest, TestRelPathComparePrefix)
{
    auto s1 = "prefixfoo"s;
    auto s2 = "prefixbar"s;
    RelPathCompare comp("prefix");

    EXPECT_TRUE(comp(s2, s1));
    EXPECT_FALSE(comp(s1, s2));

    auto s3 = "bar"s;
    auto s4 = "foo"s;

    EXPECT_FALSE(comp(s4, s3));
    EXPECT_TRUE(comp(s3, s4));

    auto s5 = "prefixbar"s;
    auto s6 = "foo"s;

    EXPECT_FALSE(comp(s6, s5));
    EXPECT_TRUE(comp(s5, s6));

    auto s7 = "bar"s;
    auto s8 = "prefixfoo"s;

    EXPECT_FALSE(comp(s8, s7));
    EXPECT_TRUE(comp(s7, s8));
}

TEST(UtilsTest, TestRelPathCompareNoPrefix)
{
    auto s1 = "prefixfoo"s;
    auto s2 = "prefixbar"s;
    RelPathCompare comp("");

    EXPECT_TRUE(comp(s2, s1));
    EXPECT_FALSE(comp(s1, s2));

    auto s3 = "bar"s;
    auto s4 = "foo"s;

    EXPECT_FALSE(comp(s4, s3));
    EXPECT_TRUE(comp(s3, s4));

    auto s5 = "prefixbar"s;
    auto s6 = "foo"s;

    EXPECT_TRUE(comp(s6, s5));
    EXPECT_FALSE(comp(s5, s6));

    auto s7 = "bar"s;
    auto s8 = "prefixfoo"s;

    EXPECT_FALSE(comp(s8, s7));
    EXPECT_TRUE(comp(s7, s8));
}
