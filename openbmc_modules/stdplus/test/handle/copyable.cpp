#include <gtest/gtest.h>
#include <optional>
#include <stdplus/handle/copyable.hpp>
#include <string>
#include <utility>
#include <vector>

namespace stdplus
{
namespace
{

static std::vector<int> reffed;
static int stored_ref = 0;
static std::vector<int> dropped;
static int stored_drop = 0;

int ref(const int& i)
{
    reffed.push_back(i);
    return i + 1;
}

void drop(int&& i)
{
    dropped.push_back(std::move(i));
}

int ref(const int& i, std::string&, int& si)
{
    reffed.push_back(i);
    // Make sure we can update the stored data
    stored_ref = si++;
    return i + 1;
}

void drop(int&& i, std::string&, int& si)
{
    dropped.push_back(std::move(i));
    // Make sure we can update the stored data
    stored_drop = si++;
}

using SimpleHandle = Copyable<int>::Handle<drop, ref>;
using StoreHandle = Copyable<int, std::string, int>::Handle<drop, ref>;

class CopyableHandleTest : public ::testing::Test
{
  protected:
    void SetUp()
    {
        reffed.clear();
        dropped.clear();
    }

    void TearDown()
    {
        EXPECT_TRUE(reffed.empty());
        EXPECT_TRUE(dropped.empty());
    }
};

TEST_F(CopyableHandleTest, EmptyNoStorage)
{
    SimpleHandle h(std::nullopt);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    h.reset();
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
}

TEST_F(CopyableHandleTest, EmptyWithStorage)
{
    auto maybeV = std::nullopt;
    StoreHandle h(maybeV, "str", 5);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    h.reset(maybeV);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
}

TEST_F(CopyableHandleTest, SimplePopulated)
{
    constexpr int expected = 3;
    {
        int val = expected;
        SimpleHandle h(std::move(val));
        EXPECT_TRUE(h);
        EXPECT_EQ(expected, *h);
        EXPECT_EQ(expected, h.value());
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, OptionalPopulated)
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
    EXPECT_TRUE(reffed.empty());
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
    {
        const std::optional<int> maybeVal{expected};
        SimpleHandle h(maybeVal);
        EXPECT_TRUE(h);
        EXPECT_EQ(expected + 1, *h);
        EXPECT_EQ(expected + 1, h.value());
        EXPECT_EQ(std::vector{expected}, reffed);
        reffed.clear();
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(std::vector{expected + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, SimplePopulatedWithStorage)
{
    constexpr int expected = 3;
    {
        StoreHandle h(expected, std::string{"str"}, 5);
        EXPECT_TRUE(h);
        EXPECT_EQ(expected + 1, *h);
        EXPECT_EQ(expected + 1, h.value());
        EXPECT_EQ(5, stored_ref);
        EXPECT_EQ(std::vector{expected}, reffed);
        reffed.clear();
        EXPECT_TRUE(dropped.empty());
    }
    EXPECT_EQ(6, stored_drop);
    EXPECT_EQ(std::vector{expected + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, ResetPopulatedWithStorage)
{
    constexpr int expected = 3;
    const std::string s{"str"};
    StoreHandle h(int{expected}, s, 5);
    EXPECT_TRUE(dropped.empty());
    h.reset(std::nullopt);
    EXPECT_FALSE(h);
    EXPECT_THROW(h.value(), std::bad_optional_access);
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, ResetNewPopulated)
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

TEST_F(CopyableHandleTest, ResetCopyPopulated)
{
    constexpr int expected = 3, expected2 = 10;
    {
        SimpleHandle h(int{expected});
        EXPECT_TRUE(reffed.empty());
        EXPECT_TRUE(dropped.empty());
        const std::optional<int> maybe2{expected2};
        h.reset(maybe2);
        EXPECT_TRUE(h);
        EXPECT_EQ(expected2 + 1, *h);
        EXPECT_EQ(expected2 + 1, h.value());
        EXPECT_EQ(std::vector{expected2}, reffed);
        reffed.clear();
        EXPECT_EQ(std::vector{expected}, dropped);
        dropped.clear();
    }
    EXPECT_EQ(std::vector{expected2 + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, ResetCopyPopulatedWithStorage)
{
    constexpr int expected = 3, expected2 = 10;
    {
        StoreHandle h(int{expected}, "str", 5);
        EXPECT_TRUE(dropped.empty());
        h.reset(expected2);
        EXPECT_TRUE(h);
        EXPECT_EQ(expected2 + 1, *h);
        EXPECT_EQ(expected2 + 1, h.value());
        EXPECT_EQ(5, stored_ref);
        EXPECT_EQ(std::vector{expected2}, reffed);
        reffed.clear();
        EXPECT_EQ(6, stored_drop);
        EXPECT_EQ(std::vector{expected}, dropped);
        dropped.clear();
    }
    EXPECT_EQ(7, stored_drop);
    EXPECT_EQ(std::vector{expected2 + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, MoveConstructWithStorage)
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
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, MoveAssignWithStorage)
{
    constexpr int expected = 3, expected2 = 10;
    {
        StoreHandle h1(int{expected}, "str", 5);
        StoreHandle h2(int{expected2}, "str", 10);
        EXPECT_TRUE(dropped.empty());

        h2 = std::move(h1);
        EXPECT_EQ(10, stored_drop);
        EXPECT_EQ(std::vector{expected2}, dropped);
        dropped.clear();
        EXPECT_FALSE(h1);
        EXPECT_THROW(h1.value(), std::bad_optional_access);
        EXPECT_TRUE(h2);
        EXPECT_EQ(expected, *h2);
        EXPECT_EQ(expected, h2.value());
    }
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, CopyConstructSrcEmptyWithStorage)
{
    StoreHandle h1(std::nullopt, "str", 5);
    StoreHandle h2(h1);
}

TEST_F(CopyableHandleTest, CopyConstructWithStorage)
{
    constexpr int expected = 3;
    StoreHandle h1(int{expected}, "str", 5);
    StoreHandle h2(h1);
    EXPECT_EQ(5, stored_ref);
    EXPECT_EQ(std::vector{expected}, reffed);
    reffed.clear();

    h1.reset();
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
    h2.reset();
    EXPECT_EQ(6, stored_drop);
    EXPECT_EQ(std::vector{expected + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, CopyAssignBothEmptyWithStorage)
{
    StoreHandle h1(std::nullopt, "str", 5);
    StoreHandle h2(std::nullopt, "str", 10);
    h2 = h1;
}

TEST_F(CopyableHandleTest, CopyAssignSrcEmptyWithStorage)
{
    constexpr int expected = 3;
    StoreHandle h1(std::nullopt, "str", 5);
    StoreHandle h2(int{expected}, "str", 10);
    h2 = h1;
    EXPECT_EQ(10, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, CopyAssignDstEmptyWithStorage)
{
    constexpr int expected = 3;
    StoreHandle h1(int{expected}, "str", 5);
    StoreHandle h2(std::nullopt, "str", 10);
    h2 = h1;
    EXPECT_EQ(5, stored_ref);
    EXPECT_EQ(std::vector{expected}, reffed);
    reffed.clear();

    h1.reset();
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
    h2.reset();
    EXPECT_EQ(6, stored_drop);
    EXPECT_EQ(std::vector{expected + 1}, dropped);
    dropped.clear();
}

TEST_F(CopyableHandleTest, CopyAssignWithStorage)
{
    constexpr int expected = 3, expected2 = 15;
    StoreHandle h1(int{expected}, "str", 5);
    StoreHandle h2(int{expected2}, "str", 10);
    h2 = h1;
    EXPECT_EQ(10, stored_drop);
    EXPECT_EQ(std::vector{expected2}, dropped);
    dropped.clear();
    EXPECT_EQ(5, stored_ref);
    EXPECT_EQ(std::vector{expected}, reffed);
    reffed.clear();

    h1.reset();
    EXPECT_EQ(5, stored_drop);
    EXPECT_EQ(std::vector{expected}, dropped);
    dropped.clear();
    h2.reset();
    EXPECT_EQ(6, stored_drop);
    EXPECT_EQ(std::vector{expected + 1}, dropped);
    dropped.clear();
}

} // namespace
} // namespace stdplus
