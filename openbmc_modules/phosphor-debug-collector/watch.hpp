#pragma once

#include "dump_utils.hpp"

#include <sys/inotify.h>
#include <systemd/sd-event.h>

#include <experimental/filesystem>
#include <functional>
#include <map>

namespace phosphor
{
namespace dump
{
namespace inotify
{

namespace fs = std::experimental::filesystem;

// User specific call back function input map(path:event) type.
using UserMap = std::map<fs::path, uint32_t>;

// User specific callback function wrapper type.
using UserType = std::function<void(const UserMap&)>;

/** @class Watch
 *
 *  @brief Adds inotify watch on directory.
 *
 *  The inotify watch is hooked up with sd-event, so that on call back,
 *  appropriate actions are taken to collect files from the directory
 *  initialized by the object.
 */
class Watch
{
  public:
    /** @brief ctor - hook inotify watch with sd-event
     *
     *  @param[in] eventObj - Event loop object
     *  @param[in] flags - inotify flags
     *  @param[in] mask  - Mask of events
     *  @param[in] events - Events to be watched
     *  @param[in] path - File path to be watched
     *  @param[in] userFunc - User specific callback fnction wrapper.
     *
     */
    Watch(const EventPtr& eventObj, int flags, uint32_t mask, uint32_t events,
          const fs::path& path, UserType userFunc);

    Watch(const Watch&) = delete;
    Watch& operator=(const Watch&) = delete;
    Watch(Watch&&) = default;
    Watch& operator=(Watch&&) = default;

    /* @brief dtor - remove inotify watch and close fd's */
    ~Watch();

  private:
    /** @brief sd-event callback.
     *  @details Collects the files and event info and call the
     *           appropriate user function for further action.
     *
     *  @param[in] s - event source, floating (unused) in our case
     *  @param[in] fd - inotify fd
     *  @param[in] revents - events that matched for fd
     *  @param[in] userdata - pointer to Watch object
     *
     *  @returns 0 on success, -1 on fail
     */
    static int callback(sd_event_source* s, int fd, uint32_t revents,
                        void* userdata);

    /**  initialize an inotify instance and returns file descriptor */
    int inotifyInit();

    /** @brief inotify flags */
    int flags;

    /** @brief Mask of events */
    uint32_t mask;

    /** @brief Events to be watched */
    uint32_t events;

    /** @brief File path to be watched */
    fs::path path;

    /** @brief dump file directory watch descriptor */
    int wd = -1;

    /** @brief file descriptor manager */
    CustomFd fd;

    /** @brief The user level callback function wrapper */
    UserType userFunc;
};

} // namespace inotify
} // namespace dump
} // namespace phosphor
