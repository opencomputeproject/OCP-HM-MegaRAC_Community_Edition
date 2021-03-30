#include "phosphor-ldap-config/utils.hpp"
#include <netinet/in.h>
#include <gtest/gtest.h>
#include <ldap.h>

namespace phosphor
{
namespace ldap
{
constexpr auto LDAPscheme = "ldap";
constexpr auto LDAPSscheme = "ldaps";

class TestUtil : public testing::Test
{
  public:
    TestUtil()
    {
        // Empty
    }
};

TEST_F(TestUtil, URIValidation)
{
    std::string ipaddress = "ldap://0.0.0.0";
    EXPECT_EQ(true, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldap://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldaps://9.3.185.83";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldap://9.3.a.83";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldap://9.3.185.a";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldap://x.x.x.x";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPscheme));

    ipaddress = "ldaps://0.0.0.0";
    EXPECT_EQ(true, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldap://0.0.0.0";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldaps://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldap://9.3.185.83";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldaps://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldaps://9.3.185.a";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldaps://9.3.a.83";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));

    ipaddress = "ldaps://x.x.x.x";
    EXPECT_EQ(false, isValidLDAPURI(ipaddress.c_str(), LDAPSscheme));
}
} // namespace ldap
} // namespace phosphor
