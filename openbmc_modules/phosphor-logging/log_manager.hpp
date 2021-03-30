#pragma once

#include "elog_block.hpp"
#include "elog_entry.hpp"
#include "xyz/openbmc_project/Collection/DeleteAll/server.hpp"
#include "xyz/openbmc_project/Logging/Create/server.hpp"
#include "xyz/openbmc_project/Logging/Entry/server.hpp"
#include "xyz/openbmc_project/Logging/Internal/Manager/server.hpp"

#include <list>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace logging
{

extern const std::map<std::string, std::vector<std::string>> g_errMetaMap;
extern const std::map<std::string, level> g_errLevelMap;

using CreateIface = sdbusplus::xyz::openbmc_project::Logging::server::Create;
using DeleteAllIface =
    sdbusplus::xyz::openbmc_project::Collection::server::DeleteAll;

namespace details
{
template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;

using ManagerIface =
    sdbusplus::xyz::openbmc_project::Logging::Internal::server::Manager;

} // namespace details

constexpr size_t ffdcFormatPos = 0;
constexpr size_t ffdcSubtypePos = 1;
constexpr size_t ffdcVersionPos = 2;
constexpr size_t ffdcFDPos = 3;

using FFDCEntry = std::tuple<CreateIface::FFDCFormat, uint8_t, uint8_t,
                             sdbusplus::message::unix_fd>;

using FFDCEntries = std::vector<FFDCEntry>;

namespace internal
{

/** @class Manager
 *  @brief OpenBMC logging manager implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Logging.Internal.Manager DBus API.
 */
class Manager : public details::ServerObject<details::ManagerIface>
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     */
    Manager(sdbusplus::bus::bus& bus, const char* objPath) :
        details::ServerObject<details::ManagerIface>(bus, objPath), busLog(bus),
        entryId(0), fwVersion(readFWVersion()){};

    /*
     * @fn commit()
     * @brief sd_bus Commit method implementation callback.
     * @details Create an error/event log based on transaction id and
     *          error message.
     * @param[in] transactionId - Unique identifier of the journal entries
     *                            to be committed.
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     */
    void commit(uint64_t transactionId, std::string errMsg) override;

    /*
     * @fn commit()
     * @brief sd_bus CommitWithLvl method implementation callback.
     * @details Create an error/event log based on transaction id and
     *          error message.
     * @param[in] transactionId - Unique identifier of the journal entries
     *                            to be committed.
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] errLvl - level of the error
     */
    void commitWithLvl(uint64_t transactionId, std::string errMsg,
                       uint32_t errLvl) override;

    /** @brief Erase specified entry d-bus object
     *
     * @param[in] entryId - unique identifier of the entry
     */
    void erase(uint32_t entryId);

    /** @brief Construct error d-bus objects from their persisted
     *         representations.
     */
    void restore();

    /** @brief  Erase all error log entries
     *
     */
    void eraseAll()
    {
        auto iter = entries.begin();
        while (iter != entries.end())
        {
            auto e = iter->first;
            ++iter;
            erase(e);
        }
    }

    /** @brief Returns the count of high severity errors
     *
     *  @return int - count of real errors
     */
    int getRealErrSize();

    /** @brief Returns the count of Info errors
     *
     *  @return int - count of info errors
     */
    int getInfoErrSize();

    /** @brief Returns the number of blocking errors
     *
     *  @return int - count of blocking errors
     */
    int getBlockingErrSize()
    {
        return blockingErrors.size();
    }

    /** @brief Returns the number of property change callback objects
     *
     *  @return int - count of property callback entries
     */
    int getEntryCallbackSize()
    {
        return propChangedEntryCallback.size();
    }

    sdbusplus::bus::bus& getBus()
    {
        return busLog;
    }

    /** @brief Creates an event log
     *
     *  This is an alternative to the _commit() API.  It doesn't use
     *  the journal to look up event log metadata like _commit does.
     *
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] severity - level of the error
     * @param[in] additionalData - The AdditionalData property for the error
     */
    void create(
        const std::string& message,
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level severity,
        const std::map<std::string, std::string>& additionalData);

    /** @brief Creates an event log, and accepts FFDC files
     *
     * This is the same as create(), but also takes an FFDC argument.
     *
     * The FFDC argument is a vector of tuples that allows one to pass in file
     * descriptors for files that contain FFDC (First Failure Data Capture).
     * These will be passed to any event logging extensions.
     *
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] severity - level of the error
     * @param[in] additionalData - The AdditionalData property for the error
     * @param[in] ffdc - A vector of FFDC file info
     */
    void createWithFFDC(
        const std::string& message,
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level severity,
        const std::map<std::string, std::string>& additionalData,
        const FFDCEntries& ffdc);

    /** @brief Common wrapper for creating an Entry object
     *
     * @return true if quiesce on error setting is enabled, false otherwise
     */
    bool isQuiesceOnErrorEnabled();

    /** @brief Check if error has callout and if so, block boot
     *
     * @param[in] entry - The error to check for callouts
     */
    void checkQuiesceOnError(const Entry& entry);

    /** @brief Check if inventory callout present in input entry
     *
     * @param[in] entry - The error to check for callouts
     *
     * @return true if inventory item in associations, false otherwise
     */
    bool isCalloutPresent(const Entry& entry);

    /** @brief Check (and remove) entry being erased from blocking errors
     *
     * @param[in] entryId - The entry that is being erased
     */
    void checkAndRemoveBlockingError(uint32_t entryId);

  private:
    /*
     * @fn _commit()
     * @brief commit() helper
     * @param[in] transactionId - Unique identifier of the journal entries
     *                            to be committed.
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] errLvl - level of the error
     */
    void _commit(uint64_t transactionId, std::string&& errMsg,
                 Entry::Level errLvl);

    /** @brief Call metadata handler(s), if any. Handlers may create
     *         associations.
     *  @param[in] errorName - name of the error
     *  @param[in] additionalData - list of metadata (in key=value format)
     *  @param[out] objects - list of error's association objects
     */
    void processMetadata(const std::string& errorName,
                         const std::vector<std::string>& additionalData,
                         AssociationList& objects) const;

    /** @brief Synchronize unwritten journal messages to disk.
     *  @details This is the same implementation as the systemd command
     *  "journalctl --sync".
     */
    void journalSync();

    /** @brief Reads the BMC code level
     *
     *  @return std::string - the version string
     */
    static std::string readFWVersion();

    /** @brief Call any create() functions provided by any extensions.
     *  This is called right after an event log is created to allow
     *  extensions to create their own log based on this one.
     *
     *  @param[in] entry - the new event log entry
     *  @param[in] ffdc - A vector of FFDC file info
     */
    void doExtensionLogCreate(const Entry& entry, const FFDCEntries& ffdc);

    /** @brief Common wrapper for creating an Entry object
     *
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] errLvl - level of the error
     * @param[in] additionalData - The AdditionalData property for the error
     * @param[in] ffdc - A vector of FFDC file info. Defaults to an empty
     * vector.
     */
    void createEntry(std::string errMsg, Entry::Level errLvl,
                     std::vector<std::string> additionalData,
                     const FFDCEntries& ffdc = FFDCEntries{});

    /** @brief Notified on entry property changes
     *
     * If an entry is blocking, this callback will be registered to monitor for
     * the entry having it's Resolved field set to true. If it is then remove
     * the blocking object.
     *
     * @param[in] msg - sdbusplus dbusmessage
     */
    void onEntryResolve(sdbusplus::message::message& msg);

    /** @brief Remove block objects for any resolved entries  */
    void findAndRemoveResolvedBlocks();

    /** @brief Quiesce host if it is running
     *
     * This is called when the user has requested the system be quiesced
     * if a log with a callout is created
     */
    void checkAndQuiesceHost();

    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus::bus& busLog;

    /** @brief Persistent map of Entry dbus objects and their ID */
    std::map<uint32_t, std::unique_ptr<Entry>> entries;

    /** @brief List of error ids for high severity errors */
    std::list<uint32_t> realErrors;

    /** @brief List of error ids for Info(and below) severity */
    std::list<uint32_t> infoErrors;

    /** @brief Id of last error log entry */
    uint32_t entryId;

    /** @brief The BMC firmware version */
    const std::string fwVersion;

    /** @brief Array of blocking errors */
    std::vector<std::unique_ptr<Block>> blockingErrors;

    /** @brief Map of entry id to call back object on properties changed */
    std::map<uint32_t, std::unique_ptr<sdbusplus::bus::match::match>>
        propChangedEntryCallback;
};

} // namespace internal

/** @class Manager
 *  @brief Implementation for deleting all error log entries and
 *         creating new logs.
 *  @details A concrete implementation for the
 *           xyz.openbmc_project.Collection.DeleteAll and
 *           xyz.openbmc_project.Logging.Create interfaces.
 */
class Manager : public details::ServerObject<DeleteAllIface, CreateIface>
{
  public:
    Manager() = delete;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;
    virtual ~Manager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *         Defer signal registration (pass true for deferSignal to the
     *         base class) until after the properties are set.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] manager - Reference to internal manager object.
     */
    Manager(sdbusplus::bus::bus& bus, const std::string& path,
            internal::Manager& manager) :
        details::ServerObject<DeleteAllIface, CreateIface>(bus, path.c_str(),
                                                           true),
        manager(manager){};

    /** @brief Delete all d-bus objects.
     */
    void deleteAll()
    {
        manager.eraseAll();
    }

    /** @brief D-Bus method call implementation to create an event log.
     *
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] severity - Level of the error
     * @param[in] additionalData - The AdditionalData property for the error
     */
    void create(
        std::string message,
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level severity,
        std::map<std::string, std::string> additionalData) override
    {
        manager.create(message, severity, additionalData);
    }

    /** @brief D-Bus method call implementation to create an event log with FFDC
     *
     * The same as create(), but takes an extra FFDC argument.
     *
     * @param[in] errMsg - The error exception message associated with the
     *                     error log to be committed.
     * @param[in] severity - Level of the error
     * @param[in] additionalData - The AdditionalData property for the error
     * @param[in] ffdc - A vector of FFDC file info
     */
    void createWithFFDCFiles(
        std::string message,
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level severity,
        std::map<std::string, std::string> additionalData,
        std::vector<std::tuple<CreateIface::FFDCFormat, uint8_t, uint8_t,
                               sdbusplus::message::unix_fd>>
            ffdc) override
    {
        manager.createWithFFDC(message, severity, additionalData, ffdc);
    }

  private:
    /** @brief This is a reference to manager object */
    internal::Manager& manager;
};

} // namespace logging
} // namespace phosphor
