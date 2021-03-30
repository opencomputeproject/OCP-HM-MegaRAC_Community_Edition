#pragma once
#include "watch.hpp"

#include <memory>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/io.hpp>
namespace phosphor
{
namespace certs
{
/** @class Watch
 *
 *  @brief Adds inotify watch on certificate directory
 *
 *  The inotify watch is hooked up with sd-event, so that on call back,
 *  appropriate actions related to a certificate upload can be taken.
 */
class Watch
{
  public:
    using Callback = std::function<void()>;
    /** @brief ctor - hook inotify watch with sd-event
     *
     *  @param[in] loop - sd-event object
     *  @param[in] cb - The callback function for processing
     *                             certificate upload
     */
    Watch(sdeventplus::Event& event, std::string& certFile, Callback cb);
    Watch(const Watch&) = delete;
    Watch& operator=(const Watch&) = delete;
    Watch(Watch&&) = delete;
    Watch& operator=(Watch&&) = delete;

    /** @brief dtor - remove inotify watch and close fd's
     */
    ~Watch();

    /** @brief start watch on the specified path
     */
    void startWatch();

    /** @brief stop watch on the specified path
     */
    void stopWatch();

  private:
    /** @brief certificate upload directory watch descriptor */
    int wd = -1;

    /** @brief inotify file descriptor */
    int fd = -1;

    /** @brief SDEventPlus IO pointer added to event loop */
    std::unique_ptr<sdeventplus::source::IO> ioPtr = nullptr;

    /** @brief sd-event object */
    sdeventplus::Event& event;

    /** @brief callback method to be called */
    Callback callback;

    /** @brief Certificate directory to watch */
    std::string watchDir;

    /** @brief Certificate file to watch */
    std::string watchFile;

    /** @brief Certificate file with path */
    std::string certFile;
};
} // namespace certs
} // namespace phosphor
