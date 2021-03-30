#include "config_parser.hpp"
#include "ipaddress.hpp"
#include "mock_network_manager.hpp"
#include "mock_syscall.hpp"
#include "vlan_interface.hpp"

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include <exception>
#include <filesystem>
#include <sdbusplus/bus.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{

namespace fs = std::filesystem;

class TestVlanInterface : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    MockManager manager;
    EthernetInterface interface;
    std::string confDir;
    TestVlanInterface() :
        bus(sdbusplus::bus::new_default()),
        manager(bus, "/xyz/openbmc_test/network", "/tmp"),
        interface(makeInterface(bus, manager))

    {
        setConfDir();
    }

    ~TestVlanInterface()
    {
        if (confDir != "")
        {
            fs::remove_all(confDir);
        }
    }

    static EthernetInterface makeInterface(sdbusplus::bus::bus& bus,
                                           MockManager& manager)
    {
        mock_clear();
        mock_addIF("test0", 1);
        return {bus, "/xyz/openbmc_test/network/test0",
                EthernetInterface::DHCPConf::none, manager};
    }

    void setConfDir()
    {
        char tmp[] = "/tmp/VlanInterface.XXXXXX";
        confDir = mkdtemp(tmp);
        manager.setConfDir(confDir);
    }

    void createVlan(VlanId id)
    {
        std::string ifname = "test0.";
        ifname += std::to_string(id);
        mock_addIF(ifname.c_str(), 1000 + id);
        interface.createVLAN(id);
    }

    void deleteVlan(const std::string& interfaceName)
    {
        interface.deleteVLANObject(interfaceName);
    }

    int countIPObjects()
    {
        return interface.getAddresses().size();
    }

    bool isIPObjectExist(const std::string& ipaddress)
    {
        auto address = interface.getAddresses().find(ipaddress);
        if (address == interface.getAddresses().end())
        {
            return false;
        }
        return true;
    }

    bool deleteIPObject(const std::string& ipaddress)
    {
        auto address = interface.getAddresses().find(ipaddress);
        if (address == interface.getAddresses().end())
        {
            return false;
        }
        address->second->delete_();
        return true;
    }

    void createIPObject(IP::Protocol addressType, const std::string& ipaddress,
                        uint8_t subnetMask, const std::string& gateway)
    {
        interface.iP(addressType, ipaddress, subnetMask, gateway);
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

TEST_F(TestVlanInterface, createVLAN)
{
    createVlan(50);
    fs::path filePath = confDir;
    filePath /= "test0.50.netdev";

    config::Parser parser(filePath.string());
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;

    std::tie(rc, values) = parser.getValues("NetDev", "Name");
    std::string expectedValue = "test0.50";
    bool found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("NetDev", "Kind");
    expectedValue = "vlan";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("VLAN", "Id");
    expectedValue = "50";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);
}

TEST_F(TestVlanInterface, deleteVLAN)
{
    createVlan(50);
    deleteVlan("test0.50");
    bool fileFound = false;

    fs::path filePath = confDir;
    filePath /= "test0.50.netdev";
    if (fs::is_regular_file(filePath.string()))
    {
        fileFound = true;
    }
    EXPECT_EQ(fileFound, false);
}

TEST_F(TestVlanInterface, createMultipleVLAN)
{
    createVlan(50);
    createVlan(60);

    fs::path filePath = confDir;
    filePath /= "test0.50.netdev";
    config::Parser parser(filePath.string());
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;

    std::tie(rc, values) = parser.getValues("NetDev", "Name");
    std::string expectedValue = "test0.50";
    bool found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("NetDev", "Kind");
    expectedValue = "vlan";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("VLAN", "Id");
    expectedValue = "50";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    filePath = confDir;
    filePath /= "test0.60.netdev";
    parser.setFile(filePath.string());
    std::tie(rc, values) = parser.getValues("NetDev", "Name");
    expectedValue = "test0.60";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    std::tie(rc, values) = parser.getValues("VLAN", "Id");
    expectedValue = "60";
    found = isValueFound(values, expectedValue);
    EXPECT_EQ(found, true);

    deleteVlan("test0.50");
    deleteVlan("test0.60");
}

} // namespace network
} // namespace phosphor
