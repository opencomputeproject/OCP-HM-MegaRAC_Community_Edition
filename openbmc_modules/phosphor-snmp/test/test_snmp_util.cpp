#include <gtest/gtest.h>
#include <netinet/in.h>
#include "snmp_util.hpp"
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{
namespace snmp
{

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

TEST(TestUtil, IpValidation)
{
    // valid IPv4 address
    std::string ipaddress = "0.0.0.0";
    EXPECT_EQ(ipaddress, resolveAddress(ipaddress));

    ipaddress = "9.3.185.83";
    EXPECT_EQ(ipaddress, resolveAddress(ipaddress));

    // Invalid IPv4 address
    ipaddress = "9.3.185.a";
    EXPECT_THROW(resolveAddress(ipaddress), InternalFailure);

    ipaddress = "9.3.a.83";
    EXPECT_THROW(resolveAddress(ipaddress), InternalFailure);

    ipaddress = "x.x.x.x";
    EXPECT_THROW(resolveAddress(ipaddress), InternalFailure);

    // valid IPv6 address
    ipaddress = "0:0:0:0:0:0:0:0";
    EXPECT_EQ("::", resolveAddress(ipaddress));

    ipaddress = "1:0:0:0:0:0:0:8";
    EXPECT_EQ("1::8", resolveAddress(ipaddress));

    ipaddress = "1::8";
    EXPECT_EQ(ipaddress, resolveAddress(ipaddress));

    ipaddress = "0:0:0:0:0:FFFF:204.152.189.116";
    EXPECT_EQ("::ffff:204.152.189.116", resolveAddress(ipaddress));

    ipaddress = "::ffff:204.152.189.116";
    EXPECT_EQ(ipaddress, resolveAddress(ipaddress));

    ipaddress = "a:0:0:0:0:FFFF:204.152.189.116";
    EXPECT_EQ("a::ffff:cc98:bd74", resolveAddress(ipaddress));

    // Invalid IPv6 address
    ipaddress = "abcd::xyz::";
    EXPECT_THROW(resolveAddress(ipaddress), InternalFailure);

    // resolve the local host
    ipaddress = "localhost";
    auto isLocal = false;
    auto addr = resolveAddress(ipaddress);
    if (addr == "127.0.0.1" || addr == "::1")
    {
        isLocal = true;
    }
    EXPECT_TRUE(isLocal);
}

} // namespace snmp
} // namespace network
} // namespace phosphor
