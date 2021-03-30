#include "data_types.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace phosphor::dbus::monitoring;

using Index = std::map<std::tuple<size_t, size_t, size_t>, size_t>;

#include "propertywatchgentest.hpp"

auto expectedStorageCount = 16;

const std::array<Index, 4> expectedIndicies = {{
    {
        {Index::key_type{0, 0, 0}, 0},
        {Index::key_type{0, 1, 0}, 1},
        {Index::key_type{1, 0, 0}, 2},
        {Index::key_type{1, 1, 0}, 3},
        {Index::key_type{2, 0, 0}, 4},
        {Index::key_type{2, 1, 0}, 5},
        {Index::key_type{3, 0, 0}, 6},
        {Index::key_type{3, 1, 0}, 7},
    },
    {
        {Index::key_type{2, 2, 1}, 8},
        {Index::key_type{2, 2, 2}, 9},
        {Index::key_type{3, 2, 1}, 10},
        {Index::key_type{3, 2, 2}, 11},
        {Index::key_type{4, 2, 1}, 12},
        {Index::key_type{4, 2, 2}, 13},
        {Index::key_type{5, 2, 1}, 14},
        {Index::key_type{5, 2, 2}, 15},
    },
    {
        {Index::key_type{3, 0, 0}, 6},
    },
    {
        {Index::key_type{3, 2, 2}, 11},
        {Index::key_type{5, 2, 2}, 15},
    },
}};

const std::array<std::tuple<std::string, size_t>, 4> expectedWatches = {{
    std::tuple<std::string, size_t>{"std::string"s, 0},
    std::tuple<std::string, size_t>{"uint32_t"s, 1},
    std::tuple<std::string, size_t>{"int32_t"s, 2},
    std::tuple<std::string, size_t>{"std::string"s, 3},
}};

TEST(PropertyWatchGenTest, storageCount)
{
    ASSERT_EQ(expectedStorageCount, storageCount);
}

TEST(PropertyWatchGenTest, IndiciesSameSize)
{
    ASSERT_EQ(sizeof(expectedIndicies), sizeof(indices));
}

TEST(PropertyWatchGenTest, WatchesSameSize)
{
    ASSERT_EQ(sizeof(expectedWatches), sizeof(watches));
}

TEST(PropertyWatchGenTest, WatchesSameContent)
{
    size_t i;
    for (i = 0; i < expectedWatches.size(); ++i)
    {
        ASSERT_EQ(watches[i], expectedWatches[i]);
    }
}

TEST(PropertyWatchGenTest, IndiciesSameContent)
{
    size_t i;
    for (i = 0; i < expectedIndicies.size(); ++i)
    {
        ASSERT_EQ(indices[i], expectedIndicies[i]);
    }
}
