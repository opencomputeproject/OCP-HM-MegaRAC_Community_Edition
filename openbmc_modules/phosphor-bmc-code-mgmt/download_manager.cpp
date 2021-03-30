#include "config.h"

#include "download_manager.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <sys/wait.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

namespace phosphor
{
namespace software
{
namespace manager
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;
using namespace phosphor::logging;
namespace fs = std::filesystem;

void Download::downloadViaTFTP(std::string fileName, std::string serverAddress)
{
    using Argument = xyz::openbmc_project::Common::InvalidArgument;

    // Sanitize the fileName string
    if (!fileName.empty())
    {
        fileName.erase(std::remove(fileName.begin(), fileName.end(), '/'),
                       fileName.end());
        fileName = fileName.substr(fileName.find_first_not_of('.'));
    }

    if (fileName.empty())
    {
        log<level::ERR>("Error FileName is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("FileName"),
                              Argument::ARGUMENT_VALUE(fileName.c_str()));
        return;
    }

    if (serverAddress.empty())
    {
        log<level::ERR>("Error ServerAddress is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("ServerAddress"),
                              Argument::ARGUMENT_VALUE(serverAddress.c_str()));
        return;
    }

    log<level::INFO>("Downloading via TFTP",
                     entry("FILENAME=%s", fileName.c_str()),
                     entry("SERVERADDRESS=%s", serverAddress.c_str()));

    // Check if IMAGE DIR exists
    fs::path imgDirPath(IMG_UPLOAD_DIR);
    if (!fs::is_directory(imgDirPath))
    {
        log<level::ERR>("Error Image Dir does not exist");
        elog<InternalFailure>();
        return;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        pid_t nextPid = fork();
        if (nextPid == 0)
        {
            // child process
            execl("/usr/bin/tftp", "tftp", "-g", "-r", fileName.c_str(),
                  serverAddress.c_str(), "-l",
                  (std::string{IMG_UPLOAD_DIR} + '/' + fileName).c_str(),
                  (char*)0);
            // execl only returns on fail
            log<level::ERR>("Error occurred during the TFTP call");
            elog<InternalFailure>();
        }
        else if (nextPid < 0)
        {
            log<level::ERR>("Error occurred during fork");
            elog<InternalFailure>();
        }
        // do nothing as parent if all is going well
        // when parent exits, child will be reparented under init
        // and then be reaped properly
        exit(0);
    }
    else if (pid < 0)
    {
        log<level::ERR>("Error occurred during fork");
        elog<InternalFailure>();
    }
    else
    {
        int status;
        if (waitpid(pid, &status, 0) < 0)
        {
            log<level::ERR>("waitpid error");
        }
        else if (WEXITSTATUS(status) != 0)
        {
            log<level::ERR>("failed to launch tftp");
        }
    }

    return;
}

} // namespace manager
} // namespace software
} // namespace phosphor
