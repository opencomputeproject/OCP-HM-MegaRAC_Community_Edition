#include "data_types.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace phosphor::dbus::monitoring;
using PathMeta = TupleOfRefs<const std::string, const std::string>;

#include "pathgentest.hpp"

const std::array<std::string, 3> expectedMeta = {
    "PATH1"s,
    "PATH3"s,
    "PATH2"s,
};

const std::array<std::string, 6> expectedPaths = {
    "/xyz/openbmc_project/testing/inst1"s,
    "/xyz/openbmc_project/testing/inst2"s,
    "/xyz/openbmc_project/testing/inst3"s,
    "/xyz/openbmc_project/testing/inst4"s,
    "/xyz/openbmc_project/testing/inst5"s,
    "/xyz/openbmc_project/testing/inst6"s,
};

const std::array<PathMeta, 14> expectedPathMeta = {{
    PathMeta{paths[0], meta[0]},
    PathMeta{paths[1], meta[0]},
    PathMeta{paths[2], meta[0]},
    PathMeta{paths[3], meta[0]},
    PathMeta{paths[0], meta[1]},
    PathMeta{paths[1], meta[1]},
    PathMeta{paths[2], meta[1]},
    PathMeta{paths[3], meta[1]},
    PathMeta{paths[4], meta[0]},
    PathMeta{paths[5], meta[0]},
    PathMeta{paths[3], meta[2]},
    PathMeta{paths[2], meta[2]},
    PathMeta{paths[1], meta[2]},
    PathMeta{paths[0], meta[2]},
}};

const std::array<RefVector<const std::string>, 4> expectedGroups = {{
    {
        paths[0],
        paths[1],
        paths[2],
        paths[3],
    },
    {
        paths[0],
        paths[1],
        paths[2],
        paths[3],
    },
    {
        paths[0],
        paths[1],
        paths[4],
        paths[5],
    },
    {
        paths[3],
        paths[2],
        paths[1],
        paths[0],
    },
}};

TEST(PathGenTest, MetaSameSize)
{
    ASSERT_EQ(sizeof(expectedMeta), sizeof(meta));
}

TEST(PathGenTest, PathsSameSize)
{
    ASSERT_EQ(sizeof(expectedPaths), sizeof(paths));
}

TEST(PathGenTest, PathMetaSameSize)
{
    ASSERT_EQ(sizeof(expectedPathMeta), sizeof(pathMeta));
}

TEST(PathGenTest, GroupsSameSize)
{
    ASSERT_EQ(sizeof(expectedGroups), sizeof(groups));
}

TEST(PathGenTest, MetaSameContent)
{
    size_t i;
    for (i = 0; i < expectedMeta.size(); ++i)
    {
        ASSERT_EQ(meta[i], expectedMeta[i]);
    }
}

TEST(PathGenTest, PathsSameContent)
{
    size_t i;
    for (i = 0; i < expectedPaths.size(); ++i)
    {
        ASSERT_EQ(paths[i], expectedPaths[i]);
    }
}

TEST(PathGenTest, PathMetaSameContent)
{
    size_t i;
    for (i = 0; i < expectedPathMeta.size(); ++i)
    {
        const auto& path = std::get<0>(pathMeta[i]).get();
        const auto& expPath = std::get<0>(expectedPathMeta[i]).get();
        const auto& meta = std::get<1>(pathMeta[i]).get();
        const auto& expMeta = std::get<1>(expectedPathMeta[i]).get();

        ASSERT_EQ(path, expPath);
        ASSERT_EQ(meta, expMeta);
    }
}

TEST(PathGenTest, GroupsSameContent)
{
    size_t i;
    for (i = 0; i < expectedGroups.size(); ++i)
    {
        size_t j;
        for (j = 0; j < groups[i].size(); ++j)
        {
            ASSERT_EQ(groups[i][j].get(), expectedGroups[i][j].get());
        }
    }
}
