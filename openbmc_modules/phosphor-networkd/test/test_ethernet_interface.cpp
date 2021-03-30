#include "config_parser.hpp"
#include "ipaddress.hpp"
#include "mock_network_manager.hpp"
#include "mock_syscall.hpp"
#include "util.hpp"

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>

#include <exception>
#include <fstream>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{

class TestEthernetInterface : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    MockManager manager;
    MockEthernetInterface interface;
    std::string confDir;
    TestEthernetInterface() :
        bus(sdbusplus::bus::new_default()),
        manager(bus, "/xyz/openbmc_test/network", "/tmp/"),
        interface(makeInterface(bus, manager))

    {
        setConfDir();
    }

    void setConfDir()
    {
        char tmp[] = "/tmp/EthernetInterface.XXXXXX";
        confDir = mkdtemp(tmp);
        manager.setConfDir(confDir);
    }

    ~TestEthernetInterface()
    {
        if (confDir != "")
        {
            fs::remove_all(confDir);
        }
    }

    static constexpr ether_addr mac{0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    static MockEthernetInterface makeInterface(sdbusplus::bus::bus& bus,
                                               MockManager& manager)
    {
        mock_clear();
        mock_addIF("test0", 1, mac);
        return {bus, "/xyz/openbmc_test/network/test0",
                EthernetInterface::DHCPConf::none, manager, true};
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

    std::string getObjectPath(const std::string& ipaddress, uint8_t subnetMask,
                              const std::string& gateway)
    {
        IP::Protocol addressType = IP::Protocol::IPv4;

        return interface.generateObjectPath(addressType, ipaddress, subnetMask,
                                            gateway);
    }

    void createIPObject(IP::Protocol addressType, const std::string& ipaddress,
                        uint8_t subnetMask, const std::string& gateway)
    {
        interface.iP(addressType, ipaddress, subnetMask, gateway);
    }
};

TEST_F(TestEthernetInterface, NoIPaddress)
{
    EXPECT_EQ(countIPObjects(), 0);
    EXPECT_EQ(mac_address::toString(mac), interface.mACAddress());
}

TEST_F(TestEthernetInterface, AddIPAddress)
{
    IP::Protocol addressType = IP::Protocol::IPv4;
    createIPObject(addressType, "10.10.10.10", 16, "10.10.10.1");
    EXPECT_EQ(true, isIPObjectExist("10.10.10.10"));
}

TEST_F(TestEthernetInterface, AddMultipleAddress)
{
    IP::Protocol addressType = IP::Protocol::IPv4;
    createIPObject(addressType, "10.10.10.10", 16, "10.10.10.1");
    createIPObject(addressType, "20.20.20.20", 16, "20.20.20.1");
    EXPECT_EQ(true, isIPObjectExist("10.10.10.10"));
    EXPECT_EQ(true, isIPObjectExist("20.20.20.20"));
}

TEST_F(TestEthernetInterface, DeleteIPAddress)
{
    IP::Protocol addressType = IP::Protocol::IPv4;
    createIPObject(addressType, "10.10.10.10", 16, "10.10.10.1");
    createIPObject(addressType, "20.20.20.20", 16, "20.20.20.1");
    deleteIPObject("10.10.10.10");
    EXPECT_EQ(false, isIPObjectExist("10.10.10.10"));
    EXPECT_EQ(true, isIPObjectExist("20.20.20.20"));
}

TEST_F(TestEthernetInterface, DeleteInvalidIPAddress)
{
    EXPECT_EQ(false, deleteIPObject("10.10.10.10"));
}

TEST_F(TestEthernetInterface, CheckObjectPath)
{
    std::string ipaddress = "10.10.10.10";
    uint8_t prefix = 16;
    std::string gateway = "10.10.10.1";

    std::string expectedObjectPath = "/xyz/openbmc_test/network/test0/ipv4/";
    std::stringstream hexId;

    std::string hashString = ipaddress;
    hashString += std::to_string(prefix);
    hashString += gateway;

    hexId << std::hex << ((std::hash<std::string>{}(hashString)) & 0xFFFFFFFF);
    expectedObjectPath += hexId.str();

    EXPECT_EQ(expectedObjectPath, getObjectPath(ipaddress, prefix, gateway));
}

TEST_F(TestEthernetInterface, addStaticNameServers)
{
    ServerList servers = {"9.1.1.1", "9.2.2.2", "9.3.3.3"};
    EXPECT_CALL(manager, restartSystemdUnit(networkdService)).Times(1);
    interface.staticNameServers(servers);
    fs::path filePath = confDir;
    filePath /= "00-bmc-test0.network";
    config::Parser parser(filePath.string());
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;
    std::tie(rc, values) = parser.getValues("Network", "DNS");
    EXPECT_EQ(servers, values);
}

TEST_F(TestEthernetInterface, addDynamicNameServers)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::Error;
    ServerList servers = {"9.1.1.1", "9.2.2.2", "9.3.3.3"};
    EXPECT_THROW(interface.nameservers(servers), NotAllowed);
}

TEST_F(TestEthernetInterface, getDynamicNameServers)
{
    ServerList servers = {"9.1.1.1", "9.2.2.2", "9.3.3.3"};
    EXPECT_CALL(interface, getNameServerFromResolvd())
        .WillRepeatedly(testing::Return(servers));
    EXPECT_EQ(interface.getNameServerFromResolvd(), servers);
}

TEST_F(TestEthernetInterface, addNTPServers)
{
    ServerList servers = {"10.1.1.1", "10.2.2.2", "10.3.3.3"};
    EXPECT_CALL(manager, restartSystemdUnit(networkdService)).Times(1);
    interface.nTPServers(servers);
    fs::path filePath = confDir;
    filePath /= "00-bmc-test0.network";
    config::Parser parser(filePath.string());
    config::ReturnCode rc = config::ReturnCode::SUCCESS;
    config::ValueList values;
    std::tie(rc, values) = parser.getValues("Network", "NTP");
    EXPECT_EQ(servers, values);
}

} // namespace network
} // namespace phosphor
