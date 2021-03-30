#include "common/utils.hpp"
#include "host-bmc/dbus_to_host_effecters.hpp"
#include "mocked_utils.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

#include <gtest/gtest.h>

using namespace pldm::host_effecters;
using namespace pldm::utils;
using namespace pldm::dbus_api;

class MockHostEffecterParser : public HostEffecterParser
{
  public:
    MockHostEffecterParser(int fd, const pldm_pdr* repo,
                           DBusHandler* const dbusHandler,
                           const std::string& jsonPath) :
        HostEffecterParser(nullptr, fd, repo, dbusHandler, jsonPath)
    {}

    MOCK_METHOD(void, setHostStateEffecter,
                (size_t, std::vector<set_effecter_state_field>&, uint16_t),
                (const override));

    MOCK_METHOD(void, createHostEffecterMatch,
                (const std::string&, const std::string&, size_t, size_t,
                 uint16_t),
                (const override));

    const std::vector<EffecterInfo>& gethostEffecterInfo()
    {
        return hostEffecterInfo;
    }
};

TEST(HostEffecterParser, parseEffecterJsonGoodPath)
{
    MockdBusHandler dbusHandler;
    int sockfd{};
    MockHostEffecterParser hostEffecterParserGood(sockfd, nullptr, &dbusHandler,
                                                  "./host_effecter_jsons/good");
    auto hostEffecterInfo = hostEffecterParserGood.gethostEffecterInfo();
    ASSERT_EQ(hostEffecterInfo.size(), 1);
    ASSERT_EQ(hostEffecterInfo[0].entityInstance, 0);
    ASSERT_EQ(hostEffecterInfo[0].entityType, 33);
    ASSERT_EQ(hostEffecterInfo[0].dbusInfo.size(), 1);
    DBusEffecterMapping dbusInfo{
        {"/xyz/openbmc_project/control/host0/boot",
         "xyz.openbmc_project.Control.Boot.Mode", "BootMode", "string"},
        {"xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"},
        {196, {2}}};
    auto& temp = hostEffecterInfo[0].dbusInfo[0];
    ASSERT_EQ(temp.dbusMap.objectPath == dbusInfo.dbusMap.objectPath, true);
    ASSERT_EQ(temp.dbusMap.interface == dbusInfo.dbusMap.interface, true);
    ASSERT_EQ(temp.dbusMap.propertyName == dbusInfo.dbusMap.propertyName, true);
    ASSERT_EQ(temp.dbusMap.propertyType == dbusInfo.dbusMap.propertyType, true);
}

TEST(HostEffecterParser, parseEffecterJsonBadPath)
{
    MockdBusHandler dbusHandler;
    int sockfd{};
    MockHostEffecterParser hostEffecterParser(sockfd, nullptr, &dbusHandler,
                                              "./host_effecter_jsons/no_json");
    ASSERT_THROW(
        hostEffecterParser.parseEffecterJson("./host_effecter_jsons/no_json"),
        std::exception);
    ASSERT_THROW(
        hostEffecterParser.parseEffecterJson("./host_effecter_jsons/malformed"),
        std::exception);
}

TEST(HostEffecterParser, findNewStateValue)
{
    MockdBusHandler dbusHandler;
    int sockfd{};
    MockHostEffecterParser hostEffecterParser(sockfd, nullptr, &dbusHandler,
                                              "./host_effecter_jsons/good");

    PropertyValue val1{std::in_place_type<std::string>,
                       "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"};
    PropertyValue val2{std::in_place_type<std::string>,
                       "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup"};
    auto newState = hostEffecterParser.findNewStateValue(0, 0, val1);
    ASSERT_EQ(newState, 2);

    ASSERT_THROW(hostEffecterParser.findNewStateValue(0, 0, val2),
                 std::exception);
}
