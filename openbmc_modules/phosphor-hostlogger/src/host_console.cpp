// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "host_console.hpp"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <system_error>

/**
 * @brief Base path to the console's socket.
 *        See obmc-console for details.
 */
static constexpr char socketPath[] = "\0obmc-console";

HostConsole::HostConsole(const std::string& socketId) :
    socketId(socketId), socketFd(-1)
{}

HostConsole::~HostConsole()
{
    if (socketFd != -1)
    {
        close(socketFd);
    }
}

void HostConsole::connect()
{
    if (socketFd != -1)
    {
        throw std::runtime_error("Socket already opened");
    }

    // Create socket
    socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFd == -1)
    {
        std::error_code ec(errno ? errno : EIO, std::generic_category());
        throw std::system_error(ec, "Unable to create socket");
    }

    // Set non-blocking mode for socket
    int opt = 1;
    if (ioctl(socketFd, FIONBIO, &opt))
    {
        std::error_code ec(errno ? errno : EIO, std::generic_category());
        throw std::system_error(ec, "Unable to set non-blocking mode");
    }

    // Construct path to the socket file (see obmc-console for details)
    std::string path(socketPath, socketPath + sizeof(socketPath) - 1);
    if (!socketId.empty())
    {
        path += '.';
        path += socketId;
    }
    if (path.length() > sizeof(sockaddr_un::sun_path))
    {
        throw std::invalid_argument("Invalid socket ID");
    }

    sockaddr_un sa;
    sa.sun_family = AF_UNIX;
    memcpy(&sa.sun_path, path.c_str(), path.length());

    // Connect to host's log stream via socket.
    // The owner of the socket (server) is obmc-console service and
    // we have a dependency on it written in the systemd unit file, but
    // we can't guarantee that the socket is initialized at the moment.
    size_t connectAttempts = 60; // Number of attempts
    const socklen_t len = sizeof(sa) - sizeof(sa.sun_path) + path.length();
    int rc = -1;
    while (connectAttempts--)
    {
        rc = ::connect(socketFd, reinterpret_cast<const sockaddr*>(&sa), len);
        if (!rc)
        {
            break;
        }
        else
        {
            sleep(1); // Make 1 second pause between attempts
        }
    }
    if (rc)
    {
        std::string err = "Unable to connect to console";
        if (!socketId.empty())
        {
            err += ' ';
            err += socketId;
        }
        std::error_code ec(errno ? errno : EIO, std::generic_category());
        throw std::system_error(ec, err);
    }
}

size_t HostConsole::read(char* buf, size_t sz) const
{
    ssize_t rsz = ::read(socketFd, buf, sz);
    if (rsz < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // We are in non-blocking mode, so ignore these codes
            rsz = 0;
        }
        else
        {
            std::string err = "Unable to read socket";
            if (!socketId.empty())
            {
                err += ' ';
                err += socketId;
            }
            std::error_code ec(errno ? errno : EIO, std::generic_category());
            throw std::system_error(ec, err);
        }
    }

    return static_cast<size_t>(rsz);
}

HostConsole::operator int() const
{
    return socketFd;
}
