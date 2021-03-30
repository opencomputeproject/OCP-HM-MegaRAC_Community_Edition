// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <zlib.h>

#include <exception>
#include <string>

/**
 * @class ZlibException
 * @brief zLib exception.
 */
class ZlibException : public std::exception
{
  public:
    /** @brief File operation types. */
    enum Operation
    {
        create,
        write,
        close
    };

    /**
     * @brief Constructor.
     *
     * @param[in] op type of operation
     * @param[in] code zLib status code
     * @param[in] fd zLib file descriptor
     * @param[in] fileName file name
     */
    ZlibException(Operation op, int code, gzFile fd,
                  const std::string& fileName);

    // From std::exception
    const char* what() const noexcept override;

  private:
    /** @brief Error description buffer. */
    std::string errDesc;
};
