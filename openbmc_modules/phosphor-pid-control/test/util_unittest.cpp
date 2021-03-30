#include "util.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(UtilTest, WriteTypeEmptyString_ReturnsNONE)
{
    // Verify it responds to an empty string.

    EXPECT_EQ(IOInterfaceType::NONE, getWriteInterfaceType(""));
}

TEST(UtilTest, WriteTypeNonePath_ReturnsNONE)
{
    // Verify it responds to a path of "None"

    EXPECT_EQ(IOInterfaceType::NONE, getWriteInterfaceType("None"));
}

TEST(UtilTest, WriteTypeSysfs_ReturnsSYSFS)
{
    // Verify the sysfs type is determined with an expected path

    std::string path = "/sys/devices/asfdadsf";
    EXPECT_EQ(IOInterfaceType::SYSFS, getWriteInterfaceType(path));
}

TEST(UtilTest, WriteTypeUnknown_ReturnsUNKNOWN)
{
    // Verify it reports unknown by default.

    std::string path = "/xyz/openbmc_project";
    EXPECT_EQ(IOInterfaceType::UNKNOWN, getWriteInterfaceType(path));
}

TEST(UtilTest, ReadTypeEmptyString_ReturnsNONE)
{
    // Verify it responds to an empty string.

    EXPECT_EQ(IOInterfaceType::NONE, getReadInterfaceType(""));
}

TEST(UtilTest, ReadTypeNonePath_ReturnsNONE)
{
    // Verify it responds to a path of "None"

    EXPECT_EQ(IOInterfaceType::NONE, getReadInterfaceType("None"));
}

TEST(UtilTest, ReadTypeExternalSensors_ReturnsEXTERNAL)
{
    // Verify it responds to a path that represents a host sensor.

    std::string path = "/xyz/openbmc_project/extsensors/temperature/fleeting0";
    EXPECT_EQ(IOInterfaceType::EXTERNAL, getReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeOpenBMCSensor_ReturnsDBUSPASSIVE)
{
    // Verify it responds to a path that represents a dbus sensor.

    std::string path = "/xyz/openbmc_project/sensors/fan_tach/fan1";
    EXPECT_EQ(IOInterfaceType::DBUSPASSIVE, getReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeSysfsPath_ReturnsSYSFS)
{
    // Verify the sysfs type is determined with an expected path

    std::string path = "/sys/devices/asdf/asdf0";
    EXPECT_EQ(IOInterfaceType::SYSFS, getReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeUnknownDefault_ReturnsUNKNOWN)
{
    // Verify it reports unknown by default.

    std::string path = "asdf09as0df9a0fd";
    EXPECT_EQ(IOInterfaceType::UNKNOWN, getReadInterfaceType(path));
}
