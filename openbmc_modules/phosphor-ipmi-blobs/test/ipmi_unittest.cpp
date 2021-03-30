#include "ipmi.hpp"

#include <cstring>

#include <gtest/gtest.h>

namespace blobs
{

// ipmid.hpp isn't installed where we can grab it and this value is per BMC
// SoC.
#define MAX_IPMI_BUFFER 64

TEST(StringInputTest, NullPointerInput)
{
    // The method should verify it did receive a non-null input pointer.

    EXPECT_STREQ("", stringFromBuffer(NULL, 5).c_str());
}

TEST(StringInputTest, ZeroBytesInput)
{
    // Verify that if the input length is 0 that it'll return the empty string.

    const char* request = "asdf";
    EXPECT_STREQ("", stringFromBuffer(request, 0).c_str());
}

TEST(StringInputTest, NulTerminatorNotFound)
{
    // Verify that if there isn't a nul-terminator found in an otherwise valid
    // string, it'll return the emptry string.

    char request[MAX_IPMI_BUFFER];
    std::memset(request, 'a', sizeof(request));
    EXPECT_STREQ("", stringFromBuffer(request, sizeof(request)).c_str());
}

TEST(StringInputTest, TwoNulsFound)
{
    // Verify it makes you use the entire data region for the string.
    char request[MAX_IPMI_BUFFER];
    request[0] = 'a';
    request[1] = 0;
    std::memset(&request[2], 'b', sizeof(request) - 2);
    request[MAX_IPMI_BUFFER - 1] = 0;

    // This case has two strings, and the last character is a nul-terminator.
    EXPECT_STREQ("", stringFromBuffer(request, sizeof(request)).c_str());
}

TEST(StringInputTest, NulTerminatorFound)
{
    // Verify that if it's provided a valid nul-terminated string, it'll
    // return it.

    const char* request = "asdf";
    EXPECT_STREQ("asdf", stringFromBuffer(request, 5).c_str());
}
} // namespace blobs
