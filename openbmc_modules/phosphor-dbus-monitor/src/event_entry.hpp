#pragma once

#include "sdbusplus.hpp"
#include "xyz/openbmc_project/Logging/Event/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>

namespace phosphor
{
namespace events
{

using namespace phosphor::dbus::monitoring;

using EntryIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Logging::server::Event>;

/** @class Entry
 *  @brief OpenBMC Event entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Event.Entry.
 */
class Entry : public EntryIface
{
  public:
    Entry() = delete;
    Entry(const Entry&) = delete;
    Entry& operator=(const Entry&) = delete;
    Entry(Entry&&) = delete;
    Entry& operator=(Entry&&) = delete;
    virtual ~Entry() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] path - Path to attach at.
     *  @param[in] eventId - The event entry id.
     *  @param[in] timestamp - timestamp when the event created.
     *  @param[in] msg - The message of the event.
     *  @param[in] metaData - The event metadata.
     */
    Entry(const std::string& path, uint64_t eventTimestamp, std::string&& msg,
          std::vector<std::string>&& metaData) :
        EntryIface(SDBusPlus::getBus(), path.c_str(), true),
        objectPath(path)
    {
        timestamp(eventTimestamp);
        message(msg);
        additionalData(metaData);
        // Emit deferred signal.
        this->emit_object_added();
    }

    /** @brief Constructor to create an empty event object with only
     *  timestamp, caller should make a call to emit added signal.
     *  @param[in] path - Path to attach at.
     *  @param[in] timestamp - timestamp when the event created.
     */
    Entry(const std::string& path, uint64_t eventTimestamp) :
        EntryIface(SDBusPlus::getBus(), path.c_str(), true), objectPath(path)
    {
        timestamp(eventTimestamp);
    }

    /** @brief Path of Object. */
    std::string objectPath;
};

} // namespace events
} // namespace phosphor
