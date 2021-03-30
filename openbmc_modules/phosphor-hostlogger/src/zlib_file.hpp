// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <zlib.h>

#include <ctime>
#include <string>

/**
 * @class ZlibFile
 * @brief Log file writer.
 */
class ZlibFile
{
  public:
    /**
     * @brief Constructor create new file for writing logs.
     *
     * @param[in] fileName path to the file
     *
     * @throw ZlibException in case of errors
     */
    ZlibFile(const std::string& fileName);

    ~ZlibFile();

    ZlibFile(const ZlibFile&) = delete;
    ZlibFile& operator=(const ZlibFile&) = delete;

    /**
     * @brief Close file.
     *
     * @throw ZlibException in case of errors
     */
    void close();

    /**
     * @brief Write single log message to the file.
     *
     * @param[in] timeStamp time stamp of the log message
     * @param[in] message log message text
     *
     * @throw ZlibException in case of errors
     */
    void write(const tm& timeStamp, const std::string& message) const;

  private:
    /** @brief File name. */
    std::string fileName;
    /** @brief zLib file descriptor. */
    gzFile fd;
};
