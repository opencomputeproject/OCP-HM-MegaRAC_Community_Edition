#include <gtest/gtest.h>
#include <optional>
#include <stdplus/handle/managed.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace stdplus
{
namespace
{

static std::vector<int> dropped;
static int stored = 0;

void drop(int&& i)
{
    dropped.push_back(std::move(i));
}

void drop(int&& i, std::string&, int& si)
{
    dropped.push_back(std::move(i));
    // Make sure we can update the stored data
    stored = si++;
}

using SimpleHandle = Managed<int>::Handle<drop>;
using StoreHandle = Managed<int, std::string, int>::Handle<drop>;

class ManagedHandleTest : public ::testing::Test
{
  protected:
    void SetUp()
    {
        dropped.clear();
    }

    void TearDown()
    {
        EXPECT_TRUE(dropped.empty());
    }
};

TEST_F(ManagedHandleTest, EmptyNoStorage)
{
    SimpleHandle h(std::nullopt);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    EXPECT_THROW((void)h.release(), std::bad_optional_access);
    h.reset();
    EXPECT_FALSE(h.has_value());
    EXPECT_THROW(h.value(), std::bad_optional_access);
    EXPECT_THROW((void)h.release(), std::bad_optional_access);
    EXPECT_EQ(std::nullopt, h.maybe_value());
    EXPECT_EQ(std::nullopt, h.maybe_release());
}

TEST_F(ManagedHandleTest, EmptyWithStorage)
{
    StoreHandle h(std::nullopt, "str", 5);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    h.reset(std::nullopt);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);

    StoreHandle h2(std::nullopt, std::make_tuple<std::string, int>("str", 5));
    EXPECT_FALSE(h2);
    EXPECT_THROW(h2.value(), std::bad_optional_access);
}

TEST_F(ManagedHandleTest, SimplePopulated)
{
    constexpr int expected = 3;
    {
        int val = expected;
        SimpleHandle h(std::move(val));
        EXPECT_TRUE(h.has_value());
        EXPECT_EQ(expected, *h);
        EXPECT_EQ(expected, h.value());
        EXPECT_EQ(expected, h.maybe_value());
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, OptionalPopulated)
{
    constexpr int expected = 3;
    {
        std::optional<int> maybeVal{expected};
        SimpleHandle h(std::move(maybeVal));
        EXPECT_TRUE(h);
        EXPECT_EQ(expected, *h);
        EXPECT_EQ(expected, h.value());
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, SimplePopulatedWithStorage)
{
    constexpr int expected = 3;
    {
        StoreHandle h(int{expected}, std::make_tuple(std::string{"str"}, 5));
        EXPECT_TRUE(h);
        EXPECT_EQ(expected, *h);
        EXPECT_EQ(expected, h.value());
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(5, stored);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, ResetPopulatedWithStorage)
{
    constexpr int expected = 3;
    const std::string s{"str"};
    StoreHandle h(int{expected}, s, 5);
    EXPECT_TRUE(dropped.empty());
    h.reset(std::nullopt);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    EXPECT_EQ(5, stored);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, ResetNewPopulated)
{
    constexpr int expected = 3, expected2 = 10;
    {
        SimpleHandle h(int{expected});
        EXPECT_TRUE(dropped.empty());
        h.reset(int{expected2});
        EXPECT_TRUE(h);
        EXPECT_EQ(expected2, *h);
        EXPECT_EQ(expected2, h.value());
        EXPECT_EQ(std::vector{expected}, dropped);
        dropped.clear();
    }
    EXPECT_EQ(std::vector{expected2}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, ResetNewPopulatedWithStorage)
{
    constexpr int expected = 3, expected2 = 10;
    {
        StoreHandle h(int{expected}, "str", 5);
        EXPECT_TRUE(dropped.empty());
        h.reset(int{expected2});
        EXPECT_TRUE(h);
        EXPECT_EQ(expected2, *h);
        EXPECT_EQ(expected2, h.value());
        EXPECT_EQ(5, stored);
        EXPECT_EQ(std::vector{expected}, dropped);
        dropped.clear();
    }
    EXPECT_EQ(6, stored);
    EXPECT_EQ(std::vector{expected2}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, Release)
{
    constexpr int expected = 3;
    int val = expected;
    SimpleHandle h(std::move(val));
    EXPECT_EQ(expected, h.release());
    EXPECT_FALSE(h);
}

TEST_F(ManagedHandleTest, MaybeRelease)
{
    constexpr int expected = 3;
    int val = expected;
    SimpleHandle h(std::move(val));
    EXPECT_EQ(expected, h.maybe_release());
    EXPECT_FALSE(h);
}

TEST_F(ManagedHandleTest, MoveConstructWithStorage)
{
    constexpr int expected = 3;
    StoreHandle h1(int{expected}, "str", 5);
    {
        StoreHandle h2(std::move(h1));
        EXPECT_TRUE(dropped.empty());
        EXPECT_FALSE(h1);
        EXPECT_THROW(h1.value(), std::bad_optional_access);
        EXPECT_TRUE(h2);
        EXPECT_EQ(expected, *h2);
        EXPECT_EQ(expected, h2.value());
    }
    EXPECT_EQ(5, stored);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(ManagedHandleTest, MoveAssignWithStorage)
{
    constexpr int expected = 3, expected2 = 10;
    {
        StoreHandle h1(int{expected}, "str", 5);
        StoreHandle h2(int{expected2}, "str", 10);
        EXPECT_TRUE(dropped.empty());

        h2 = std::move(h1);
        EXPECT_EQ(10, stored);
        EXPECT_EQ(std::vector{expected2}, dropped);
        dropped.clear();
        EXPECT_FALSE(h1);
        EXPECT_THROW(h1.value(), std::bad_optional_access);
        EXPECT_TRUE(h2);
        EXPECT_EQ(expected, *h2);
        EXPECT_EQ(expected, h2.value());
    }
    EXPECT_EQ(5, stored);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

} // namespace
} // namespace stdplus
