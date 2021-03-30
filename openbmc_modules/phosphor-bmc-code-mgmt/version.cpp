#include "config.h"

#include "version.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <openssl/sha.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace phosphor
{
namespace software
{
namespace manager
{

using namespace phosphor::logging;
using Argument = xyz::openbmc_project::Common::InvalidArgument;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

std::string Version::getValue(const std::string& manifestFilePath,
                              std::string key)
{
    key = key + "=";
    auto keySize = key.length();

    if (manifestFilePath.empty())
    {
        log<level::ERR>("Error MANIFESTFilePath is empty");
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("manifestFilePath"),
            Argument::ARGUMENT_VALUE(manifestFilePath.c_str()));
    }

    std::string value{};
    std::ifstream efile;
    std::string line;
    efile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                     std::ifstream::eofbit);

    // Too many GCC bugs (53984, 66145) to do this the right way...
    try
    {
        efile.open(manifestFilePath);
        while (getline(efile, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                // If the manifest has CRLF line terminators, e.g. is created on
                // Windows, the line will contain \r at the end, remove it.
                line.pop_back();
            }
            if (line.compare(0, keySize, key) == 0)
            {
                value = line.substr(keySize);
                break;
            }
        }
        efile.close();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Error in reading MANIFEST file");
    }

    return value;
}

std::string Version::getId(const std::string& version)
{

    if (version.empty())
    {
        log<level::ERR>("Error version is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("Version"),
                              Argument::ARGUMENT_VALUE(version.c_str()));
    }

    unsigned char digest[SHA512_DIGEST_LENGTH];
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, version.c_str(), strlen(version.c_str()));
    SHA512_Final(digest, &ctx);
    char mdString[SHA512_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        snprintf(&mdString[i * 2], 3, "%02x", (unsigned int)digest[i]);
    }

    // Only need 8 hex digits.
    std::string hexId = std::string(mdString);
    return (hexId.substr(0, 8));
}

std::string Version::getBMCMachine(const std::string& releaseFilePath)
{
    std::string machineKey = "OPENBMC_TARGET_MACHINE=";
    std::string machine{};
    std::ifstream efile(releaseFilePath);
    std::string line;

    while (getline(efile, line))
    {
        if (line.substr(0, machineKey.size()).find(machineKey) !=
            std::string::npos)
        {
            std::size_t pos = line.find_first_of('"') + 1;
            machine = line.substr(pos, line.find_last_of('"') - pos);
            break;
        }
    }

    if (machine.empty())
    {
        log<level::ERR>("Unable to find OPENBMC_TARGET_MACHINE");
        elog<InternalFailure>();
    }

    return machine;
}

std::string Version::getBMCVersion(const std::string& releaseFilePath)
{
    std::string versionKey = "VERSION_ID=";
    std::string versionValue{};
    std::string version{};
    std::ifstream efile;
    std::string line;
    efile.open(releaseFilePath);

    while (getline(efile, line))
    {
        if (line.substr(0, versionKey.size()).find(versionKey) !=
            std::string::npos)
        {
            // Support quoted and unquoted values
            // 1. Remove the versionKey so that we process the value only.
            versionValue = line.substr(versionKey.size());

            // 2. Look for a starting quote, then increment the position by 1 to
            //    skip the quote character. If no quote is found,
            //    find_first_of() returns npos (-1), which by adding +1 sets pos
            //    to 0 (beginning of unquoted string).
            std::size_t pos = versionValue.find_first_of('"') + 1;

            // 3. Look for ending quote, then decrease the position by pos to
            //    get the size of the string up to before the ending quote. If
            //    no quote is found, find_last_of() returns npos (-1), and pos
            //    is 0 for the unquoted case, so substr() is called with a len
            //    parameter of npos (-1) which according to the documentation
            //    indicates to use all characters until the end of the string.
            version =
                versionValue.substr(pos, versionValue.find_last_of('"') - pos);
            break;
        }
    }
    efile.close();

    if (version.empty())
    {
        log<level::ERR>("Error BMC current version is empty");
        elog<InternalFailure>();
    }

    return version;
}

bool Version::isFunctional()
{
    return versionStr == getBMCVersion(OS_RELEASE_FILE);
}

void Delete::delete_()
{
    if (parent.eraseCallback)
    {
        parent.eraseCallback(parent.getId(parent.version()));
    }
}

} // namespace manager
} // namespace software
} // namespace phosphor
