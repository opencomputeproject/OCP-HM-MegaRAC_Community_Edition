// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <string>

/**
 * @class HostConsole
 * @brief Connection with host's console.
 */
class HostConsole
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] socketId socket ID used for construction path to the socket
     */
    HostConsole(const std::string& socketId);

    ~HostConsole();

    /**
     * @brief Connect to the host's console via socket.
     *
     * @throw std::invalid_argument if socket ID is invalid
     * @throw std::system_error in case of other errors
     */
    void connect();

    /**
     * @brief Non-blocking read data from console's socket.
     *
     * @param[out] buf buffer to write the incoming data
     * @param[in] sz size of the buffer
     *
     * @throw std::system_error in case of errors
     *
     * @return number of actually read bytes
     */
    size_t read(char* buf, size_t sz) const;

    /** @brief Get socket file descriptor, used for watching IO. */
    operator int() const;

  private:
    /** @brief Socket Id. */
    std::string socketId;
    /** @brief File descriptor of the socket. */
    int socketFd;
};
