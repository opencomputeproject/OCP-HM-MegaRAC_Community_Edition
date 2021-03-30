#include "config.h"

#include "sync_manager.hpp"

#include <sys/inotify.h>
#include <sys/wait.h>
#include <unistd.h>

#include <phosphor-logging/log.hpp>

#include <filesystem>

namespace phosphor
{
namespace software
{
namespace manager
{

using namespace phosphor::logging;
namespace fs = std::filesystem;

int Sync::processEntry(int mask, const fs::path& entryPath)
{
    int status{};
    pid_t pid = fork();

    if (pid == 0)
    {
        fs::path dst(ALT_RWFS);
        dst /= entryPath.relative_path();

        // rsync needs an additional --delete argument to handle file deletions
        // so need to differentiate between the different file events.
        if (mask & IN_CLOSE_WRITE)
        {
            if (!(fs::exists(dst)))
            {
                if (fs::is_directory(entryPath))
                {
                    // Source is a directory, create it at the destination.
                    fs::create_directories(dst);
                }
                else
                {
                    // Source is a file, create the directory where this file
                    // resides at the destination.
                    fs::create_directories(dst.parent_path());
                }
            }

            execl("/usr/bin/rsync", "rsync", "-a", entryPath.c_str(),
                  dst.c_str(), nullptr);
            // execl only returns on fail
            log<level::ERR>("Error occurred during the rsync call",
                            entry("ERRNO=%d", errno),
                            entry("PATH=%s", entryPath.c_str()));
            return -1;
        }
        else if (mask & IN_DELETE)
        {
            execl("/usr/bin/rsync", "rsync", "-a", "--delete",
                  entryPath.c_str(), dst.c_str(), nullptr);
            // execl only returns on fail
            log<level::ERR>("Error occurred during the rsync delete call",
                            entry("ERRNO=%d", errno),
                            entry("PATH=%s", entryPath.c_str()));
            return -1;
        }
    }
    else if (pid > 0)
    {
        waitpid(pid, &status, 0);
    }
    else
    {
        log<level::ERR>("Error occurred during fork", entry("ERRNO=%d", errno));
        return -1;
    }

    return 0;
}

} // namespace manager
} // namespace software
} // namespace phosphor
