// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include "config.hpp"
#include "dbus_loop.hpp"
#include "file_storage.hpp"
#include "host_console.hpp"
#include "log_buffer.hpp"

/**
 * @class Service
 * @brief Log service: watches for events and handles them.
 */
class Service
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] config service configuration
     *
     * @throw std::exception in case of errors
     */
    Service(const Config& config);

    /**
     * @brief Run the service.
     *
     * @throw std::exception in case of errors
     */
    void run();

  private:
    /**
     * @brief Flush log buffer to a file.
     */
    void flush();

    /**
     * @brief Read data from host console and put it into the log buffer.
     */
    void readConsole();

  private:
    /** @brief Service configuration. */
    const Config& config;
    /** @brief D-Bus event loop. */
    DbusLoop dbusLoop;
    /** @brief Host console connection. */
    HostConsole hostConsole;
    /** @brief Intermediate storage: container for parsed log messages. */
    LogBuffer logBuffer;
    /** @brief Persistent storage. */
    FileStorage fileStorage;
};
