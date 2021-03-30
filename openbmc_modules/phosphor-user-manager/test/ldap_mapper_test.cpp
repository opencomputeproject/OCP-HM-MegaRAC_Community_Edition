#include <gtest/gtest.h>
#include <experimental/filesystem>
#include <stdlib.h>
#include <sdbusplus/bus.hpp>
#include "phosphor-ldap-mapper/ldap_mapper_entry.hpp"
#include "phosphor-ldap-mapper/ldap_mapper_mgr.hpp"
#include "phosphor-ldap-mapper/ldap_mapper_serialize.hpp"
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>
#include "config.h"

namespace phosphor
{
namespace user
{

namespace fs = std::experimental::filesystem;

class TestSerialization : public testing::Test
{
  public:
    TestSerialization() : bus(sdbusplus::bus::new_default())
    {
    }

    void SetUp() override
    {
        char tempDir[] = "/tmp/privmapper_test.XXXXXX";
        dir = fs::path(mkdtemp(tempDir));
    }

    void TearDown() override
    {
        fs::remove_all(dir);
    }

    fs::path dir;
    sdbusplus::bus::bus bus;
};

TEST_F(TestSerialization, testPersistPath)
{
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          TestSerialization::dir.c_str());
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    size_t entryId = 1;
    auto dbusPath = std::string(mapperMgrRoot) + '/' + std::to_string(entryId);

    auto entry = std::make_unique<LDAPMapperEntry>(
        TestSerialization::bus, dbusPath.c_str(),
        (TestSerialization::dir).c_str(), groupName, privilege, manager);
    auto outPath = serialize(*entry, entryId, TestSerialization::dir);
    EXPECT_EQ(outPath, TestSerialization::dir / std::to_string(entryId));
}

TEST_F(TestSerialization, testPersistData)
{
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    size_t entryId = 1;
    auto dbusPath = std::string(mapperMgrRoot) + '/' + std::to_string(entryId);
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          TestSerialization::dir.c_str());

    auto input = std::make_unique<LDAPMapperEntry>(
        bus, dbusPath.c_str(), TestSerialization::dir.c_str(), groupName,
        privilege, manager);
    auto outPath = serialize(*input, entryId, TestSerialization::dir);

    auto output = std::make_unique<LDAPMapperEntry>(
        bus, dbusPath.c_str(), (TestSerialization::dir).c_str(), manager);
    auto rc = deserialize(outPath, *output);

    EXPECT_EQ(rc, true);
    EXPECT_EQ(output->groupName(), groupName);
    EXPECT_EQ(output->privilege(), privilege);
}

TEST_F(TestSerialization, testRestore)
{
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    namespace fs = std::experimental::filesystem;
    size_t entryId = 1;
    LDAPMapperMgr manager1(TestSerialization::bus, mapperMgrRoot,
                           (TestSerialization::dir).c_str());
    EXPECT_NO_THROW(manager1.create(groupName, privilege));

    EXPECT_EQ(fs::exists(TestSerialization::dir / std::to_string(entryId)),
              true);
    LDAPMapperMgr manager2(TestSerialization::bus, mapperMgrRoot,
                           (TestSerialization::dir).c_str());
    EXPECT_NO_THROW(manager2.restore());
    EXPECT_NO_THROW(manager2.deletePrivilegeMapper(entryId));
    EXPECT_EQ(fs::exists(TestSerialization::dir / std::to_string(entryId)),
              false);
}

TEST_F(TestSerialization, testPrivilegeMapperCreation)
{
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          (TestSerialization::dir).c_str());
    EXPECT_NO_THROW(manager.create(groupName, privilege));
}

TEST_F(TestSerialization, testDuplicateGroupName)
{
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    using PrivilegeMappingExists = sdbusplus::xyz::openbmc_project::User::
        Common::Error::PrivilegeMappingExists;
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          (TestSerialization::dir).c_str());
    auto objectPath = manager.create(groupName, privilege);
    EXPECT_THROW(manager.create(groupName, privilege), PrivilegeMappingExists);
}

TEST_F(TestSerialization, testValidPrivilege)
{
    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    size_t entryId = 1;
    auto dbusPath = std::string(mapperMgrRoot) + '/' + std::to_string(entryId);
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          TestSerialization::dir.c_str());

    auto entry = std::make_unique<LDAPMapperEntry>(
        TestSerialization::bus, dbusPath.c_str(),
        (TestSerialization::dir).c_str(), groupName, privilege, manager);

    EXPECT_NO_THROW(entry->privilege("priv-operator"));
    EXPECT_NO_THROW(entry->privilege("priv-user"));
}

TEST_F(TestSerialization, testInvalidPrivilege)
{
    std::string groupName = "admin";
    std::string privilege = "priv-test";
    using InvalidArgument =
        sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
    LDAPMapperMgr manager(TestSerialization::bus, mapperMgrRoot,
                          (TestSerialization::dir).c_str());
    EXPECT_THROW(manager.create(groupName, privilege), InvalidArgument);
}

} // namespace user
} // namespace phosphor
