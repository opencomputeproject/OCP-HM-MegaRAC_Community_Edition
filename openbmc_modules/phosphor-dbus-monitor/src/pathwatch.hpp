/**
 * @file PathWatch.hpp
 * @brief Add watch for the object path for interfaces added/removed signal
 *
 * In general class users should include pathwatchimpl.hpp instead to avoid
 * link failures.
 */
#pragma once

#include "data_types.hpp"
#include "watch.hpp"

#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

class Callback;

/** @class PathWatch
 *  @brief Watch on object path for interfaceadded/interfaceremoved signals
 */
template <typename DBusInterfaceType>
class PathWatch : public Watch
{
  public:
    PathWatch() = delete;
    PathWatch(const PathWatch&) = delete;
    PathWatch(PathWatch&&) = default;
    PathWatch& operator=(const PathWatch&) = delete;
    PathWatch& operator=(PathWatch&&) = default;
    virtual ~PathWatch() = default;
    PathWatch(const std::string& path, Callback& callback) :
        Watch(), objectPath(path), cb(callback), alreadyRan(false)
    {
    }

    /** @brief Start the watch.
     *
     *  Watch start interface implementation for PathWatch.
     */
    void start() override;

    /** @brief Run the watch callback method.
     *
     *  Watch callback interface implementation for PathWatch.
     */
    void callback(Context ctx) override;

    /** @brief Run the watch callback method.
     *
     *  Watch callback interface implementation for PathWatch.
     */
    void callback(Context ctx, sdbusplus::message::message& msg) override;

  protected:
    /** @brief Path of the D-Bus object to watch for. */
    const std::string& objectPath;

    /** @brief Optional callback method. */
    Callback& cb;

    /** @brief The start method should only be invoked once. */
    bool alreadyRan;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
