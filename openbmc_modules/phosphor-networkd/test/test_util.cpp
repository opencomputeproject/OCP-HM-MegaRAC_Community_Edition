#include "util.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>

#include <cstddef>
#include <cstring>
#include <stdplus/raw.hpp>
#include <string>
#include <string_view>
#include <xyz/openbmc_project/Common/error.hpp>

#include <gtest/gtest.h>

namespace phosphor
{
namespace network
{

using namespace std::literals;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
class TestUtil : public testing::Test
{
  public:
    TestUtil()
    {
        // Empty
    }
};

TEST_F(TestUtil, AddrFromBuf)
{
    std::string tooSmall(1, 'a');
    std::string tooLarge(24, 'a');

    struct in_addr ip1;
    EXPECT_EQ(1, inet_pton(AF_INET, "192.168.10.1", &ip1));
    std::string_view buf1(reinterpret_cast<char*>(&ip1), sizeof(ip1));
    InAddrAny res1 = addrFromBuf(AF_INET, buf1);
    EXPECT_EQ(0, memcmp(&ip1, &std::get<struct in_addr>(res1), sizeof(ip1)));
    EXPECT_THROW(addrFromBuf(AF_INET, tooSmall), std::runtime_error);
    EXPECT_THROW(addrFromBuf(AF_INET, tooLarge), std::runtime_error);
    EXPECT_THROW(addrFromBuf(AF_UNSPEC, buf1), std::runtime_error);

    struct in6_addr ip2;
    EXPECT_EQ(1, inet_pton(AF_INET6, "fdd8:b5ad:9d93:94ee::2:1", &ip2));
    std::string_view buf2(reinterpret_cast<char*>(&ip2), sizeof(ip2));
    InAddrAny res2 = addrFromBuf(AF_INET6, buf2);
    EXPECT_EQ(0, memcmp(&ip2, &std::get<struct in6_addr>(res2), sizeof(ip2)));
    EXPECT_THROW(addrFromBuf(AF_INET6, tooSmall), std::runtime_error);
    EXPECT_THROW(addrFromBuf(AF_INET6, tooLarge), std::runtime_error);
    EXPECT_THROW(addrFromBuf(AF_UNSPEC, buf2), std::runtime_error);
}

TEST_F(TestUtil, IpToString)
{
    struct in_addr ip1;
    EXPECT_EQ(1, inet_pton(AF_INET, "192.168.10.1", &ip1));
    EXPECT_EQ("192.168.10.1", toString(InAddrAny(ip1)));

    struct in6_addr ip2;
    EXPECT_EQ(1, inet_pton(AF_INET6, "fdd8:b5ad:9d93:94ee::2:1", &ip2));
    EXPECT_EQ("fdd8:b5ad:9d93:94ee::2:1", toString(InAddrAny(ip2)));
}

TEST_F(TestUtil, IpValidation)
{
    std::string ipaddress = "0.0.0.0";
    EXPECT_EQ(true, isValidIP(AF_INET, ipaddress));

    ipaddress = "9.3.185.83";
    EXPECT_EQ(true, isValidIP(AF_INET, ipaddress));

    ipaddress = "9.3.185.a";
    EXPECT_EQ(false, isValidIP(AF_INET, ipaddress));

    ipaddress = "9.3.a.83";
    EXPECT_EQ(false, isValidIP(AF_INET, ipaddress));

    ipaddress = "x.x.x.x";
    EXPECT_EQ(false, isValidIP(AF_INET, ipaddress));

    ipaddress = "0:0:0:0:0:0:0:0";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "1:0:0:0:0:0:0:8";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "1::8";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "0:0:0:0:0:FFFF:204.152.189.116";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "::ffff:204.152.189.116";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "a:0:0:0:0:FFFF:204.152.189.116";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));

    ipaddress = "1::8";
    EXPECT_EQ(true, isValidIP(AF_INET6, ipaddress));
}

TEST_F(TestUtil, PrefixValidation)
{
    uint8_t prefixLength = 1;
    EXPECT_EQ(true, isValidPrefix(AF_INET, prefixLength));

    prefixLength = 32;
    EXPECT_EQ(true, isValidPrefix(AF_INET, prefixLength));

    prefixLength = 0;
    EXPECT_EQ(false, isValidPrefix(AF_INET, prefixLength));

    prefixLength = 33;
    EXPECT_EQ(false, isValidPrefix(AF_INET, prefixLength));

    prefixLength = 33;
    EXPECT_EQ(true, isValidPrefix(AF_INET6, prefixLength));

    prefixLength = 65;
    EXPECT_EQ(false, isValidPrefix(AF_INET, prefixLength));
}

TEST_F(TestUtil, ConvertV4MasktoPrefix)
{
    std::string mask = "255.255.255.0";
    uint8_t prefix = toCidr(AF_INET, mask);
    EXPECT_EQ(prefix, 24);

    mask = "255.255.0.0";
    prefix = toCidr(AF_INET, mask);
    EXPECT_EQ(prefix, 16);

    mask = "255.0.0.0";
    prefix = toCidr(AF_INET, mask);
    EXPECT_EQ(prefix, 8);

    mask = "255.224.0.0";
    prefix = toCidr(AF_INET, mask);
    EXPECT_EQ(prefix, 11);

    // Invalid Mask
    mask = "255.0.255.0";
    prefix = toCidr(AF_INET, mask);
    EXPECT_EQ(prefix, 0);
}

TEST_F(TestUtil, convertV6MasktoPrefix)
{
    std::string mask = "ffff:ffff::";
    uint8_t prefix = toCidr(AF_INET6, mask);
    EXPECT_EQ(prefix, 32);

    mask = "ffff:ffff:ffff::";
    prefix = toCidr(AF_INET6, mask);
    EXPECT_EQ(prefix, 48);

    mask = "ffff:ffff:fc00::";
    prefix = toCidr(AF_INET6, mask);
    EXPECT_EQ(prefix, 38);

    // Invalid Mask
    mask = "ffff:0fff::";
    prefix = toCidr(AF_INET6, mask);
    EXPECT_EQ(prefix, 0);
}

TEST_F(TestUtil, isLinkLocaladdress)
{
    std::string ipaddress = "fe80:fec0::";
    EXPECT_TRUE(isLinkLocalIP(ipaddress));

    ipaddress = "2000:fe80:789::";
    EXPECT_FALSE(isLinkLocalIP(ipaddress));

    ipaddress = "2000:fe80::";
    EXPECT_FALSE(isLinkLocalIP(ipaddress));

    ipaddress = "169.254.3.3";
    EXPECT_TRUE(isLinkLocalIP(ipaddress));

    ipaddress = "3.169.254.3";
    EXPECT_FALSE(isLinkLocalIP(ipaddress));

    ipaddress = "3.3.169.254";
    EXPECT_FALSE(isLinkLocalIP(ipaddress));
}

TEST_F(TestUtil, convertPrefixToMask)
{
    std::string mask = toMask(AF_INET, 24);
    EXPECT_EQ(mask, "255.255.255.0");

    mask = toMask(AF_INET, 8);
    EXPECT_EQ(mask, "255.0.0.0");

    mask = toMask(AF_INET, 27);
    EXPECT_EQ(mask, "255.255.255.224");
}

TEST_F(TestUtil, InterfaceToUbootEthAddr)
{
    EXPECT_EQ(std::nullopt, interfaceToUbootEthAddr("et"));
    EXPECT_EQ(std::nullopt, interfaceToUbootEthAddr("eth"));
    EXPECT_EQ(std::nullopt, interfaceToUbootEthAddr("sit0"));
    EXPECT_EQ(std::nullopt, interfaceToUbootEthAddr("ethh0"));
    EXPECT_EQ(std::nullopt, interfaceToUbootEthAddr("eth0h"));
    EXPECT_EQ("ethaddr", interfaceToUbootEthAddr("eth0"));
    EXPECT_EQ("eth1addr", interfaceToUbootEthAddr("eth1"));
    EXPECT_EQ("eth5addr", interfaceToUbootEthAddr("eth5"));
    EXPECT_EQ("eth28addr", interfaceToUbootEthAddr("eth28"));
}

namespace mac_address
{

TEST(MacFromString, Bad)
{
    EXPECT_THROW(fromString("0x:00:00:00:00:00"), std::runtime_error);
    EXPECT_THROW(fromString("00:00:00:00:00"), std::runtime_error);
    EXPECT_THROW(fromString(""), std::runtime_error);
}

TEST(MacFromString, Valid)
{
    EXPECT_TRUE(
        stdplus::raw::equal(ether_addr{}, fromString("00:00:00:00:00:00")));
    EXPECT_TRUE(
        stdplus::raw::equal(ether_addr{0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa},
                            fromString("FF:EE:DD:cc:bb:aa")));
    EXPECT_TRUE(
        stdplus::raw::equal(ether_addr{0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
                            fromString("0:1:2:3:4:5")));
}

TEST(MacToString, Valid)
{
    EXPECT_EQ("11:22:33:44:55:66",
              toString({0x11, 0x22, 0x33, 0x44, 0x55, 0x66}));
    EXPECT_EQ("01:02:03:04:05:67",
              toString({0x01, 0x02, 0x03, 0x04, 0x05, 0x67}));
    EXPECT_EQ("00:00:00:00:00:00",
              toString({0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
}

TEST(MacIsEmpty, True)
{
    EXPECT_TRUE(isEmpty({}));
}

TEST(MacIsEmpty, False)
{
    EXPECT_FALSE(isEmpty(fromString("01:00:00:00:00:00")));
    EXPECT_FALSE(isEmpty(fromString("00:00:00:10:00:00")));
    EXPECT_FALSE(isEmpty(fromString("00:00:00:00:00:01")));
}

TEST(MacIsMulticast, True)
{
    EXPECT_TRUE(isMulticast(fromString("ff:ff:ff:ff:ff:ff")));
    EXPECT_TRUE(isMulticast(fromString("01:00:00:00:00:00")));
}

TEST(MacIsMulticast, False)
{
    EXPECT_FALSE(isMulticast(fromString("00:11:22:33:44:55")));
    EXPECT_FALSE(isMulticast(fromString("FE:11:22:33:44:55")));
}

TEST(MacIsUnicast, True)
{
    EXPECT_TRUE(isUnicast(fromString("00:11:22:33:44:55")));
    EXPECT_TRUE(isUnicast(fromString("FE:11:22:33:44:55")));
}

TEST(MacIsUnicast, False)
{
    EXPECT_FALSE(isUnicast(fromString("00:00:00:00:00:00")));
    EXPECT_FALSE(isUnicast(fromString("01:00:00:00:00:00")));
    EXPECT_FALSE(isUnicast(fromString("ff:ff:ff:ff:ff:ff")));
}

} // namespace mac_address
} // namespace network
} // namespace phosphor
