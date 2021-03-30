#pragma once

#include "dns_updater.hpp"
#include "types.hpp"
#include "util.hpp"

#include <sys/inotify.h>
#include <systemd/sd-event.h>

#include <filesystem>
#include <functional>
#include <map>

namespace phosphor
{
namespace network
{
namespace inotify
{

namespace fs = std::filesystem;

// Auxiliary callback to be invoked on inotify events
using UserCallBack = std::function<void(const std::string&)>;

/** @class Watch
 *
 *  @brief Adds inotify watch on directory
 *
 *  @details Calls back user function on matching events
 */
class Watch
{
  public:
    Watch() = delete;
    Watch(const Watch&) = delete;
    Watch& operator=(const Watch&) = delete;
    Watch(Watch&&) = delete;
    Watch& operator=(Watch&&) = delete;

    /** @brief Hooks inotify watch with sd-event
     *
     *  @param[in] eventPtr - Reference to sd_event wrapped in unique_ptr
     *  @param[in] path     - File path to be watched
     *  @param[in] userFunc - User specific callback function on events
     *  @param[in] flags    - Flags to be supplied to inotify
     *  @param[in] mask     - Mask of events to be supplied to inotify
     *  @param[in] events   - Events to be watched
     */
    Watch(phosphor::network::EventPtr& eventPtr, const fs::path path,
          UserCallBack userFunc, int flags = IN_NONBLOCK,
          uint32_t mask = IN_CLOSE_WRITE, uint32_t events = EPOLLIN);

    /** @brief Remove inotify watch and close fd's */
    ~Watch()
    {
        if ((fd() >= 0) && (wd >= 0))
        {
            inotify_rm_watch(fd(), wd);
        }
    }

  private:
    /** @brief Callback invoked when inotify event fires
     *
     *  @details On a matching event, calls back into user supplied
     *           function if there is one registered
     *
     *  @param[in] eventSource - Event source
     *  @param[in] fd          - Inotify fd
     *  @param[in] retEvents   - Events that matched for fd
     *  @param[in] userData    - Pointer to Watch object
     *
     *  @returns 0 on success, -1 on fail
     */
    static int processEvents(sd_event_source* eventSource, int fd,
                             uint32_t retEvents, void* userData);

    /** @brief Initializes an inotify instance
     *
     *  @return Descriptor on success, -1 on failure
     */
    int inotifyInit();

    /** @brief File path to be watched */
    const fs::path path;

    /** @brief User callback function */
    UserCallBack userFunc;

    /** @brief Inotify flags */
    int flags;

    /** @brief Mask of events */
    uint32_t mask;

    /** @brief Events to be watched */
    uint32_t events;

    /** @brief Watch descriptor */
    int wd = -1;

    /** @brief File descriptor manager */
    phosphor::Descriptor fd;
};

} // namespace inotify
} // namespace network
} // namespace phosphor
