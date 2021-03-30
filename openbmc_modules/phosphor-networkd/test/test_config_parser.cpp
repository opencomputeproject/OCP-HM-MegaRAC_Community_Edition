#include "config.h"

#include "config_parser.hpp"

#include <exception>
#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <stdexcept>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{

class TestConfigParser : public testing::Test
{
  public:
    config::Parser parser;
    TestConfigParser()
    {
        remove("/tmp/eth0.network");
        std::ofstream filestream("/tmp/eth0.network");

        filestream << "[Match]\nName=eth0\n"
                   << "[Network]\nDHCP=true\n[DHCP]\nClientIdentifier= mac\n";
        filestream.close();
        parser.setFile("/tmp/eth0.network");
    }

    bool isValueFound(const std::vector<std::string>& values,
                      const std::string& expectedValue)
    {
        for (const auto& value : values)
        {
            if (expectedValue == value)
            {
                return true;
            }
        }
        return false;
    }
};

TEST_F(TestConfigParser, ReadConfigDataFromFile)
{
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;

    std::tie(rc, values) = parser.getValues("Network", "DHCP");
    std::string expectedValue = "true";
    bool found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("DHCP", "ClientIdentifier");
    expectedValue = "mac";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("Match", "Name");
    expectedValue = "eth0";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);
}

TEST_F(TestConfigParser, SectionNotExist)
{
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;
    std::tie(rc, values) = parser.getValues("abc", "ipaddress");
    EXPECT_EQ(config::ReturnCode::SECTION_NOT_FOUND, rc);
}

TEST_F(TestConfigParser, KeyNotFound)
{
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;
    std::tie(rc, values) = parser.getValues("Network", "abc");
    EXPECT_EQ(config::ReturnCode::KEY_NOT_FOUND, rc);
    remove("/tmp/eth0.network");
}

} // namespace network
} // namespace phosphor
