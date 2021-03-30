#include "src/processing.hpp"

#include <gtest/gtest.h>

// Verify if name is empty, false is returned
TEST(NeedToIntrospect, PassEmptyName)
{
    WhiteBlackList whiteList;
    WhiteBlackList blackList;
    std::string process_name;

    EXPECT_FALSE(needToIntrospect(process_name, whiteList, blackList));
}

// Verify if name is on whitelist, true is returned
TEST(NeedToIntrospect, ValidWhiteListName)
{
    WhiteBlackList whiteList = {"xyz.openbmc_project"};
    WhiteBlackList blackList;
    std::string process_name = "xyz.openbmc_project.State.Host";

    EXPECT_TRUE(needToIntrospect(process_name, whiteList, blackList));
}

// Verify if name is on blacklist, false is returned
TEST(NeedToIntrospect, ValidBlackListName)
{
    WhiteBlackList whiteList;
    WhiteBlackList blackList = {"xyz.openbmc_project.State.Host"};
    std::string process_name = "xyz.openbmc_project.State.Host";

    EXPECT_FALSE(needToIntrospect(process_name, whiteList, blackList));
}

// Verify if name is on whitelist and blacklist, false is returned
TEST(NeedToIntrospect, ValidWhiteAndBlackListName)
{
    WhiteBlackList whiteList = {"xyz.openbmc_project"};
    WhiteBlackList blackList = {"xyz.openbmc_project.State.Host"};
    std::string process_name = "xyz.openbmc_project.State.Host";

    EXPECT_FALSE(needToIntrospect(process_name, whiteList, blackList));
}
