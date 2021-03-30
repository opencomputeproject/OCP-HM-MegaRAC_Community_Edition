#include <gtest/gtest.h>

#include <xyz/openbmc_project/Common/error.hpp>

#include "snmp_conf_manager.hpp"
#include "snmp_serialize.hpp"

namespace phosphor
{
namespace network
{
namespace snmp
{

using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;

auto mgrObjPath = "/xyz/openbmc_test/snmp/manager";

class TestSNMPClient : public testing::Test
{
  public:
    sdbusplus::bus::bus bus;
    ConfManager manager;
    std::string confDir;
    TestSNMPClient() :
        bus(sdbusplus::bus::new_default()), manager(bus, mgrObjPath)
    {
        char tmp[] = "/tmp/snmpClient.XXXXXX";
        std::string confDir = mkdtemp(tmp);
        manager.dbusPersistentLocation = confDir;
    }

    ~TestSNMPClient()
    {
        fs::remove_all(manager.dbusPersistentLocation);
    }
};

TEST_F(TestSNMPClient, UpdateProperty)
{
    std::string objPath = mgrObjPath;
    objPath += "/" + std::to_string(1);
    Client client(bus, objPath.c_str(), manager, "1.1.1.1", 202);
    EXPECT_EQ(client.address(), "1.1.1.1");
    EXPECT_EQ(client.port(), 202);
    client.address("2.2.2.2");
    EXPECT_EQ(client.address(), "2.2.2.2");
    client.port(404);
    EXPECT_EQ(client.port(), 404);
}

TEST_F(TestSNMPClient, AddEmptyAddress)
{
    std::string objPath = mgrObjPath;
    objPath += "/" + std::to_string(1);
    Client client(bus, objPath.c_str(), manager, "1.1.1.1", 202);
    EXPECT_EQ(client.address(), "1.1.1.1");
    EXPECT_EQ(client.port(), 202);

    EXPECT_THROW(client.address(""), InvalidArgument);
}

TEST_F(TestSNMPClient, CheckPersistency)
{
    std::string objPath = mgrObjPath;
    objPath += "/" + std::to_string(1);

    Client client(bus, objPath.c_str(), manager, "1.1.1.1", 23);
    client.address("2.2.2.2");

    Client restoreClient(bus, objPath.c_str(), manager);
    auto persistentPath = manager.dbusPersistentLocation;
    persistentPath += "/1";
    deserialize(persistentPath, restoreClient);

    EXPECT_EQ("2.2.2.2", restoreClient.address());
    EXPECT_EQ(23, restoreClient.port());
}

} // namespace snmp
} // namespace network
} // namespace phosphor
