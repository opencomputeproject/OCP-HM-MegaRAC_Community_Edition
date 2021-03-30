/**
 * Copyright © 2017 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pmbus.hpp"

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/Device/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <filesystem>
#include <fstream>

namespace witherspoon
{
namespace pmbus
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace sdbusplus::xyz::openbmc_project::Common::Device::Error;
namespace fs = std::filesystem;

/**
 * @brief Helper to close a file handle
 */
struct FileCloser
{
    void operator()(FILE* fp) const
    {
        fclose(fp);
    }
};

std::string PMBus::insertPageNum(const std::string& templateName, size_t page)
{
    auto name = templateName;

    // insert the page where the P was
    auto pos = name.find('P');
    if (pos != std::string::npos)
    {
        name.replace(pos, 1, std::to_string(page));
    }

    return name;
}

fs::path PMBus::getPath(Type type)
{
    switch (type)
    {
        default:
        /* fall through */
        case Type::Base:
            return basePath;
            break;
        case Type::Hwmon:
            return basePath / "hwmon" / hwmonDir;
            break;
        case Type::Debug:
            return debugPath / "pmbus" / hwmonDir;
            break;
        case Type::DeviceDebug:
        {
            auto dir = driverName + "." + std::to_string(instance);
            return debugPath / dir;
            break;
        }
        case Type::HwmonDeviceDebug:
            return debugPath / "pmbus" / hwmonDir / getDeviceName();
            break;
    }
}

std::string PMBus::getDeviceName()
{
    std::string name;
    std::ifstream file;
    auto path = basePath / "name";

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);
    try
    {
        file.open(path);
        file >> name;
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Unable to read PMBus device name",
                        entry("PATH=%s", path.c_str()));
    }

    return name;
}

bool PMBus::readBitInPage(const std::string& name, size_t page, Type type)
{
    auto pagedBit = insertPageNum(name, page);
    return readBit(pagedBit, type);
}

bool PMBus::readBit(const std::string& name, Type type)
{
    unsigned long int value = 0;
    std::ifstream file;
    fs::path path = getPath(type);

    path /= name;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);

    try
    {
        char* err = NULL;
        std::string val{1, '\0'};

        file.open(path);
        file.read(&val[0], 1);

        value = strtoul(val.c_str(), &err, 10);

        if (*err)
        {
            log<level::ERR>("Invalid character in sysfs file",
                            entry("FILE=%s", path.c_str()),
                            entry("CONTENTS=%s", val.c_str()));

            // Catch below and handle as a read failure
            elog<InternalFailure>();
        }
    }
    catch (std::exception& e)
    {
        auto rc = errno;

        log<level::ERR>("Failed to read sysfs file",
                        entry("FILENAME=%s", path.c_str()));

        using metadata = xyz::openbmc_project::Common::Device::ReadFailure;

        elog<ReadFailure>(
            metadata::CALLOUT_ERRNO(rc),
            metadata::CALLOUT_DEVICE_PATH(fs::canonical(basePath).c_str()));
    }

    return value != 0;
}

bool PMBus::exists(const std::string& name, Type type)
{
    auto path = getPath(type);
    path /= name;
    return fs::exists(path);
}

uint64_t PMBus::read(const std::string& name, Type type)
{
    uint64_t data = 0;
    std::ifstream file;
    auto path = getPath(type);
    path /= name;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);

    try
    {
        file.open(path);
        file >> std::hex >> data;
    }
    catch (std::exception& e)
    {
        auto rc = errno;
        log<level::ERR>("Failed to read sysfs file",
                        entry("FILENAME=%s", path.c_str()));

        using metadata = xyz::openbmc_project::Common::Device::ReadFailure;

        elog<ReadFailure>(
            metadata::CALLOUT_ERRNO(rc),
            metadata::CALLOUT_DEVICE_PATH(fs::canonical(basePath).c_str()));
    }

    return data;
}

std::string PMBus::readString(const std::string& name, Type type)
{
    std::string data;
    std::ifstream file;
    auto path = getPath(type);
    path /= name;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                    std::ifstream::eofbit);

    try
    {
        file.open(path);
        file >> data;
    }
    catch (std::exception& e)
    {
        auto rc = errno;
        log<level::ERR>("Failed to read sysfs file",
                        entry("FILENAME=%s", path.c_str()));

        using metadata = xyz::openbmc_project::Common::Device::ReadFailure;

        elog<ReadFailure>(
            metadata::CALLOUT_ERRNO(rc),
            metadata::CALLOUT_DEVICE_PATH(fs::canonical(basePath).c_str()));
    }

    return data;
}

std::vector<uint8_t> PMBus::readBinary(const std::string& name, Type type,
                                       size_t length)
{
    auto path = getPath(type) / name;

    // Use C style IO because it's easier to handle telling the difference
    // between hitting EOF or getting an actual error.
    std::unique_ptr<FILE, FileCloser> file{fopen(path.c_str(), "rb")};

    if (file)
    {
        std::vector<uint8_t> data(length, 0);

        auto bytes =
            fread(data.data(), sizeof(decltype(data[0])), length, file.get());

        if (bytes != length)
        {
            // If hit EOF, just return the amount of data that was read.
            if (feof(file.get()))
            {
                data.erase(data.begin() + bytes, data.end());
            }
            else if (ferror(file.get()))
            {
                auto rc = errno;
                log<level::ERR>("Failed to read sysfs file",
                                entry("FILENAME=%s", path.c_str()));

                using metadata =
                    xyz::openbmc_project::Common::Device::ReadFailure;

                elog<ReadFailure>(metadata::CALLOUT_ERRNO(rc),
                                  metadata::CALLOUT_DEVICE_PATH(
                                      fs::canonical(basePath).c_str()));
            }
        }
        return data;
    }

    return std::vector<uint8_t>{};
}

void PMBus::write(const std::string& name, int value, Type type)
{
    std::ofstream file;
    fs::path path = getPath(type);

    path /= name;

    file.exceptions(std::ofstream::failbit | std::ofstream::badbit |
                    std::ofstream::eofbit);

    try
    {
        file.open(path);
        file << value;
    }
    catch (const std::exception& e)
    {
        auto rc = errno;

        log<level::ERR>("Failed to write sysfs file",
                        entry("FILENAME=%s", path.c_str()));

        using metadata = xyz::openbmc_project::Common::Device::WriteFailure;

        elog<WriteFailure>(
            metadata::CALLOUT_ERRNO(rc),
            metadata::CALLOUT_DEVICE_PATH(fs::canonical(basePath).c_str()));
    }
}

void PMBus::findHwmonDir()
{
    fs::path path{basePath};
    path /= "hwmon";

    // Make sure the directory exists, otherwise for things that can be
    // dynamically present or not present an exception will be thrown if the
    // hwmon directory is not there, resulting in a program termination.
    if (fs::is_directory(path))
    {
        // look for <basePath>/hwmon/hwmonN/
        for (auto& f : fs::directory_iterator(path))
        {
            if ((f.path().filename().string().find("hwmon") !=
                 std::string::npos) &&
                (fs::is_directory(f.path())))
            {
                hwmonDir = f.path().filename();
                break;
            }
        }
    }

    // Don't really want to crash here, just log it
    // and let accesses fail later
    if (hwmonDir.empty())
    {
        log<level::INFO>("Unable to find hwmon directory "
                         "in device base path",
                         entry("DEVICE_PATH=%s", basePath.c_str()));
    }
}

} // namespace pmbus
} // namespace witherspoon
