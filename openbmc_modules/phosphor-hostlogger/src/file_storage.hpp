// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include "log_buffer.hpp"

#include <filesystem>

/**
 * @class FileStorage
 * @brief Persistent file storage with automatic log file rotation.
 */
class FileStorage
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] path absolute path to the output directory
     * @param[in] prefix prefix used for log file names
     * @param[in] maxFiles max number of log files that can be stored
     *
     * @throw std::exception in case of errors
     */
    FileStorage(const std::string& path, const std::string& prefix,
                size_t maxFiles);

    /**
     * @brief Save log buffer to a file.
     *
     * @param[in] buf buffer with log message to save
     *
     * @throw std::exception in case of errors
     *
     * @return path to saved file
     */
    std::string save(const LogBuffer& buf) const;

  private:
    /**
     * @brief Prepare output directory for a new log file and construct path.
     *
     * @throw std::exception in case of errors
     *
     * @return full path to the new file
     */
    std::string newFile() const;

    /**
     * @brief Rotate log files in the output directory by removing the oldest
     *        logs.
     *
     * @throw std::exception in case of errors
     */
    void rotate() const;

  private:
    /** @brief Output directory. */
    std::filesystem::path outDir;
    /** @brief Prefix used for log file names. */
    std::string filePrefix;
    /** @brief Max number of log files that can be stored. */
    size_t filesLimit;
};
