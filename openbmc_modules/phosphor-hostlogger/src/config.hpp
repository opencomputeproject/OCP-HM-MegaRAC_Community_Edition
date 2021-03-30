// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <cstddef>

/**
 * @struct Config
 * @brief Configuration of the service, initialized with default values.
 */
struct Config
{
    /**
     * @brief Constructor: load configuration from environment variables.
     *
     * @throw std::invalid_argument invalid format in one of the variables
     */
    Config();

    /** @brief Socket ID used for connection with host console. */
    const char* socketId = "";
    /** @brief Max number of messages stored inside intermediate buffer. */
    size_t bufMaxSize = 3000;
    /** @brief Max age of messages (in minutes) inside intermediate buffer. */
    size_t bufMaxTime = 0;
    /** @brief Flag indicated we need to flush console buffer as it fills. */
    bool bufFlushFull = false;
    /** @brief Path to D-Bus object that provides host's state information. */
    const char* hostState = "/xyz/openbmc_project/state/host0";
    /** @brief Absolute path to the output directory for log files. */
    const char* outDir = "/var/lib/obmc/hostlogs";
    /** @brief Max number of log files in the output directory. */
    size_t maxFiles = 10;
};
