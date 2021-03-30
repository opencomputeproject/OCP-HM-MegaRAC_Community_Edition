// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "file_storage.hpp"

#include "zlib_file.hpp"

#include <set>

namespace fs = std::filesystem;

/** @brief File extension for log files. */
static const std::string fileExt = ".log.gz";

FileStorage::FileStorage(const std::string& path, const std::string& prefix,
                         size_t maxFiles) :
    outDir(path),
    filePrefix(prefix), filesLimit(maxFiles)
{
    // Check path
    if (!outDir.is_absolute())
    {
        throw std::invalid_argument("Path must be absolute");
    }
    fs::create_directories(outDir);

    // Normalize file name prefix
    if (filePrefix.empty())
    {
        filePrefix = "host";
    }
}

std::string FileStorage::save(const LogBuffer& buf) const
{
    if (buf.empty())
    {
        return std::string(); // Buffer is empty, nothing to save
    }

    const std::string fileName = newFile();
    ZlibFile logFile(fileName);

    // Write full datetime stamp as the first record
    tm tmLocal;
    localtime_r(&buf.begin()->timeStamp, &tmLocal);
    char tmText[20]; // asciiz for YYYY-MM-DD HH:MM:SS
    strftime(tmText, sizeof(tmText), "%F %T", &tmLocal);
    std::string titleMsg = ">>> Log collection started at ";
    titleMsg += tmText;
    logFile.write(tmLocal, titleMsg);

    // Write messages
    for (const auto& msg : buf)
    {
        localtime_r(&msg.timeStamp, &tmLocal);
        logFile.write(tmLocal, msg.text);
    }

    logFile.close();

    rotate();

    return fileName;
}

std::string FileStorage::newFile() const
{
    // Prepare directory
    fs::create_directories(outDir);

    // Construct log file name: {prefix}_{timestamp}[_N].{ext}
    std::string fileName = outDir / (filePrefix + '_');

    time_t tmCurrent;
    time(&tmCurrent);
    tm tmLocal;
    localtime_r(&tmCurrent, &tmLocal);
    char tmText[16]; // asciiz for YYYYMMDD_HHMMSS
    strftime(tmText, sizeof(tmText), "%Y%m%d_%H%M%S", &tmLocal);
    fileName += tmText;

    // Handle duplicate files
    std::string dupPostfix;
    size_t dupCounter = 0;
    while (fs::exists(fileName + dupPostfix + fileExt))
    {
        dupPostfix = '_' + std::to_string(++dupCounter);
    }
    fileName += dupPostfix;
    fileName += fileExt;

    return fileName;
}

void FileStorage::rotate() const
{
    if (!filesLimit)
    {
        return; // Unlimited
    }

    // Get file list to ordered set
    std::set<std::string> logFiles;
    for (const auto& file : fs::directory_iterator(outDir))
    {
        if (!fs::is_regular_file(file))
        {
            continue;
        }
        const std::string fileName = file.path().filename();

        const size_t minFileNameLen = filePrefix.length() +
                                      15 + // time stamp YYYYMMDD_HHMMSS
                                      fileExt.length();
        if (fileName.length() < minFileNameLen)
        {
            continue;
        }

        if (fileName.compare(fileName.length() - fileExt.length(),
                             fileExt.length(), fileExt))
        {
            continue;
        }

        const std::string fullPrefix = filePrefix + '_';
        if (fileName.compare(0, fullPrefix.length(), fullPrefix))
        {
            continue;
        }

        logFiles.insert(fileName);
    }

    // Log file has a name with a timestamp generated. The sorted set contains
    // the oldest file on the top, remove them.
    if (logFiles.size() > filesLimit)
    {
        size_t removeCount = logFiles.size() - filesLimit;
        for (const auto& fileName : logFiles)
        {
            fs::remove(outDir / fileName);
            if (!--removeCount)
            {
                break;
            }
        }
    }
}
