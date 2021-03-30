#pragma once

#include "data_types.hpp"

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class Watch
 *  @brief Watch interface.
 *
 *  The start method is invoked by main() on all watches of any type
 *  at application startup, to allow watches to perform custom setup
 *  or initialization.  Typical implementations might register dbus
 *  callbacks or perform queries.
 *
 *  The callback method is invoked by main() on all watches of any
 *  type at application startup, after all watches have performed
 *  their setup.  Typical implementations will forward the call
 *  to their associated callback.
 */
class Watch
{
  public:
    Watch() = default;
    Watch(const Watch&) = default;
    Watch(Watch&&) = default;
    Watch& operator=(const Watch&) = default;
    Watch& operator=(Watch&&) = default;
    virtual ~Watch() = default;

    /** @brief Start the watch. */
    virtual void start() = 0;

    /** @brief Invoke the callback associated with the watch. */
    virtual void callback(Context ctx) = 0;

    /** @brief Invoke the callback associated with the watch. */
    virtual void callback(Context ctx, sdbusplus::message::message& msg){};
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
