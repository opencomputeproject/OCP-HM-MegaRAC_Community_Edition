#include <ipmid/api.h>

#include <cstring>
#include <ipmid/oemrouter.hpp>

#include "sample.h"

#include <gtest/gtest.h>

// Watch for correct singleton behavior.
static oem::Router* singletonUnderTest;

static ipmid_callback_t wildHandler;

static ipmi_netfn_t lastNetFunction;

// Fake ipmi_register_callback() for this test.
void ipmi_register_callback(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_context_t context, ipmid_callback_t cb,
                            ipmi_cmd_privilege_t priv)
{
    EXPECT_EQ(NETFUN_OEM_GROUP, netfn);
    EXPECT_EQ(IPMI_CMD_WILDCARD, cmd);
    EXPECT_EQ(reinterpret_cast<void*>(singletonUnderTest), context);
    EXPECT_EQ(PRIVILEGE_OEM, priv);
    lastNetFunction = netfn;
    wildHandler = cb;
}

namespace oem
{

namespace
{
void MakeRouter()
{
    if (!singletonUnderTest)
    {
        singletonUnderTest = mutableRouter();
    }
    ASSERT_EQ(singletonUnderTest, mutableRouter());
}

void ActivateRouter()
{
    MakeRouter();
    singletonUnderTest->activate();
    ASSERT_EQ(NETFUN_OEM_GROUP, lastNetFunction);
}

void RegisterWithRouter(Number oen, ipmi_cmd_t cmd, Handler cb)
{
    ActivateRouter();
    singletonUnderTest->registerHandler(oen, cmd, cb);
}

uint8_t msgPlain[] = {0x56, 0x34, 0x12};
uint8_t replyPlain[] = {0x56, 0x34, 0x12, 0x31, 0x41};
uint8_t msgPlus2[] = {0x67, 0x45, 0x23, 0x10, 0x20};
uint8_t msgBadOen[] = {0x57, 0x34, 0x12};

void RegisterTwoWays(ipmi_cmd_t* nextCmd)
{
    Handler f = [](ipmi_cmd_t cmd, const uint8_t* reqBuf, uint8_t* replyBuf,
                   size_t* dataLen) {
        // Check inputs
        EXPECT_EQ(0x78, cmd);
        EXPECT_EQ(0, *dataLen); // Excludes OEN

        // Generate reply.
        *dataLen = 2;
        std::memcpy(replyBuf, replyPlain + 3, *dataLen);
        return 0;
    };
    RegisterWithRouter(0x123456, 0x78, f);

    *nextCmd = IPMI_CMD_WILDCARD;
    Handler g = [nextCmd](ipmi_cmd_t cmd, const uint8_t* reqBuf,
                          uint8_t* replyBuf, size_t* dataLen) {
        // Check inputs
        EXPECT_EQ(*nextCmd, cmd);
        EXPECT_EQ(2, *dataLen); // Excludes OEN
        if (2 != *dataLen)
        {
            return 0xE0;
        }
        EXPECT_EQ(msgPlus2[3], reqBuf[0]);
        EXPECT_EQ(msgPlus2[4], reqBuf[1]);

        // Generate reply.
        *dataLen = 0;
        return 0;
    };
    RegisterWithRouter(0x234567, IPMI_CMD_WILDCARD, g);
}
} // namespace

TEST(OemRouterTest, MakeRouterProducesConsistentSingleton)
{
    MakeRouter();
}

TEST(OemRouterTest, ActivateRouterSetsLastNetToOEMGROUP)
{
    lastNetFunction = 0;
    ActivateRouter();
}

TEST(OemRouterTest, VerifiesSpecificCommandMatches)
{
    ipmi_cmd_t cmd;
    uint8_t reply[256];
    size_t dataLen;

    RegisterTwoWays(&cmd);

    dataLen = 3;
    EXPECT_EQ(0, wildHandler(NETFUN_OEM_GROUP, 0x78, msgPlain, reply, &dataLen,
                             nullptr));
    EXPECT_EQ(5, dataLen);
    EXPECT_EQ(replyPlain[0], reply[0]);
    EXPECT_EQ(replyPlain[1], reply[1]);
    EXPECT_EQ(replyPlain[2], reply[2]);
    EXPECT_EQ(replyPlain[3], reply[3]);
    EXPECT_EQ(replyPlain[4], reply[4]);
}

TEST(OemRouterTest, WildCardMatchesTwoRandomCodes)
{
    ipmi_cmd_t cmd;
    uint8_t reply[256];
    size_t dataLen;

    RegisterTwoWays(&cmd);

    // Check two random command codes.
    dataLen = 5;
    cmd = 0x89;
    EXPECT_EQ(0, wildHandler(NETFUN_OEM_GROUP, cmd, msgPlus2, reply, &dataLen,
                             nullptr));
    EXPECT_EQ(3, dataLen);

    dataLen = 5;
    cmd = 0x67;
    EXPECT_EQ(0, wildHandler(NETFUN_OEM_GROUP, cmd, msgPlus2, reply, &dataLen,
                             nullptr));
    EXPECT_EQ(3, dataLen);
}

TEST(OemRouterTest, CommandsAreRejectedIfInvalid)
{
    ipmi_cmd_t cmd;
    uint8_t reply[256];
    size_t dataLen;

    RegisterTwoWays(&cmd);

    // Message too short to include whole OEN?
    dataLen = 2;
    EXPECT_EQ(IPMI_CC_REQ_DATA_LEN_INVALID,
              wildHandler(NETFUN_OEM_GROUP, 0x78, msgPlain, reply, &dataLen,
                          nullptr));

    // Wrong specific command?
    dataLen = 3;
    EXPECT_EQ(IPMI_CC_INVALID, wildHandler(NETFUN_OEM_GROUP, 0x89, msgPlain,
                                           reply, &dataLen, nullptr));

    // Wrong OEN?
    dataLen = 3;
    EXPECT_EQ(IPMI_CC_INVALID, wildHandler(NETFUN_OEM_GROUP, 0x78, msgBadOen,
                                           reply, &dataLen, nullptr));
}

} // namespace oem
