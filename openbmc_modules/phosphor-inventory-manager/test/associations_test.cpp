#include "association_manager.hpp"

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

using namespace phosphor::inventory::manager::associations;
namespace fs = std::filesystem;

static const auto goodJson = R"(
[
    {
        "path": "system/PS0",
        "endpoints":
        [
            {
                "types":
                {
                    "rType": "inventory",
                    "fType": "sensors"
                },
                "paths":
                [
                    "power/ps0_input_power",
                    "voltage/ps0_input_voltage",
                    "current/ps0_output_current",
                    "voltage/ps0_output_voltage"
                ]
            },
            {
                "types":
                {
                    "rType": "inventory",
                    "fType": "fans"
                },
                "paths":
                [
                    "fan_tach/ps0_fan"
                ]
            }
        ]
    },
    {
        "path": "system/fan42",
        "endpoints":
        [
            {
                "types":
                {
                    "rType": "inventory",
                    "fType": "sensors"
                },
                "paths":
                [
                    "fan_tach/fan42"
                ]
            },
            {
                "types":
                {
                    "rType": "inventory",
                    "fType": "led"
                },
                "paths":
                [
                    "led/fan42"
                ]
            }
        ]
    }
])";

// Malformed JSON
static const auto badJson0 = R"(
    "hello": world
})";

// Uses 'blah' instead of 'paths'
static const auto badJson1 = R"(
[
    {
        "blah": "system/PS0",
        "endpoints":
        [
            {
                "types":
                {
                    "fType": "inventory",
                    "rType": "sensors"
                },
                "paths":
                [
                    "ps0_input_power",
                ]
            }
        ]
    }
])";

// Uses 'blah' instead of 'rType'
static const auto badJson2 = R"(
[
    {
        "paths": "system/PS0",
        "endpoints":
        [
            {
                "types":
                {
                    "blah": "inventory",
                    "fType": "sensors"
                },
                "paths":
                [
                    "ps0_input_power",
                ]
            }
        ]
    }
])";

// Missing the endpoints/paths array
static const auto badJson3 = R"(
[
    {
        "paths": "system/PS0",
        "endpoints":
        [
            {
                "types":
                {
                    "rType": "inventory",
                    "fType": "sensors"
                }
            }
        ]
    }
])";

class AssocsTest : public ::testing::Test
{
  protected:
    AssocsTest() : ::testing::Test(), bus(sdbusplus::bus::new_default())
    {
    }

    fs::path jsonDir;
    sdbusplus::bus::bus bus;

    virtual void SetUp()
    {
        char dir[] = {"assocTestXXXXXX"};
        jsonDir = mkdtemp(dir);
    }

    virtual void TearDown()
    {
        fs::remove_all(jsonDir);
    }

    std::string writeFile(const char* data)
    {
        fs::path path = jsonDir / "associations.json";

        std::ofstream f{path};
        f << data;
        f.close();

        return path;
    }
};

TEST_F(AssocsTest, TEST_NO_JSON)
{
    try
    {
        Manager m{bus};
        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}

TEST_F(AssocsTest, TEST_GOOD_JSON)
{
    auto path = writeFile(goodJson);
    Manager m(bus, path);

    const auto& a = m.getAssociationsConfig();
    EXPECT_EQ(a.size(), 2);

    {
        auto x = a.find("/xyz/openbmc_project/inventory/system/PS0");
        EXPECT_NE(x, a.end());

        auto& endpoints = x->second;
        EXPECT_EQ(endpoints.size(), 2);

        {
            auto& types = std::get<0>(endpoints[0]);
            EXPECT_EQ(std::get<0>(types), "sensors");
            EXPECT_EQ(std::get<1>(types), "inventory");

            auto& paths = std::get<1>(endpoints[0]);
            EXPECT_EQ(paths.size(), 4);
        }
        {
            auto& types = std::get<0>(endpoints[1]);
            EXPECT_EQ(std::get<0>(types), "fans");
            EXPECT_EQ(std::get<1>(types), "inventory");

            auto& paths = std::get<1>(endpoints[1]);
            EXPECT_EQ(paths.size(), 1);
        }
    }
    {
        auto x = a.find("/xyz/openbmc_project/inventory/system/fan42");
        EXPECT_NE(x, a.end());

        auto& endpoints = x->second;
        EXPECT_EQ(endpoints.size(), 2);

        {
            auto& types = std::get<0>(endpoints[0]);
            EXPECT_EQ(std::get<0>(types), "sensors");
            EXPECT_EQ(std::get<1>(types), "inventory");

            auto& paths = std::get<1>(endpoints[0]);
            EXPECT_EQ(paths.size(), 1);
        }
        {
            auto& types = std::get<0>(endpoints[1]);
            EXPECT_EQ(std::get<0>(types), "led");
            EXPECT_EQ(std::get<1>(types), "inventory");

            auto& paths = std::get<1>(endpoints[1]);
            EXPECT_EQ(paths.size(), 1);
        }
    }
}

TEST_F(AssocsTest, TEST_BAD_JSON0)
{
    auto path = writeFile(badJson0);

    try
    {
        Manager m(bus, path);

        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}

TEST_F(AssocsTest, TEST_BAD_JSON1)
{
    auto path = writeFile(badJson1);

    try
    {
        Manager m(bus, path);

        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}

TEST_F(AssocsTest, TEST_BAD_JSON2)
{
    auto path = writeFile(badJson2);

    try
    {
        Manager m(bus, path);

        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}

TEST_F(AssocsTest, TEST_BAD_JSON3)
{
    auto path = writeFile(badJson3);

    try
    {
        Manager m(bus, path);

        EXPECT_TRUE(false);
    }
    catch (std::exception& e)
    {
    }
}
