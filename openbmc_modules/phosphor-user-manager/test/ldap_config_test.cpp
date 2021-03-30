#include "config.h"
#include "phosphor-ldap-config/ldap_config.hpp"
#include "phosphor-ldap-config/ldap_config_mgr.hpp"

#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>
#include <sdbusplus/bus.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <sys/types.h>

namespace phosphor
{
namespace ldap
{
namespace fs = std::filesystem;
namespace ldap_base = sdbusplus::xyz::openbmc_project::User::Ldap::server;
using NotAllowed = sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
using NotAllowedArgument = xyz::openbmc_project::Common::NotAllowed;

using Config = phosphor::ldap::Config;
static constexpr const char* dbusPersistFile = "Config";
using PrivilegeMappingExists = sdbusplus::xyz::openbmc_project::User::Common::
    Error::PrivilegeMappingExists;

class TestLDAPConfig : public testing::Test
{
  public:
    TestLDAPConfig() : bus(sdbusplus::bus::new_default())
    {
    }
    void SetUp() override
    {
        using namespace phosphor::ldap;
        char tmpldap[] = "/tmp/ldap_test.XXXXXX";
        dir = fs::path(mkdtemp(tmpldap));
        fs::path tlsCacertFilePath{TLS_CACERT_PATH};
        tlsCacertFile = tlsCacertFilePath.filename().c_str();
        fs::path tlsCertFilePath{TLS_CERT_FILE};
        tlsCertFile = tlsCertFilePath.filename().c_str();

        fs::path confFilePath{LDAP_CONFIG_FILE};
        ldapconfFile = confFilePath.filename().c_str();
        std::fstream fs;
        fs.open(dir / defaultNslcdFile, std::fstream::out);
        fs.close();
        fs.open(dir / nsSwitchFile, std::fstream::out);
        fs.close();
        fs.open(dir / tlsCacertFile, std::fstream::out);
        fs.close();
        fs.open(dir / tlsCertFile, std::fstream::out);
        fs.close();
    }

    void TearDown() override
    {
        fs::remove_all(dir);
    }

  protected:
    fs::path dir;
    std::string tlsCacertFile;
    std::string tlsCertFile;
    std::string ldapconfFile;
    sdbusplus::bus::bus bus;
};

class MockConfigMgr : public phosphor::ldap::ConfigMgr
{
  public:
    MockConfigMgr(sdbusplus::bus::bus& bus, const char* path,
                  const char* filePath, const char* dbusPersistentFile,
                  const char* caCertFile, const char* certFile) :
        phosphor::ldap::ConfigMgr(bus, path, filePath, dbusPersistentFile,
                                  caCertFile, certFile)
    {
    }
    MOCK_METHOD1(restartService, void(const std::string& service));
    MOCK_METHOD1(stopService, void(const std::string& service));
    std::unique_ptr<Config>& getOpenLdapConfigPtr()
    {
        return openLDAPConfigPtr;
    }

    std::string configBindPassword()
    {
        return getADConfigPtr()->lDAPBindPassword;
    }

    std::unique_ptr<Config>& getADConfigPtr()
    {
        return ADConfigPtr;
    }
    void restore()
    {
        phosphor::ldap::ConfigMgr::restore();
        return;
    }

    void createDefaultObjects()
    {
        phosphor::ldap::ConfigMgr::createDefaultObjects();
    }

    bool secureLDAP()
    {
        return ADConfigPtr->secureLDAP;
    }

    friend class TestLDAPConfig;
};

TEST_F(TestLDAPConfig, testCreate)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());

    EXPECT_CALL(manager, stopService("nslcd.service")).Times(2);
    EXPECT_CALL(manager, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(manager, restartService("nscd.service")).Times(2);

    manager.createConfig(
        "ldap://9.194.251.136/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "uid", "gid");
    manager.getADConfigPtr()->enabled(true);

    manager.createConfig("ldap://9.194.251.137/", "cn=Users",
                         "cn=Users,dc=test", "MyLdap123",
                         ldap_base::Create::SearchScope::sub,
                         ldap_base::Create::Type::OpenLdap, "uid", "gid");
    manager.getOpenLdapConfigPtr()->enabled(false);

    // Below setting of username/groupname attr is to make sure
    // that in-active config should not call the start/stop service.
    manager.getOpenLdapConfigPtr()->userNameAttribute("abc");
    EXPECT_EQ(manager.getOpenLdapConfigPtr()->userNameAttribute(), "abc");

    manager.getOpenLdapConfigPtr()->groupNameAttribute("def");
    EXPECT_EQ(manager.getOpenLdapConfigPtr()->groupNameAttribute(), "def");

    EXPECT_TRUE(fs::exists(configFilePath));
    EXPECT_EQ(manager.getADConfigPtr()->lDAPServerURI(),
              "ldap://9.194.251.136/");
    EXPECT_EQ(manager.getADConfigPtr()->lDAPBindDN(), "cn=Users,dc=com");
    EXPECT_EQ(manager.getADConfigPtr()->lDAPBaseDN(), "cn=Users,dc=corp");
    EXPECT_EQ(manager.getADConfigPtr()->lDAPSearchScope(),
              ldap_base::Config::SearchScope::sub);
    EXPECT_EQ(manager.getADConfigPtr()->lDAPType(),
              ldap_base::Config::Type::ActiveDirectory);

    EXPECT_EQ(manager.getADConfigPtr()->userNameAttribute(), "uid");
    EXPECT_EQ(manager.getADConfigPtr()->groupNameAttribute(), "gid");
    EXPECT_EQ(manager.getADConfigPtr()->lDAPBindDNPassword(), "");
    EXPECT_EQ(manager.configBindPassword(), "MyLdap12");
    // change the password
    manager.getADConfigPtr()->lDAPBindDNPassword("MyLdap14");
    EXPECT_EQ(manager.getADConfigPtr()->lDAPBindDNPassword(), "");
    EXPECT_EQ(manager.configBindPassword(), "MyLdap14");
}

TEST_F(TestLDAPConfig, testDefaultObject)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));

    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());

    manager.createDefaultObjects();

    EXPECT_NE(nullptr, manager.getADConfigPtr());
    EXPECT_NE(nullptr, manager.getOpenLdapConfigPtr());
    EXPECT_EQ(manager.getADConfigPtr()->lDAPType(),
              ldap_base::Config::Type::ActiveDirectory);
    EXPECT_EQ(manager.getOpenLdapConfigPtr()->lDAPType(),
              ldap_base::Config::Type::OpenLdap);
}

TEST_F(TestLDAPConfig, testRestores)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "uid", "gid");
    managerPtr->getADConfigPtr()->enabled(false);
    EXPECT_FALSE(fs::exists(configFilePath));
    EXPECT_FALSE(managerPtr->getADConfigPtr()->enabled());
    managerPtr->getADConfigPtr()->enabled(true);

    EXPECT_TRUE(fs::exists(configFilePath));
    // Restore from configFilePath
    managerPtr->restore();
    // validate restored properties
    EXPECT_TRUE(managerPtr->getADConfigPtr()->enabled());
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPServerURI(),
              "ldap://9.194.251.138/");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBindDN(), "cn=Users,dc=com");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBaseDN(), "cn=Users,dc=corp");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPSearchScope(),
              ldap_base::Config::SearchScope::sub);
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPType(),
              ldap_base::Config::Type::ActiveDirectory);
    EXPECT_EQ(managerPtr->getADConfigPtr()->userNameAttribute(), "uid");
    EXPECT_EQ(managerPtr->getADConfigPtr()->groupNameAttribute(), "gid");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBindDNPassword(), "");
    EXPECT_EQ(managerPtr->configBindPassword(), "MyLdap12");
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testLDAPServerURI)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());

    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);

    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);

    // Change LDAP Server URI
    managerPtr->getADConfigPtr()->lDAPServerURI("ldap://9.194.251.139/");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPServerURI(),
              "ldap://9.194.251.139/");

    fs::remove(tlsCacertfile.c_str());
    // Change LDAP Server URI to make it secure
    EXPECT_THROW(
        managerPtr->getADConfigPtr()->lDAPServerURI("ldaps://9.194.251.139/"),
        NoCACertificate);

    // check once again
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPServerURI(),
              "ldap://9.194.251.139/");

    managerPtr->restore();
    // Check LDAP Server URI
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPServerURI(),
              "ldap://9.194.251.139/");
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testLDAPBindDN)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());

    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);

    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);

    // Change LDAP BindDN
    managerPtr->getADConfigPtr()->lDAPBindDN(
        "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBindDN(),
              "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    // Change LDAP BindDN
    EXPECT_THROW(
        {
            try
            {
                managerPtr->getADConfigPtr()->lDAPBindDN("");
            }
            catch (const InvalidArgument& e)
            {
                throw;
            }
        },
        InvalidArgument);

    managerPtr->restore();
    // Check LDAP BindDN after restoring
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBindDN(),
              "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testLDAPBaseDN)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);
    // Change LDAP BaseDN
    managerPtr->getADConfigPtr()->lDAPBaseDN(
        "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBaseDN(),
              "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    // Change LDAP BaseDN
    EXPECT_THROW(
        {
            try
            {
                managerPtr->getADConfigPtr()->lDAPBaseDN("");
            }
            catch (const InvalidArgument& e)
            {
                throw;
            }
        },
        InvalidArgument);

    managerPtr->restore();
    // Check LDAP BaseDN after restoring
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPBaseDN(),
              "cn=Administrator,cn=Users,dc=corp,dc=ibm,dc=com");
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testSearchScope)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);

    // Change LDAP SearchScope
    managerPtr->getADConfigPtr()->lDAPSearchScope(
        ldap_base::Config::SearchScope::one);
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPSearchScope(),
              ldap_base::Config::SearchScope::one);

    managerPtr->restore();
    // Check LDAP SearchScope after restoring
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPSearchScope(),
              ldap_base::Config::SearchScope::one);
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testLDAPType)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);

    // Change LDAP type
    // will not be changed
    EXPECT_THROW(managerPtr->getADConfigPtr()->lDAPType(
                     ldap_base::Config::Type::OpenLdap),
                 NotAllowed);
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPType(),
              ldap_base::Config::Type::ActiveDirectory);

    managerPtr->restore();
    // Check LDAP type after restoring
    EXPECT_EQ(managerPtr->getADConfigPtr()->lDAPType(),
              ldap_base::Config::Type::ActiveDirectory);
    delete managerPtr;
}

TEST_F(TestLDAPConfig, testsecureLDAPRestore)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldaps://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);
    EXPECT_TRUE(managerPtr->secureLDAP());
    managerPtr->restore();
    // Check secureLDAP variable value after restoring
    EXPECT_TRUE(managerPtr->secureLDAP());

    delete managerPtr;
}

TEST_F(TestLDAPConfig, filePermission)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(1);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(1);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");
    managerPtr->getADConfigPtr()->enabled(true);

    // Permission of the persistent file should be 640
    // Others should not be allowed to read.
    auto permission =
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read;
    auto persistFilepath = std::string(dir.c_str());
    persistFilepath += ADDbusObjectPath;
    persistFilepath += "/config";

    EXPECT_EQ(fs::status(persistFilepath).permissions(), permission);
    delete managerPtr;
}

TEST_F(TestLDAPConfig, ConditionalEnableConfig)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr* managerPtr =
        new MockConfigMgr(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    EXPECT_CALL(*managerPtr, stopService("nslcd.service")).Times(3);
    EXPECT_CALL(*managerPtr, restartService("nslcd.service")).Times(2);
    EXPECT_CALL(*managerPtr, restartService("nscd.service")).Times(2);
    managerPtr->createConfig(
        "ldap://9.194.251.138/", "cn=Users,dc=com", "cn=Users,dc=corp",
        "MyLdap12", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::ActiveDirectory, "attr1", "attr2");

    managerPtr->createConfig(
        "ldap://9.194.251.139/", "cn=Users,dc=com, dc=ldap", "cn=Users,dc=corp",
        "MyLdap123", ldap_base::Create::SearchScope::sub,
        ldap_base::Create::Type::OpenLdap, "attr1", "attr2");

    // Enable the AD configuration
    managerPtr->getADConfigPtr()->enabled(true);

    EXPECT_EQ(managerPtr->getADConfigPtr()->enabled(), true);
    EXPECT_EQ(managerPtr->getOpenLdapConfigPtr()->enabled(), false);

    // AS AD is already enabled so openldap can't be enabled.
    EXPECT_THROW(
        {
            try
            {
                managerPtr->getOpenLdapConfigPtr()->enabled(true);
            }
            catch (const NotAllowed& e)
            {
                throw;
            }
        },
        NotAllowed);
    // Check the values
    EXPECT_EQ(managerPtr->getADConfigPtr()->enabled(), true);
    EXPECT_EQ(managerPtr->getOpenLdapConfigPtr()->enabled(), false);
    // Let's disable the AD.
    managerPtr->getADConfigPtr()->enabled(false);
    EXPECT_EQ(managerPtr->getADConfigPtr()->enabled(), false);
    EXPECT_EQ(managerPtr->getOpenLdapConfigPtr()->enabled(), false);
    // Now enable the openldap
    managerPtr->getOpenLdapConfigPtr()->enabled(true);
    EXPECT_EQ(managerPtr->getOpenLdapConfigPtr()->enabled(), true);
    EXPECT_EQ(managerPtr->getADConfigPtr()->enabled(), false);

    delete managerPtr;
}

TEST_F(TestLDAPConfig, createPrivMapping)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    manager.createDefaultObjects();
    // Create the priv-mapping under the config.
    manager.getADConfigPtr()->create("admin", "priv-admin");
    // Check whether the entry has been created.
    EXPECT_THROW(
        {
            try
            {
                manager.getADConfigPtr()->checkPrivilegeMapper("admin");
            }
            catch (const PrivilegeMappingExists& e)
            {
                throw;
            }
        },
        PrivilegeMappingExists);
}

TEST_F(TestLDAPConfig, deletePrivMapping)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    manager.createDefaultObjects();
    // Create the priv-mapping under the config.
    manager.getADConfigPtr()->create("admin", "priv-admin");
    manager.getADConfigPtr()->create("user", "priv-user");
    // Check whether the entry has been created.
    EXPECT_THROW(
        {
            try
            {
                manager.getADConfigPtr()->checkPrivilegeMapper("admin");
                manager.getADConfigPtr()->checkPrivilegeMapper("user");
            }
            catch (const PrivilegeMappingExists& e)
            {
                throw;
            }
        },
        PrivilegeMappingExists);

    // This would delete the admin privilege
    manager.getADConfigPtr()->deletePrivilegeMapper(1);
    EXPECT_NO_THROW(manager.getADConfigPtr()->checkPrivilegeMapper("admin"));
    manager.getADConfigPtr()->deletePrivilegeMapper(2);
    EXPECT_NO_THROW(manager.getADConfigPtr()->checkPrivilegeMapper("user"));
}

TEST_F(TestLDAPConfig, restorePrivMapping)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    manager.createDefaultObjects();
    // Create the priv-mapping under the config.
    manager.getADConfigPtr()->create("admin", "priv-admin");
    manager.getOpenLdapConfigPtr()->create("user", "priv-user");
    manager.restore();
    EXPECT_THROW(
        {
            try
            {
                manager.getADConfigPtr()->checkPrivilegeMapper("admin");
            }
            catch (const PrivilegeMappingExists& e)
            {
                throw;
            }
        },
        PrivilegeMappingExists);

    EXPECT_THROW(
        {
            try
            {
                manager.getOpenLdapConfigPtr()->checkPrivilegeMapper("user");
            }
            catch (const PrivilegeMappingExists& e)
            {
                throw;
            }
        },
        PrivilegeMappingExists);
}

TEST_F(TestLDAPConfig, testPrivileges)
{
    auto configFilePath = std::string(dir.c_str()) + "/" + ldapconfFile;
    auto tlsCacertfile = std::string(dir.c_str()) + "/" + tlsCacertFile;
    auto tlsCertfile = std::string(dir.c_str()) + "/" + tlsCertFile;
    auto dbusPersistentFilePath = std::string(dir.c_str());

    if (fs::exists(configFilePath))
    {
        fs::remove(configFilePath);
    }
    EXPECT_FALSE(fs::exists(configFilePath));
    MockConfigMgr manager(bus, LDAP_CONFIG_ROOT, configFilePath.c_str(),
                          dbusPersistentFilePath.c_str(), tlsCacertfile.c_str(),
                          tlsCertfile.c_str());
    manager.createDefaultObjects();

    std::string groupName = "admin";
    std::string privilege = "priv-admin";
    size_t entryId = 1;
    auto dbusPath = std::string(LDAP_CONFIG_ROOT) +
                    "/active_directory/role_map/" + std::to_string(entryId);
    dbusPersistentFilePath += dbusPath;

    auto entry = std::make_unique<LDAPMapperEntry>(
        bus, dbusPath.c_str(), dbusPersistentFilePath.c_str(), groupName,
        privilege, *(manager.getADConfigPtr()));

    EXPECT_NO_THROW(entry->privilege("priv-operator"));
    EXPECT_NO_THROW(entry->privilege("priv-user"));
}

} // namespace ldap
} // namespace phosphor
