#include "data_types.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace phosphor::dbus::monitoring;

using Property =
    TupleOfRefs<const std::string, const std::string, const std::string>;

using GroupOfProperties = std::vector<::Property>;

#include "propertygentest.hpp"
const std::array<std::string, 3> expectedMeta = {
    "PROPERTY1"s,
    "PROPERTY2"s,
    "PROPERTY3"s,
};

const std::array<std::string, 4> expectedInterfaces = {
    "xyz.openbmc_project.Test.Iface3"s,
    "xyz.openbmc_project.Test.Iface2"s,
    "xyz.openbmc_project.Test.Iface6"s,
    "xyz.openbmc_project.Test.Iface1"s,
};

const std::array<std::string, 4> expectedProperties = {
    "Foo"s,
    "Value"s,
    "Bar"s,
    "Baz"s,
};

const std::array<GroupOfProperties, 4> expectedGroups = {{
    {
        ::Property{interfaces[0], properties[0], meta[0]},
        ::Property{interfaces[1], properties[1], meta[1]},
    },
    {
        ::Property{interfaces[0], properties[2], meta[0]},
        ::Property{interfaces[1], properties[0], meta[1]},
    },
    {
        ::Property{interfaces[2], properties[0], meta[0]},
        ::Property{interfaces[3], properties[1], meta[1]},
    },
    {
        ::Property{interfaces[0], properties[2], meta[0]},
        ::Property{interfaces[1], properties[1], meta[1]},
        ::Property{interfaces[2], properties[3], meta[2]},
    },
}};

const std::array<std::string, 4> expectedTypes = {
    "uint32_t"s,
    "int32_t"s,
    "std::string"s,
    "int32_t"s,
};

TEST(PropertyGenTest, MetaSameSize)
{
    ASSERT_EQ(sizeof(expectedMeta), sizeof(meta));
}

TEST(PropertyGenTest, IfacesSameSize)
{
    ASSERT_EQ(sizeof(expectedInterfaces), sizeof(interfaces));
}

TEST(PropertyGenTest, PropertiesSameSize)
{
    ASSERT_EQ(sizeof(expectedProperties), sizeof(properties));
}

TEST(PropertyGenTest, GroupsSameSize)
{
    ASSERT_EQ(sizeof(expectedGroups), sizeof(groups));
}

TEST(PropertyGenTest, TypesSameSize)
{
    ASSERT_EQ(sizeof(expectedTypes), sizeof(types));
}

TEST(PropertyGenTest, MetaSameContent)
{
    size_t i;
    for (i = 0; i < expectedMeta.size(); ++i)
    {
        ASSERT_EQ(meta[i], expectedMeta[i]);
    }
}

TEST(PropertyGenTest, IfacesSameContent)
{
    size_t i;
    for (i = 0; i < expectedInterfaces.size(); ++i)
    {
        ASSERT_EQ(interfaces[i], expectedInterfaces[i]);
    }
}

TEST(PropertyGenTest, PropertiesSameContent)
{
    size_t i;
    for (i = 0; i < expectedProperties.size(); ++i)
    {
        ASSERT_EQ(expectedProperties[i], properties[i]);
    }
}

TEST(PropertyGenTest, GroupsSameContent)
{
    size_t i;
    for (i = 0; i < expectedGroups.size(); ++i)
    {
        size_t j;
        for (j = 0; j < expectedGroups[i].size(); ++j)
        {
            const auto& expectedIface = std::get<0>(expectedGroups[i][j]).get();
            const auto& actualIface = std::get<0>(groups[i][j]).get();
            ASSERT_EQ(expectedIface, actualIface);

            const auto& expectedProperty =
                std::get<1>(expectedGroups[i][j]).get();
            const auto& actualProperty = std::get<1>(groups[i][j]).get();
            ASSERT_EQ(expectedProperty, actualProperty);

            const auto& expectedMeta = std::get<1>(expectedGroups[i][j]).get();
            const auto& actualMeta = std::get<1>(groups[i][j]).get();
            ASSERT_EQ(expectedMeta, actualMeta);
        }
    }
}

TEST(PropertyGenTest, TypesSameContent)
{
    size_t i;
    for (i = 0; i < expectedTypes.size(); ++i)
    {
        ASSERT_EQ(expectedTypes[i], types[i]);
    }
}
