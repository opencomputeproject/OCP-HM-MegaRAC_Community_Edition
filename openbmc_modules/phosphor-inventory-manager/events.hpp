#pragma once

#include "types.hpp"

#include <memory>
#include <vector>

namespace phosphor
{
namespace inventory
{
namespace manager
{

class Manager;

/** @struct Event
 *  @brief Event object interface.
 *
 *  The event base is an association of an event type
 *  and an array of filter callbacks.
 */
struct Event : public std::vector<Filter>
{
    enum class Type
    {
        DBUS_SIGNAL,
        STARTUP,
    };

    virtual ~Event() = default;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;

    /** @brief Event constructor.
     *
     *  @param[in] filters - An array of filter callbacks.
     *  @param[in] t - The event type.
     */
    explicit Event(const std::vector<Filter>& filters, Type t = Type::STARTUP) :
        std::vector<Filter>(filters), type(t)
    {
    }

    /** @brief event class enumeration. */
    Type type;
};

using StartupEvent = Event;

using EventBasePtr = std::shared_ptr<Event>;

/** @struct DbusSignal
 *  @brief DBus signal event.
 *
 *  DBus signal events are an association of a match signature
 *  and filtering function object.
 */
struct DbusSignal final : public Event
{
    ~DbusSignal() = default;
    DbusSignal(const DbusSignal&) = delete;
    DbusSignal& operator=(const DbusSignal&) = delete;
    DbusSignal(DbusSignal&&) = default;
    DbusSignal& operator=(DbusSignal&&) = default;

    /** @brief Import from signature and filter constructor.
     *
     *  @param[in] sig - The DBus match signature.
     *  @param[in] filter - An array of DBus signal
     *     match callback filtering functions.
     */
    DbusSignal(const char* sig, const std::vector<Filter>& filters) :
        Event(filters, Type::DBUS_SIGNAL), signature(sig)
    {
    }

    const char* signature;
};

} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
