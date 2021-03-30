#include "config.h"

#include "serialize.hpp"

#include <cereal/archives/json.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/server.hpp>

#include <filesystem>
#include <fstream>

namespace phosphor
{
namespace software
{
namespace updater
{

using namespace phosphor::logging;
namespace fs = std::filesystem;

const std::string priorityName = "priority";
const std::string purposeName = "purpose";

void storePriority(const std::string& versionId, uint8_t priority)
{
    auto path = fs::path(PERSIST_DIR) / versionId;
    if (!fs::is_directory(path))
    {
        if (fs::exists(path))
        {
            // Delete if it's a non-directory file
            log<level::WARNING>("Removing non-directory file",
                                entry("PATH=%s", path.c_str()));
            fs::remove_all(path);
        }
        fs::create_directories(path);
    }
    path = path / priorityName;

    std::ofstream os(path.c_str());
    cereal::JSONOutputArchive oarchive(os);
    oarchive(cereal::make_nvp(priorityName, priority));
}

void storePurpose(const std::string& versionId, VersionPurpose purpose)
{
    auto path = fs::path(PERSIST_DIR) / versionId;
    if (!fs::is_directory(path))
    {
        if (fs::exists(path))
        {
            // Delete if it's a non-directory file
            log<level::WARNING>("Removing non-directory file",
                                entry("PATH=%s", path.c_str()));
            fs::remove_all(path);
        }
        fs::create_directories(path);
    }
    path = path / purposeName;

    std::ofstream os(path.c_str());
    cereal::JSONOutputArchive oarchive(os);
    oarchive(cereal::make_nvp(purposeName, purpose));
}

bool restorePriority(const std::string& versionId, uint8_t& priority)
{
    auto path = fs::path(PERSIST_DIR) / versionId / priorityName;
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in);
        try
        {
            cereal::JSONInputArchive iarchive(is);
            iarchive(cereal::make_nvp(priorityName, priority));
            return true;
        }
        catch (cereal::Exception& e)
        {
            fs::remove_all(path);
        }
    }

    // Find the mtd device "u-boot-env" to retrieve the environment variables
    std::ifstream mtdDevices("/proc/mtd");
    std::string device, devicePath;

    try
    {
        while (std::getline(mtdDevices, device))
        {
            if (device.find("u-boot-env") != std::string::npos)
            {
                devicePath = "/dev/" + device.substr(0, device.find(':'));
                break;
            }
        }

        if (!devicePath.empty())
        {
            std::ifstream input(devicePath.c_str());
            std::string envVars;
            std::getline(input, envVars);

            std::string versionVar = versionId + "=";
            auto varPosition = envVars.find(versionVar);

            if (varPosition != std::string::npos)
            {
                // Grab the environment variable for this versionId. These
                // variables follow the format "versionId=priority\0"
                auto var = envVars.substr(varPosition);
                priority = std::stoi(var.substr(versionVar.length()));
                return true;
            }
        }
    }
    catch (const std::exception& e)
    {}

    return false;
}

bool restorePurpose(const std::string& versionId, VersionPurpose& purpose)
{
    auto path = fs::path(PERSIST_DIR) / versionId / purposeName;
    if (fs::exists(path))
    {
        std::ifstream is(path.c_str(), std::ios::in);
        try
        {
            cereal::JSONInputArchive iarchive(is);
            iarchive(cereal::make_nvp(purposeName, purpose));
            return true;
        }
        catch (cereal::Exception& e)
        {
            fs::remove_all(path);
        }
    }

    return false;
}

void removePersistDataDirectory(const std::string& versionId)
{
    auto path = fs::path(PERSIST_DIR) / versionId;
    if (fs::exists(path))
    {
        fs::remove_all(path);
    }
}

} // namespace updater
} // namespace software
} // namespace phosphor
