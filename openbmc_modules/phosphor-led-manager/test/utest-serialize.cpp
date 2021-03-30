#include "serialize.hpp"

#include <filesystem>

#include <gtest/gtest.h>

using namespace phosphor::led;

TEST(SerializeTest, testStoreGroups)
{
    namespace fs = std::filesystem;

    static constexpr auto& path = "config/led-save-group.json";
    static constexpr auto& bmcBooted =
        "/xyz/openbmc_project/led/groups/bmc_booted";
    static constexpr auto& powerOn = "/xyz/openbmc_project/led/groups/power_on";
    static constexpr auto& enclosureIdentify =
        "/xyz/openbmc_project/led/groups/EnclosureIdentify";

    Serialize serialize(path);

    serialize.storeGroups(bmcBooted, true);
    ASSERT_EQ(true, serialize.getGroupSavedState(bmcBooted));

    serialize.storeGroups(powerOn, true);
    ASSERT_EQ(true, serialize.getGroupSavedState(powerOn));

    serialize.storeGroups(bmcBooted, false);
    ASSERT_EQ(false, serialize.getGroupSavedState(bmcBooted));

    serialize.storeGroups(enclosureIdentify, true);
    ASSERT_EQ(true, serialize.getGroupSavedState(enclosureIdentify));

    Serialize newSerial(path);

    ASSERT_EQ(true, newSerial.getGroupSavedState(powerOn));
    ASSERT_EQ(true, newSerial.getGroupSavedState(enclosureIdentify));

    newSerial.storeGroups(powerOn, false);
    ASSERT_EQ(false, newSerial.getGroupSavedState(powerOn));

    newSerial.storeGroups(enclosureIdentify, false);
    ASSERT_EQ(false, newSerial.getGroupSavedState(enclosureIdentify));
}
