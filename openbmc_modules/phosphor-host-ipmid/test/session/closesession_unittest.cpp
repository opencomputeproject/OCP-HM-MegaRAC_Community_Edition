#include <ipmid/sessionhelper.hpp>

#include <gtest/gtest.h>

TEST(parseSessionInputPayloadTest, ValidObjectPath)
{
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0;
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";

    EXPECT_TRUE(
        parseCloseSessionInputPayload(objectPath, sessionId, sessionHandle));
    EXPECT_EQ(0x12a4567d, sessionId);
    EXPECT_EQ(0x8a, sessionHandle);
}

TEST(parseSessionInputPayloadTest, InvalidObjectPath)
{
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0;
    // A valid object path will be like
    // "/xyz/openbmc_project/ipmi/session/channel/sessionId_sessionHandle"
    // Ex: "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a"
    // SessionId    : 0X12a4567d
    // SessionHandle: 0X8a
    std::string objectPath = "/xyz/openbmc_project/ipmi/session/eth0/12a4567d";

    EXPECT_FALSE(
        parseCloseSessionInputPayload(objectPath, sessionId, sessionHandle));
}

TEST(parseSessionInputPayloadTest, NoObjectPath)
{
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0;
    std::string objectPath;

    EXPECT_FALSE(
        parseCloseSessionInputPayload(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, ValidSessionId)
{
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";
    uint32_t sessionId = 0x12a4567d;
    uint8_t sessionHandle = 0;

    EXPECT_TRUE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, ValidSessionHandle)
{
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0x8a;

    EXPECT_TRUE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, InvalidSessionId)
{
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";
    uint32_t sessionId = 0x1234b67d;
    uint8_t sessionHandle = 0;

    EXPECT_FALSE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, InvalidSessionHandle)
{
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0x9b;

    EXPECT_FALSE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, ZeroSessionId_ZeroSessionHandle)
{
    std::string objectPath =
        "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a";
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0;

    EXPECT_FALSE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, InvalidObjectPath)
{
    // A valid object path will be like
    // "/xyz/openbmc_project/ipmi/session/channel/sessionId_sessionHandle"
    // Ex: "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a"
    // SessionId    : 0X12a4567d
    // SessionHandle: 0X8a
    std::string objectPath = "/xyz/openbmc_project/ipmi/session/eth0/12a4567d";
    uint32_t sessionId = 0x12a4567d;
    uint8_t sessionHandle = 0;

    EXPECT_FALSE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}

TEST(isSessionObjectMatchedTest, NoObjectPath)
{
    std::string objectPath;
    uint32_t sessionId = 0x12a4567d;
    uint8_t sessionHandle = 0x8a;

    EXPECT_FALSE(isSessionObjectMatched(objectPath, sessionId, sessionHandle));
}
