// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "host_console.hpp"

#include <sys/socket.h>
#include <sys/un.h>

#include <gtest/gtest.h>

static constexpr char socketPath[] = "\0obmc-console";

/**
 * @class HostConsoleTest
 * @brief Persistent file storage tests.
 */
class HostConsoleTest : public ::testing::Test
{
  protected:
    void startServer(const char* socketId)
    {
        // Start server
        serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_NE(serverSocket, -1);
        std::string path(socketPath, socketPath + sizeof(socketPath) - 1);
        if (*socketId)
        {
            path += '.';
            path += socketId;
        }
        sockaddr_un sa;
        sa.sun_family = AF_UNIX;
        memcpy(&sa.sun_path, path.c_str(), path.length());
        const socklen_t len = sizeof(sa) - sizeof(sa.sun_path) + path.length();
        ASSERT_NE(
            bind(serverSocket, reinterpret_cast<const sockaddr*>(&sa), len),
            -1);
        ASSERT_NE(listen(serverSocket, 1), -1);
    }

    void TearDown() override
    {
        // Stop server
        if (serverSocket != -1)
        {
            close(serverSocket);
        }
    }

    int serverSocket = -1;
};

TEST_F(HostConsoleTest, SingleHost)
{
    const char* socketId = "";
    startServer(socketId);

    HostConsole con(socketId);
    con.connect();

    const int clientSocket = accept(serverSocket, nullptr, nullptr);
    EXPECT_NE(clientSocket, -1);
    close(clientSocket);
}

TEST_F(HostConsoleTest, MultiHost)
{
    const char* socketId = "host123";
    startServer(socketId);

    HostConsole con(socketId);
    con.connect();

    const int clientSocket = accept(serverSocket, nullptr, nullptr);
    EXPECT_NE(clientSocket, -1);

    const char* data = "test data";
    const size_t len = strlen(data);
    EXPECT_EQ(send(clientSocket, data, len, 0), len);

    char buf[64];
    memset(buf, 0, sizeof(buf));
    EXPECT_EQ(con.read(buf, sizeof(buf)), len);
    EXPECT_STREQ(buf, data);
    EXPECT_EQ(con.read(buf, sizeof(buf)), 0);

    close(clientSocket);
}
