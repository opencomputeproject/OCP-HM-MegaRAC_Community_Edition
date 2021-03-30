#include "event.hpp"
#include "pathwatchimpl.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace phosphor::dbus::monitoring;

#include "interfaceaddtest.hpp"

const std::array<std::string, 1> expectedPaths = {
    "/xyz/openbmc_project/testing/inst1"s,
};

const std::array<std::string, 1> expectedWatches = {
    "/xyz/openbmc_project/testing/inst1"s,
};

TEST(InterfaceAddTest, PathsSameSize)
{
    ASSERT_EQ(sizeof(expectedPaths), sizeof(paths));
}

TEST(InterfaceAddTest, WatchSameSize)
{
    ASSERT_EQ(expectedWatches.size(), pathwatches.size());
}