// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "zlib_exception.hpp"

#include <cstring>

ZlibException::ZlibException(Operation op, int code, gzFile fd,
                             const std::string& fileName)
{
    std::string details;
    if (code == Z_ERRNO)
    {
        // System error
        const int errCode = errno ? errno : EIO;
        details = strerror(errCode);
    }
    else if (fd != Z_NULL)
    {
        // Try to get description from zLib
        int lastErrCode = 0;
        const char* lastErrDesc = gzerror(fd, &lastErrCode);
        if (lastErrCode)
        {
            details = '[';
            details += std::to_string(lastErrCode);
            details += "] ";
            details += lastErrDesc;
        }
    }
    if (details.empty())
    {
        details = "Internal zlib error (code ";
        details += std::to_string(code);
        details += ')';
    }

    errDesc = "Unable to ";
    switch (op)
    {
        case create:
            errDesc += "create";
            break;
        case close:
            errDesc += "close";
            break;
        case write:
            errDesc += "write";
            break;
    }
    errDesc += " file ";
    errDesc += fileName;
    errDesc += ": ";
    errDesc += details;
}

const char* ZlibException::what() const noexcept
{
    return errDesc.c_str();
}
