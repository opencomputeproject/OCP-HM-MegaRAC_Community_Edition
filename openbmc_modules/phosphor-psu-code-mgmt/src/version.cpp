#include "version.hpp"

#include "item_updater.hpp"

#include <fstream>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace software
{
namespace updater
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

std::map<std::string, std::string>
    Version::getValues(const std::string& filePath,
                       const std::vector<std::string>& keys)
{
    if (filePath.empty())
    {
        log<level::ERR>("Error filePath is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("FilePath"),
                              Argument::ARGUMENT_VALUE(filePath.c_str()));
    }

    std::ifstream efile(filePath);
    std::string line;
    std::map<std::string, std::string> ret;

    while (getline(efile, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            // Remove \r from the end of line
            line.pop_back();
        }
        for (const auto& key : keys)
        {
            auto value = key + "=";
            auto keySize = value.length();
            if (line.compare(0, keySize, value) == 0)
            {
                ret.emplace(key, line.substr(keySize));
                break;
            }
        }
    }
    return ret;
}

std::string Version::getValue(const std::string& filePath,
                              const std::string& key)
{
    std::string ret;
    auto values = Version::getValues(filePath, {key});
    const auto it = values.find(key);
    if (it != values.end())
    {
        ret = it->second;
    }
    return ret;
}

std::map<std::string, std::string>
    Version::getExtVersionInfo(const std::string& extVersion)
{
    // The extVersion shall be key/value pairs separated by comma,
    // e.g. key1=value1,key2=value2
    std::map<std::string, std::string> result;
    std::stringstream ss(extVersion);

    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');
        auto pos = substr.find('=');
        if (pos != std::string::npos)
        {
            std::string key = substr.substr(0, pos);
            std::string value = substr.substr(pos + 1);
            result.emplace(key, value);
        }
    }
    return result;
}

void Delete::delete_()
{
    if (version.eraseCallback)
    {
        version.eraseCallback(version.getVersionId());
    }
}

} // namespace updater
} // namespace software
} // namespace phosphor
