#include "json-config.hpp"

#include <gtest/gtest.h>

TEST(loadJsonConfig, testGoodPath)
{
    static constexpr auto jsonPath = "config/led-group-config.json";
    LedMap ledMap = loadJsonConfig(jsonPath);

    std::string objPath = "/xyz/openbmc_project/led/groups";
    std::string bmcBooted = objPath + "/bmc_booted";
    std::string powerOn = objPath + "/power_on";
    std::string enclosureIdentify = objPath + "/enclosure_identify";

    ASSERT_NE(ledMap.find(bmcBooted), ledMap.end());
    ASSERT_NE(ledMap.find(powerOn), ledMap.end());
    ASSERT_NE(ledMap.find(enclosureIdentify), ledMap.end());

    LedAction bmcBootedActions = ledMap.at(bmcBooted);
    LedAction powerOnActions = ledMap.at(powerOn);
    LedAction enclosureIdentifyActions = ledMap.at(enclosureIdentify);

    for (const auto& group : bmcBootedActions)
    {
        ASSERT_EQ(group.name, "heartbeat");
        ASSERT_EQ(group.action, phosphor::led::Layout::On);
        ASSERT_EQ(group.dutyOn, 50);
        ASSERT_EQ(group.period, 0);
        ASSERT_EQ(group.priority, phosphor::led::Layout::Blink);
    }

    for (const auto& group : powerOnActions)
    {
        ASSERT_EQ(group.name, "power");
        ASSERT_EQ(group.action, phosphor::led::Layout::On);
        ASSERT_EQ(group.dutyOn, 50);
        ASSERT_EQ(group.period, 0);
        ASSERT_EQ(group.priority, phosphor::led::Layout::On);
    }

    for (const auto& group : enclosureIdentifyActions)
    {
        if (group.name == "front_id")
        {
            ASSERT_EQ(group.action, phosphor::led::Layout::Blink);
            ASSERT_EQ(group.dutyOn, 50);
            ASSERT_EQ(group.period, 1000);
            ASSERT_EQ(group.priority, phosphor::led::Layout::Blink);
        }
        else if (group.name == "rear_id")
        {
            ASSERT_EQ(group.action, phosphor::led::Layout::Blink);
            ASSERT_EQ(group.dutyOn, 50);
            ASSERT_EQ(group.period, 1000);
            ASSERT_EQ(group.priority, phosphor::led::Layout::Blink);
        }
        else
        {
            ASSERT_TRUE(false);
        }
    }
}

TEST(loadJsonConfig, testBadPath)
{
    static constexpr auto jsonPath = "config/led-group-config-malformed.json";
    ASSERT_THROW(loadJsonConfig(jsonPath), std::exception);
}

TEST(validatePriority, testGoodPriority)
{
    PriorityMap priorityMap{};
    validatePriority("heartbeat", phosphor::led::Layout::Blink, priorityMap);
    validatePriority("power", phosphor::led::Layout::On, priorityMap);

    ASSERT_EQ(priorityMap.at("heartbeat"), phosphor::led::Layout::Blink);
    ASSERT_EQ(priorityMap.at("power"), phosphor::led::Layout::On);
}

TEST(validatePriority, testBadPriority)
{
    PriorityMap priorityMap{};
    validatePriority("heartbeat", phosphor::led::Layout::Blink, priorityMap);

    ASSERT_EQ(priorityMap.at("heartbeat"), phosphor::led::Layout::Blink);
    ASSERT_THROW(
        validatePriority("heartbeat", phosphor::led::Layout::On, priorityMap),
        std::runtime_error);
}