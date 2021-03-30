#pragma once

#include "config.h"

#include "dump_entry.hpp"
#include "dump_utils.hpp"
#include "watch.hpp"
#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"
#include "xyz/openbmc_project/Dump/Internal/Create/server.hpp"
#include "xyz/openbmc_project/Dump/NewDump/server.hpp"

#include <experimental/filesystem>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Dump/Create/server.hpp>

namespace phosphor
{
namespace dump
{
namespace internal
{

class Manager;

} // namespace internal

using UserMap = phosphor::dump::inotify::UserMap;

using Type =
    sdbusplus::xyz::openbmc_project::Dump::Internal::server::Create::Type;

using CreateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll,
    sdbusplus::xyz::openbmc_project::Dump::server::Create,
    sdbusplus::xyz::openbmc_project::Dump::server::NewDump>;

namespace fs = std::experimental::filesystem;

using Watch = phosphor::dump::inotify::Watch;

// Type to dreport type  string map
static const std::map<Type, std::string> TypeMap = {
    {Type::ApplicationCored, "core"},
    {Type::UserRequested, "user"},
    {Type::InternalFailure, "elog"},
    {Type::Checkstop, "checkstop"}};

/** @class Manager
 *  @brief OpenBMC Dump  manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Dump.Create DBus API and
 *  xyz::openbmc_project::Collection::server::DeleteAll.
 */
class Manager : public CreateIface
{
    friend class internal::Manager;
    friend class Entry;

  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] event - Dump manager sd_event loop.
     *  @param[in] path - Path to attach at.
     */
    Manager(sdbusplus::bus::bus& bus, const EventPtr& event, const char* path) :
        CreateIface(bus, path), bus(bus), eventLoop(event.get()),
        lastEntryId(0),
        dumpWatch(
            eventLoop, IN_NONBLOCK, IN_CLOSE_WRITE | IN_CREATE, EPOLLIN,
            BMC_DUMP_PATH,
            std::bind(std::mem_fn(&phosphor::dump::Manager::watchCallback),
                      this, std::placeholders::_1))
    {
    }

    /** @brief Implementation for CreateDump
     *  Method to create Dump.
     *
     *  @return id - The Dump entry id number.
     */
    uint32_t createDump() override;

    /** @brief Implementation of dump watch call back
     *  @param [in] fileInfo - map of file info  path:event
     */
    void watchCallback(const UserMap& fileInfo);

    /** @brief Construct dump d-bus objects from their persisted
     *        representations.
     */
    void restore();

    /** @brief Notify the dump manager about creation of a new dump.
     *  @param[in] dumpType - Type of the Dump.
     *  @param[in] dumpId - Id from the source of the dump.
     *  @param[in] size - Size of the dump.
     */
    void notify(NewDump::DumpType dumpType, uint32_t dumpId, uint64_t size);

  private:
    /** @brief Create Dump entry d-bus object
     *  @param[in] fullPath - Full path of the Dump file name
     */
    void createEntry(const fs::path& fullPath);

    /**  @brief Capture BMC Dump based on the Dump type.
     *  @param[in] type - Type of the Dump.
     *  @param[in] fullPaths - List of absolute paths to the files
     *             to be included as part of Dump package.
     *  @return id - The Dump entry id number.
     */
    uint32_t captureDump(Type type, const std::vector<std::string>& fullPaths);

    /** @brief Erase specified entry d-bus object
     *
     * @param[in] entryId - unique identifier of the entry
     */
    void erase(uint32_t entryId);

    /** @brief  Erase all BMC dump entries and  Delete all Dump files
     * from Permanent location
     *
     */
    void deleteAll() override;

    /** @brief sd_event_add_child callback
     *
     *  @param[in] s - event source
     *  @param[in] si - signal info
     *  @param[in] userdata - pointer to Watch object
     *
     *  @returns 0 on success, -1 on fail
     */
    static int callback(sd_event_source* s, const siginfo_t* si, void* userdata)
    {
        // No specific action required in
        // the sd_event_add_child callback.
        return 0;
    }
    /** @brief Remove specified watch object pointer from the
     *        watch map and associated entry from the map.
     *        @param[in] path - unique identifier of the map
     */
    void removeWatch(const fs::path& path);

    /** @brief Calculate per dump allowed size based on the available
     *        size in the dump location.
     *  @returns dump size in kilobytes.
     */
    size_t getAllowedSize();

    /** @brief sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief sdbusplus Dump event loop */
    EventPtr eventLoop;

    /** @brief Dump Entry dbus objects map based on entry id */
    std::map<uint32_t, std::unique_ptr<Entry>> entries;

    /** @brief Id of the last Dump entry */
    uint32_t lastEntryId;

    /** @brief Dump main watch object */
    Watch dumpWatch;

    /** @brief Child directory path and its associated watch object map
     *        [path:watch object]
     */
    std::map<fs::path, std::unique_ptr<Watch>> childWatchMap;
};

} // namespace dump
} // namespace phosphor
