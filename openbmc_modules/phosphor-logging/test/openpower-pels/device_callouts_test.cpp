/**
 * Copyright © 2020 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "extensions/openpower-pels/device_callouts.hpp"
#include "extensions/openpower-pels/paths.hpp"

#include <fstream>

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace openpower::pels::device_callouts;
namespace fs = std::filesystem;

// The callout JSON looks like:
// "I2C":
//   "<bus>":
//     "<address>":
//       "Callouts": ...
//
// "FSI":
//   "<fsi link>":
//     "Callouts": ...
//
// "FSI-I2C":
//    "<fsi link>":
//      "<bus>":
//        "<address>":
//          "Callouts": ...
//
// "FSI-SPI":
//    "<fsi link>":
//      "<bus>":
//        "Callouts": ...

const auto calloutJSON = R"(
{
    "I2C":
    {
        "0":
        {
            "32":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H"
                    }
                ],
                "Dest": "proc-0 target"
            },
            "81":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H"
                    }
                ],
                "Dest": "proc-0 target"
            },
            "90":
            {
                "Callouts":[
                    {
                       "Name": "This is missing the location code",
                       "Priority": "H"
                    }
                ],
                "Dest": "proc-0 target"
            }
        },
        "14":
        {
            "112":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H"
                    }
                ],
                "Dest": "proc-0 target"
            },
            "114":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H",
                       "MRU": "core0"
                    },
                    {
                       "Name": "/chassis/motherboard",
                       "LocationCode": "P1",
                       "Priority": "M"
                    }
                ],
                "Dest": "proc-0 target"
            }
        }
    },
    "FSI":
    {
        "0":
        {
           "Callouts":[
                {
                    "Name": "/chassis/motherboard/cpu0",
                    "LocationCode": "P1-C19",
                    "Priority": "H"
                }
           ],
           "Dest": "proc-0 target"
        },
        "0-1":
        {
           "Callouts":[
                {
                    "Name": "/chassis/motherboard/cpu0",
                    "LocationCode": "P1-C19",
                    "Priority": "H",
                    "MRU": "core"
                }
           ],
           "Dest": "proc-0 target"
        }
    },
    "FSI-I2C":
    {
        "0-3":
        {
           "7":
           {
              "24":
              {
                 "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H"
                    }
                 ],
                 "Dest": "proc-0 target"
              },
              "25":
              {
                 "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu5",
                       "LocationCode": "P1-C25",
                       "Priority": "H"
                    },
                    {
                       "Name": "/chassis/motherboard",
                       "LocationCode": "P1",
                       "Priority": "M"
                    },
                    {
                        "Name": "/chassis/motherboard/bmc",
                        "LocationCode": "P2",
                        "Priority": "L"
                    }
                 ],
                 "Dest": "proc-5 target"
              }
           }
        }
    },
    "FSI-SPI":
    {
        "8":
        {
            "3":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu0",
                       "LocationCode": "P1-C19",
                       "Priority": "H"
                    }
                ],
                "Dest": "proc-0 target"
            },
            "4":
            {
                "Callouts":[
                    {
                       "Name": "/chassis/motherboard/cpu2",
                       "LocationCode": "P1-C12",
                       "Priority": "M"
                    }
                ],
                "Dest": "proc-0 target"
            }
        }
    }
})"_json;

class DeviceCalloutsTest : public ::testing::Test
{
  public:
    static void SetUpTestCase()
    {
        dataPath = getPELReadOnlyDataPath();
        std::ofstream file{dataPath / filename};
        file << calloutJSON.dump();
    }

    static void TearDownTestCase()
    {
        fs::remove_all(dataPath);
    }

    static std::string filename;
    static fs::path dataPath;
};

std::string DeviceCalloutsTest::filename = "systemA_dev_callouts.json";
fs::path DeviceCalloutsTest::dataPath;

namespace openpower::pels::device_callouts
{

// Helpers to compair vectors of Callout objects
bool operator!=(const Callout& left, const Callout& right)
{
    return (left.priority != right.priority) ||
           (left.locationCode != right.locationCode) ||
           (left.name != right.name) || (left.mru != right.mru) ||
           (left.debug != right.debug);
}

bool operator==(const std::vector<Callout>& left,
                const std::vector<Callout>& right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (size_t i = 0; i < left.size(); i++)
    {
        if (left[i] != right[i])
        {
            return false;
        }
    }

    return true;
}

} // namespace openpower::pels::device_callouts

// Test looking up the JSON file based on the system compatible names
TEST_F(DeviceCalloutsTest, getJSONFilenameTest)
{
    {
        std::vector<std::string> compatibles{"system1", "systemA", "system3"};
        EXPECT_EQ(util::getJSONFilename(compatibles),
                  fs::path{dataPath / filename});
    }

    // Actual filename not in compatibles
    {
        std::vector<std::string> compatibles{"system5", "system6"};
        EXPECT_THROW(util::getJSONFilename(compatibles), std::invalid_argument);
    }
}

// Test determining the callout type from the device path
TEST_F(DeviceCalloutsTest, getCalloutTypeTest)
{

    // Invalid
    {
        EXPECT_EQ(util::getCalloutType("/some/bad/device/path"),
                  util::CalloutType::unknown);
    }

    // I2C
    {
        EXPECT_EQ(util::getCalloutType(
                      "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@1e78a000/"
                      "1e78a340.i2c-bus/i2c-14/14-0072"),
                  util::CalloutType::i2c);
    }

    // FSI
    {
        EXPECT_EQ(util::getCalloutType(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                      "slave@01:00/01:01:00:06/sbefifo2-dev0/occ-hwmon.2"),
                  util::CalloutType::fsi);
    }

    // FSI-I2C
    {
        EXPECT_EQ(util::getCalloutType(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                      "slave@01:00/01:01:00:03/i2c-211/211-0055"),
                  util::CalloutType::fsii2c);
    }

    // FSI-SPI
    {

        EXPECT_EQ(
            util::getCalloutType(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/fsi-master/"
                "fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/slave@08:00/"
                "01:03:00:04/spi_master/spi9/spi9.0/spi9.00/nvmem"),
            util::CalloutType::fsispi);
    }
}

// Test getting I2C search keys
TEST_F(DeviceCalloutsTest, getI2CSearchKeysTest)
{

    {
        EXPECT_EQ(util::getI2CSearchKeys(
                      "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@1e78a000/"
                      "1e78a340.i2c-bus/i2c-10/10-0022"),
                  (std::tuple{10, 0x22}));

        EXPECT_EQ(util::getI2CSearchKeys(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                      "slave@01:00/01:01:00:03/i2c-211/211-0055"),
                  (std::tuple{11, 0x55}));
    }

    {
        EXPECT_THROW(util::getI2CSearchKeys("/sys/some/bad/path"),
                     std::invalid_argument);
    }
}
//
// Test getting SPI search keys
TEST_F(DeviceCalloutsTest, getSPISearchKeysTest)
{
    {
        EXPECT_EQ(
            util::getSPISearchKeys(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/fsi-master/"
                "fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/slave@08:00/"
                "01:03:00:04/spi_master/spi9/spi9.0/spi9.00/nvmem"),
            9);
    }

    {
        EXPECT_THROW(util::getSPISearchKeys("/sys/some/bad/path"),
                     std::invalid_argument);
    }
}

// Test getting FSI search keys
TEST_F(DeviceCalloutsTest, getFSISearchKeysTest)
{
    {
        EXPECT_EQ(util::getFSISearchKeys(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:04/spi_master/spi2/"
                      "spi2.0/spi2.00/nvmem"),
                  "0");
    }

    {
        EXPECT_EQ(util::getFSISearchKeys(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                      "slave@01:00/01:01:00:06/sbefifo2-dev0/occ-hwmon.2"),
                  "0-1");
    }

    {
        EXPECT_EQ(
            util::getFSISearchKeys(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                "slave@01:00/01:01:00:0a:/fsi-master/slave@04:00/01:01:00:0a"),
            "0-1-4");
    }

    {
        EXPECT_THROW(util::getFSISearchKeys("/sys/some/bad/path"),
                     std::invalid_argument);
    }
}

// Test getting FSI-I2C search keys
TEST_F(DeviceCalloutsTest, getFSII2CSearchKeysTest)
{
    {
        // Link 0-1 bus 11 address 0x55
        EXPECT_EQ(util::getFSII2CSearchKeys(
                      "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                      "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                      "slave@01:00/01:01:00:03/i2c-211/211-0055"),
                  (std::tuple{"0-1", std::tuple{11, 0x55}}));
    }

    {
        EXPECT_THROW(util::getFSII2CSearchKeys("/sys/some/bad/path"),
                     std::invalid_argument);
    }
}

// Test getting FSI-SPI search keys
TEST_F(DeviceCalloutsTest, getFSISPISearchKeysTest)
{
    {
        // Link 0-8 SPI 9
        EXPECT_EQ(
            util::getFSISPISearchKeys(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/fsi-master/"
                "fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/slave@08:00/"
                "01:03:00:04/spi_master/spi9/spi9.0/spi9.00/nvmem"),
            (std::tuple{"0-8", 9}));
    }

    {
        EXPECT_THROW(util::getFSISPISearchKeys("/sys/some/bad/path"),
                     std::invalid_argument);
    }
}

TEST_F(DeviceCalloutsTest, getCalloutsTest)
{
    std::vector<std::string> systemTypes{"systemA", "systemB"};

    // A really bogus path
    {
        EXPECT_THROW(getCallouts("/bad/path", systemTypes),
                     std::invalid_argument);
    }

    // I2C
    {
        auto callouts = getCallouts(
            "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@1e78a000/"
            "1e78a340.i2c-bus/i2c-14/14-0072",
            systemTypes);

        std::vector<Callout> expected{
            {"H", "P1-C19", "/chassis/motherboard/cpu0", "core0",
             "I2C: bus: 14 address: 114 dest: proc-0 target"},
            {"M", "P1", "/chassis/motherboard", "", ""}};

        EXPECT_EQ(callouts, expected);

        // Use the bus/address API instead of the device path one
        callouts = getI2CCallouts(14, 0x72, systemTypes);
        EXPECT_EQ(callouts, expected);

        // I2C address not in JSON
        EXPECT_THROW(
            getCallouts(
                "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@1e78a000/"
                "1e78a340.i2c-bus/i2c-14/14-0099",
                systemTypes),
            std::invalid_argument);

        // A bad JSON entry, missing the location code
        EXPECT_THROW(
            getCallouts(
                "/sys/devices/platform/ahb/ahb:apb/ahb:apb:bus@1e78a000/"
                "1e78a340.i2c-bus/i2c-0/0-005a",
                systemTypes),
            std::runtime_error);
    }

    // FSI
    {
        auto callouts = getCallouts(
            "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
            "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
            "slave@01:00/01:01:00:06/sbefifo2-dev0/occ-hwmon.2",
            systemTypes);

        std::vector<Callout> expected{{"H", "P1-C19",
                                       "/chassis/motherboard/cpu0", "core",
                                       "FSI: links: 0-1 dest: proc-0 target"}};

        EXPECT_EQ(callouts, expected);

        // link 9-1 not in JSON
        EXPECT_THROW(
            getCallouts(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                "fsi-master/fsi0/slave@09:00/00:00:00:0a/fsi-master/fsi1/"
                "slave@01:00/01:01:00:06/sbefifo2-dev0/occ-hwmon.2",
                systemTypes),
            std::invalid_argument);
    }

    // FSI-I2C
    {
        auto callouts = getCallouts(
            "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
            "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
            "slave@03:00/01:01:00:03/i2c-207/207-0019",
            systemTypes);

        std::vector<Callout> expected{
            {"H", "P1-C25", "/chassis/motherboard/cpu5", "",
             "FSI-I2C: links: 0-3 bus: 7 addr: 25 dest: proc-5 target"},
            {"M", "P1", "/chassis/motherboard", "", ""},
            {"L", "P2", "/chassis/motherboard/bmc", "", ""}};

        EXPECT_EQ(callouts, expected);

        // Bus 2 not in JSON
        EXPECT_THROW(
            getCallouts(
                "/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                "fsi-master/fsi0/slave@00:00/00:00:00:0a/fsi-master/fsi1/"
                "slave@03:00/01:01:00:03/i2c-202/202-0019",
                systemTypes),
            std::invalid_argument);
    }

    // FSI-SPI
    {
        auto callouts =
            getCallouts("/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                        "fsi-master/fsi0/slave@08:00/00:00:00:04/spi_master/"
                        "spi3/spi3.0/spi3.00/nvmem",
                        systemTypes);

        std::vector<Callout> expected{
            {"H", "P1-C19", "/chassis/motherboard/cpu0", "",
             "FSI-SPI: links: 8 bus: 3 dest: proc-0 target"}};

        EXPECT_EQ(callouts, expected);

        // Bus 7 not in the JSON
        EXPECT_THROW(
            getCallouts("/sys/devices/platform/ahb/ahb:apb/1e79b000.fsi/"
                        "fsi-master/fsi0/slave@08:00/00:00:00:04/spi_master/"
                        "spi7/spi7.0/spi7.00/nvmem",
                        systemTypes),
            std::invalid_argument);
    }
}
