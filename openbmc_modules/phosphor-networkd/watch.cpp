#include "watch.hpp"

#include <errno.h>
#include <sys/inotify.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace network
{
namespace inotify
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

Watch::Watch(phosphor::network::EventPtr& eventPtr, fs::path path,
             UserCallBack userFunc, int flags, uint32_t mask, uint32_t events) :
    path(path),
    userFunc(userFunc), flags(flags), mask(mask), events(events),
    fd(inotifyInit())
{
    // Check if watch file exists
    // This is supposed to be there always
    if (!fs::is_regular_file(path))
    {
        log<level::ERR>("Watch file doesn't exist",
                        entry("FILE=%s", path.c_str()));
        elog<InternalFailure>();
    }

    auto dirPath = path.parent_path();
    wd = inotify_add_watch(fd(), dirPath.c_str(), mask);
    if (wd == -1)
    {
        log<level::ERR>("Error from inotify_add_watch",
                        entry("ERRNO=%d", errno));
        elog<InternalFailure>();
    }

    // Register the fd with sd_event infrastructure and setup a
    // callback handler to be invoked on events
    auto rc = sd_event_add_io(eventPtr.get(), nullptr, fd(), events,
                              Watch::processEvents, this);
    if (rc < 0)
    {
        // Failed to add to event loop
        log<level::ERR>("Error registering with sd_event_add_io",
                        entry("RC=%d", rc));
        elog<InternalFailure>();
    }
}

int Watch::inotifyInit()
{
    auto fd = inotify_init1(flags);
    if (fd < 0)
    {
        log<level::ERR>("Error from inotify_init1", entry("ERRNO=%d", errno));
        elog<InternalFailure>();
    }
    return fd;
}

int Watch::processEvents(sd_event_source* /*eventSource*/, int fd,
                         uint32_t retEvents, void* userData)
{
    auto watch = static_cast<Watch*>(userData);

    // Not the ones we are interested in
    if (!(retEvents & watch->events))
    {
        return 0;
    }

    // Buffer size to be used while reading events.
    // per inotify(7), below number should be fine for reading
    // at-least one event
    constexpr auto maxBytes = sizeof(struct inotify_event) + NAME_MAX + 1;
    uint8_t eventData[maxBytes]{};

    auto bytes = read(fd, eventData, maxBytes);
    if (bytes <= 0)
    {
        // Failed to read inotify event data
        // Report error and return
        log<level::ERR>("Error reading inotify event",
                        entry("ERRNO=%d", errno));
        report<InternalFailure>();
        return 0;
    }

    auto offset = 0;
    auto stateFile = watch->path.filename();
    while (offset < bytes)
    {
        auto event = reinterpret_cast<inotify_event*>(&eventData[offset]);

        // Filter the interesting ones
        auto mask = event->mask & watch->mask;
        if (mask)
        {
            if ((event->len > 0) &&
                (strstr(event->name, stateFile.string().c_str())))
            {
                if (watch->userFunc)
                {
                    watch->userFunc(watch->path);
                }
                // Found the event of interest
                break;
            }
        }
        // Move past this entry
        offset += offsetof(inotify_event, name) + event->len;
    }
    return 0;
}

} // namespace inotify
} // namespace network
} // namespace phosphor
