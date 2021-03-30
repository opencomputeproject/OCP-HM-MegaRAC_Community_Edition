#include "config.h"

#include "log_manager.hpp"

#include "elog_entry.hpp"
#include "elog_meta.hpp"
#include "elog_serialize.hpp"
#include "extensions.hpp"
#include "util.hpp"

#include <poll.h>
#include <sys/inotify.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/vtable.hpp>
#include <set>
#include <string>
#include <vector>
#include <xyz/openbmc_project/State/Host/server.hpp>

using namespace phosphor::logging;
using namespace std::chrono;
using sdbusplus::exception::SdBusError;
extern const std::map<metadata::Metadata,
                      std::function<metadata::associations::Type>>
    meta;

namespace phosphor
{
namespace logging
{
namespace internal
{
inline auto getLevel(const std::string& errMsg)
{
    auto reqLevel = Entry::Level::Error; // Default to Error

    auto levelmap = g_errLevelMap.find(errMsg);
    if (levelmap != g_errLevelMap.end())
    {
        reqLevel = static_cast<Entry::Level>(levelmap->second);
    }

    return reqLevel;
}

int Manager::getRealErrSize()
{
    return realErrors.size();
}

int Manager::getInfoErrSize()
{
    return infoErrors.size();
}

void Manager::commit(uint64_t transactionId, std::string errMsg)
{
    auto level = getLevel(errMsg);
    _commit(transactionId, std::move(errMsg), level);
}

void Manager::commitWithLvl(uint64_t transactionId, std::string errMsg,
                            uint32_t errLvl)
{
    _commit(transactionId, std::move(errMsg),
            static_cast<Entry::Level>(errLvl));
}

void Manager::_commit(uint64_t transactionId, std::string&& errMsg,
                      Entry::Level errLvl)
{
    constexpr const auto transactionIdVar = "TRANSACTION_ID";
    // Length of 'TRANSACTION_ID' string.
    constexpr const auto transactionIdVarSize = std::strlen(transactionIdVar);
    // Length of 'TRANSACTION_ID=' string.
    constexpr const auto transactionIdVarOffset = transactionIdVarSize + 1;

    // Flush all the pending log messages into the journal
    journalSync();

    sd_journal* j = nullptr;
    int rc = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
    if (rc < 0)
    {
        logging::log<logging::level::ERR>(
            "Failed to open journal",
            logging::entry("DESCRIPTION=%s", strerror(-rc)));
        return;
    }

    std::string transactionIdStr = std::to_string(transactionId);
    std::set<std::string> metalist;
    auto metamap = g_errMetaMap.find(errMsg);
    if (metamap != g_errMetaMap.end())
    {
        metalist.insert(metamap->second.begin(), metamap->second.end());
    }

    // Add _PID field information in AdditionalData.
    metalist.insert("_PID");

    std::vector<std::string> additionalData;

    // Read the journal from the end to get the most recent entry first.
    // The result from the sd_journal_get_data() is of the form VARIABLE=value.
    SD_JOURNAL_FOREACH_BACKWARDS(j)
    {
        const char* data = nullptr;
        size_t length = 0;

        // Look for the transaction id metadata variable
        rc = sd_journal_get_data(j, transactionIdVar, (const void**)&data,
                                 &length);
        if (rc < 0)
        {
            // This journal entry does not have the TRANSACTION_ID
            // metadata variable.
            continue;
        }

        // journald does not guarantee that sd_journal_get_data() returns NULL
        // terminated strings, so need to specify the size to use to compare,
        // use the returned length instead of anything that relies on NULL
        // terminators like strlen().
        // The data variable is in the form of 'TRANSACTION_ID=1234'. Remove
        // the TRANSACTION_ID characters plus the (=) sign to do the comparison.
        // 'data + transactionIdVarOffset' will be in the form of '1234'.
        // 'length - transactionIdVarOffset' will be the length of '1234'.
        if ((length <= (transactionIdVarOffset)) ||
            (transactionIdStr.compare(0, transactionIdStr.size(),
                                      data + transactionIdVarOffset,
                                      length - transactionIdVarOffset) != 0))
        {
            // The value of the TRANSACTION_ID metadata is not the requested
            // transaction id number.
            continue;
        }

        // Search for all metadata variables in the current journal entry.
        for (auto i = metalist.cbegin(); i != metalist.cend();)
        {
            rc = sd_journal_get_data(j, (*i).c_str(), (const void**)&data,
                                     &length);
            if (rc < 0)
            {
                // Metadata variable not found, check next metadata variable.
                i++;
                continue;
            }

            // Metadata variable found, save it and remove it from the set.
            additionalData.emplace_back(data, length);
            i = metalist.erase(i);
        }
        if (metalist.empty())
        {
            // All metadata variables found, break out of journal loop.
            break;
        }
    }
    if (!metalist.empty())
    {
        // Not all the metadata variables were found in the journal.
        for (auto& metaVarStr : metalist)
        {
            logging::log<logging::level::INFO>(
                "Failed to find metadata",
                logging::entry("META_FIELD=%s", metaVarStr.c_str()));
        }
    }

    sd_journal_close(j);

    createEntry(errMsg, errLvl, additionalData);
}

void Manager::createEntry(std::string errMsg, Entry::Level errLvl,
                          std::vector<std::string> additionalData,
                          const FFDCEntries& ffdc)
{
    if (!Extensions::disableDefaultLogCaps())
    {
        if (errLvl < Entry::sevLowerLimit)
        {
            if (realErrors.size() >= ERROR_CAP)
            {
                erase(realErrors.front());
            }
        }
        else
        {
            if (infoErrors.size() >= ERROR_INFO_CAP)
            {
                erase(infoErrors.front());
            }
        }
    }

    entryId++;
    if (errLvl >= Entry::sevLowerLimit)
    {
        infoErrors.push_back(entryId);
    }
    else
    {
        realErrors.push_back(entryId);
    }
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    auto objPath = std::string(OBJ_ENTRY) + '/' + std::to_string(entryId);

    AssociationList objects{};
    processMetadata(errMsg, additionalData, objects);

    auto e = std::make_unique<Entry>(busLog, objPath, entryId,
                                     ms, // Milliseconds since 1970
                                     errLvl, std::move(errMsg),
                                     std::move(additionalData),
                                     std::move(objects), fwVersion, *this);
    serialize(*e);

    if (isQuiesceOnErrorEnabled())
    {
        checkQuiesceOnError(*e);
    }

    doExtensionLogCreate(*e, ffdc);

    // Note: No need to close the file descriptors in the FFDC.

    entries.insert(std::make_pair(entryId, std::move(e)));
}

bool Manager::isQuiesceOnErrorEnabled()
{
    std::variant<bool> property;

    auto method = this->busLog.new_method_call(
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/logging/settings",
        "org.freedesktop.DBus.Properties", "Get");

    method.append("xyz.openbmc_project.Logging.Settings", "QuiesceOnHwError");

    try
    {
        auto reply = this->busLog.call(method);
        reply.read(property);
    }
    catch (const SdBusError& e)
    {
        log<level::ERR>("Error reading QuiesceOnHwError property",
                        entry("ERROR=%s", e.what()));
        throw;
    }

    return std::get<bool>(property);
}

bool Manager::isCalloutPresent(const Entry& entry)
{
    for (const auto& c : entry.additionalData())
    {
        if (c.find("CALLOUT_") != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

void Manager::findAndRemoveResolvedBlocks()
{
    for (auto& entry : entries)
    {
        if (entry.second->resolved())
        {
            checkAndRemoveBlockingError(entry.first);
        }
    }
}

void Manager::onEntryResolve(sdbusplus::message::message& msg)
{
    using Interface = std::string;
    using Property = std::string;
    using Value = std::string;
    using Properties = std::map<Property, std::variant<Value>>;

    Interface interface;
    Properties properties;

    msg.read(interface, properties);

    for (const auto& p : properties)
    {
        if (p.first == "Resolved")
        {
            findAndRemoveResolvedBlocks();
            return;
        }
    }
}

void Manager::checkAndQuiesceHost()
{
    // First check host state
    std::variant<std::string> property;

    auto method = this->busLog.new_method_call(
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "Get");

    method.append("xyz.openbmc_project.State.Host", "CurrentHostState");

    try
    {
        auto reply = this->busLog.call(method);
        reply.read(property);
    }
    catch (const SdBusError& e)
    {
        // Quiescing the host is a "best effort" type function. If unable to
        // read the host state or it comes back empty, just return.
        // The boot block object will still be created and the associations to
        // find the log will be present. Don't want a dependency with
        // phosphor-state-manager service
        log<level::INFO>("Error reading QuiesceOnHwError property",
                         entry("ERROR=%s", e.what()));
        return;
    }

    std::string hostState = std::get<std::string>(property);

    // If host state is empty, do nothing
    if (hostState.empty())
    {
        return;
    }

    using Host = sdbusplus::xyz::openbmc_project::State::server::Host;
    auto state = Host::convertHostStateFromString(hostState);
    if (state != Host::HostState::Running)
    {
        return;
    }

    auto quiesce = this->busLog.new_method_call(
        "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager", "StartUnit");

    quiesce.append("obmc-host-quiesce@0.target");
    quiesce.append("replace");

    this->busLog.call_noreply(quiesce);
}

void Manager::checkQuiesceOnError(const Entry& entry)
{

    if (!isCalloutPresent(entry))
    {
        return;
    }

    logging::log<logging::level::INFO>(
        "QuiesceOnError set and callout present");

    auto blockPath =
        std::string(OBJ_LOGGING) + "/block" + std::to_string(entry.id());
    auto blockObj =
        std::make_unique<Block>(this->busLog, blockPath, entry.id());
    this->blockingErrors.push_back(std::move(blockObj));

    // Register call back if log is resolved
    using namespace sdbusplus::bus::match::rules;
    auto entryPath = std::string(OBJ_ENTRY) + '/' + std::to_string(entry.id());
    auto callback = std::make_unique<sdbusplus::bus::match::match>(
        this->busLog,
        propertiesChanged(entryPath, "xyz.openbmc_project.Logging.Entry"),
        std::bind(std::mem_fn(&Manager::onEntryResolve), this,
                  std::placeholders::_1));

    propChangedEntryCallback.insert(
        std::make_pair(entry.id(), std::move(callback)));

    checkAndQuiesceHost();
}

void Manager::doExtensionLogCreate(const Entry& entry, const FFDCEntries& ffdc)
{
    // Make the association <endpointpath>/<endpointtype> paths
    std::vector<std::string> assocs;
    for (const auto& [forwardType, reverseType, endpoint] :
         entry.associations())
    {
        std::string e{endpoint};
        e += '/' + reverseType;
        assocs.push_back(e);
    }

    for (auto& create : Extensions::getCreateFunctions())
    {
        try
        {
            create(entry.message(), entry.id(), entry.timestamp(),
                   entry.severity(), entry.additionalData(), assocs, ffdc);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("An extension's create function threw an exception",
                            phosphor::logging::entry("ERROR=%s", e.what()));
        }
    }
}

void Manager::processMetadata(const std::string& errorName,
                              const std::vector<std::string>& additionalData,
                              AssociationList& objects) const
{
    // additionalData is a list of "metadata=value"
    constexpr auto separator = '=';
    for (const auto& entryItem : additionalData)
    {
        auto found = entryItem.find(separator);
        if (std::string::npos != found)
        {
            auto metadata = entryItem.substr(0, found);
            auto iter = meta.find(metadata);
            if (meta.end() != iter)
            {
                (iter->second)(metadata, additionalData, objects);
            }
        }
    }
}

void Manager::checkAndRemoveBlockingError(uint32_t entryId)
{
    // First look for blocking object and remove
    auto it = find_if(
        blockingErrors.begin(), blockingErrors.end(),
        [&](std::unique_ptr<Block>& obj) { return obj->entryId == entryId; });
    if (it != blockingErrors.end())
    {
        blockingErrors.erase(it);
    }

    // Now remove the callback looking for the error to be resolved
    auto resolveFind = propChangedEntryCallback.find(entryId);
    if (resolveFind != propChangedEntryCallback.end())
    {
        propChangedEntryCallback.erase(resolveFind);
    }

    return;
}

void Manager::erase(uint32_t entryId)
{
    auto entryFound = entries.find(entryId);
    if (entries.end() != entryFound)
    {
        for (auto& func : Extensions::getDeleteProhibitedFunctions())
        {
            try
            {
                bool prohibited = false;
                func(entryId, prohibited);
                if (prohibited)
                {
                    // Future work remains to throw an error here.
                    return;
                }
            }
            catch (std::exception& e)
            {
                log<level::ERR>(
                    "An extension's deleteProhibited function threw "
                    "an exception",
                    entry("ERROR=%s", e.what()));
            }
        }

        // Delete the persistent representation of this error.
        fs::path errorPath(ERRLOG_PERSIST_PATH);
        errorPath /= std::to_string(entryId);
        fs::remove(errorPath);

        auto removeId = [](std::list<uint32_t>& ids, uint32_t id) {
            auto it = std::find(ids.begin(), ids.end(), id);
            if (it != ids.end())
            {
                ids.erase(it);
            }
        };
        if (entryFound->second->severity() >= Entry::sevLowerLimit)
        {
            removeId(infoErrors, entryId);
        }
        else
        {
            removeId(realErrors, entryId);
        }
        entries.erase(entryFound);

        checkAndRemoveBlockingError(entryId);

        for (auto& remove : Extensions::getDeleteFunctions())
        {
            try
            {
                remove(entryId);
            }
            catch (std::exception& e)
            {
                log<level::ERR>("An extension's delete function threw an "
                                "exception",
                                entry("ERROR=%s", e.what()));
            }
        }
    }
    else
    {
        logging::log<level::ERR>("Invalid entry ID to delete",
                                 logging::entry("ID=%d", entryId));
    }
}

void Manager::restore()
{
    auto sanity = [](const auto& id, const auto& restoredId) {
        return id == restoredId;
    };
    std::vector<uint32_t> errorIds;

    fs::path dir(ERRLOG_PERSIST_PATH);
    if (!fs::exists(dir) || fs::is_empty(dir))
    {
        return;
    }

    for (auto& file : fs::directory_iterator(dir))
    {
        auto id = file.path().filename().c_str();
        auto idNum = std::stol(id);
        auto e = std::make_unique<Entry>(
            busLog, std::string(OBJ_ENTRY) + '/' + id, idNum, *this);
        if (deserialize(file.path(), *e))
        {
            // validate the restored error entry id
            if (sanity(static_cast<uint32_t>(idNum), e->id()))
            {
                e->emit_object_added();
                if (e->severity() >= Entry::sevLowerLimit)
                {
                    infoErrors.push_back(idNum);
                }
                else
                {
                    realErrors.push_back(idNum);
                }

                entries.insert(std::make_pair(idNum, std::move(e)));
                errorIds.push_back(idNum);
            }
            else
            {
                logging::log<logging::level::ERR>(
                    "Failed in sanity check while restoring error entry. "
                    "Ignoring error entry",
                    logging::entry("ID_NUM=%d", idNum),
                    logging::entry("ENTRY_ID=%d", e->id()));
            }
        }
    }

    if (!errorIds.empty())
    {
        entryId = *(std::max_element(errorIds.begin(), errorIds.end()));
    }
}

void Manager::journalSync()
{
    bool syncRequested = false;
    auto fd = -1;
    auto rc = -1;
    auto wd = -1;
    auto bus = sdbusplus::bus::new_default();

    auto start =
        duration_cast<microseconds>(steady_clock::now().time_since_epoch())
            .count();

    // Each time an error log is committed, a request to sync the journal
    // must occur and block that error log commit until it completes. A 5sec
    // block is done to allow sufficient time for the journal to be synced.
    //
    // Number of loop iterations = 3 for the following reasons:
    // Iteration #1: Requests a journal sync by killing the journald service.
    // Iteration #2: Setup an inotify watch to monitor the synced file that
    //               journald updates with the timestamp the last time the
    //               journal was flushed.
    // Iteration #3: Poll to wait until inotify reports an event which blocks
    //               the error log from being commited until the sync completes.
    constexpr auto maxRetry = 3;
    for (int i = 0; i < maxRetry; i++)
    {
        // Read timestamp from synced file
        constexpr auto syncedPath = "/run/systemd/journal/synced";
        std::ifstream syncedFile(syncedPath);
        if (syncedFile.fail())
        {
            // If the synced file doesn't exist, a sync request will create it.
            if (errno != ENOENT)
            {
                log<level::ERR>("Failed to open journal synced file",
                                entry("FILENAME=%s", syncedPath),
                                entry("ERRNO=%d", errno));
                return;
            }
        }
        else
        {
            // Only read the synced file if it exists.
            // See if a sync happened by now
            std::string timestampStr;
            std::getline(syncedFile, timestampStr);
            auto timestamp = std::stoll(timestampStr);
            if (timestamp >= start)
            {
                break;
            }
        }

        // Let's ask for a sync, but only once
        if (!syncRequested)
        {
            syncRequested = true;

            constexpr auto JOURNAL_UNIT = "systemd-journald.service";
            auto signal = SIGRTMIN + 1;

            auto method = bus.new_method_call(SYSTEMD_BUSNAME, SYSTEMD_PATH,
                                              SYSTEMD_INTERFACE, "KillUnit");
            method.append(JOURNAL_UNIT, "main", signal);
            bus.call(method);
            if (method.is_method_error())
            {
                log<level::ERR>("Failed to kill journal service");
                break;
            }
            continue;
        }

        // Let's install the inotify watch, if we didn't do that yet. This watch
        // monitors the syncedFile for when journald updates it with a newer
        // timestamp. This means the journal has been flushed.
        if (fd < 0)
        {
            fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
            if (fd < 0)
            {
                log<level::ERR>("Failed to create inotify watch",
                                entry("ERRNO=%d", errno));
                return;
            }

            constexpr auto JOURNAL_RUN_PATH = "/run/systemd/journal";
            wd = inotify_add_watch(fd, JOURNAL_RUN_PATH,
                                   IN_MOVED_TO | IN_DONT_FOLLOW | IN_ONLYDIR);
            if (wd < 0)
            {
                log<level::ERR>("Failed to watch journal directory",
                                entry("PATH=%s", JOURNAL_RUN_PATH),
                                entry("ERRNO=%d", errno));
                close(fd);
                return;
            }
            continue;
        }

        // Let's wait until inotify reports an event
        struct pollfd fds = {
            .fd = fd,
            .events = POLLIN,
        };
        constexpr auto pollTimeout = 5; // 5 seconds
        rc = poll(&fds, 1, pollTimeout * 1000);
        if (rc < 0)
        {
            log<level::ERR>("Failed to add event", entry("ERRNO=%d", errno),
                            entry("ERR=%s", strerror(-rc)));
            inotify_rm_watch(fd, wd);
            close(fd);
            return;
        }
        else if (rc == 0)
        {
            log<level::INFO>("Poll timeout, no new journal synced data",
                             entry("TIMEOUT=%d", pollTimeout));
            break;
        }

        // Read from the specified file descriptor until there is no new data,
        // throwing away everything read since the timestamp will be read at the
        // beginning of the loop.
        constexpr auto maxBytes = 64;
        uint8_t buffer[maxBytes];
        while (read(fd, buffer, maxBytes) > 0)
            ;
    }

    if (fd != -1)
    {
        if (wd != -1)
        {
            inotify_rm_watch(fd, wd);
        }
        close(fd);
    }

    return;
}

std::string Manager::readFWVersion()
{
    auto version = util::getOSReleaseValue("VERSION_ID");

    if (!version)
    {
        log<level::ERR>("Unable to read BMC firmware version");
    }

    return version.value_or("");
}

void Manager::create(const std::string& message, Entry::Level severity,
                     const std::map<std::string, std::string>& additionalData)
{
    // Convert the map into a vector of "key=value" strings
    std::vector<std::string> ad;
    metadata::associations::combine(additionalData, ad);

    createEntry(message, severity, ad);
}

void Manager::createWithFFDC(
    const std::string& message, Entry::Level severity,
    const std::map<std::string, std::string>& additionalData,
    const FFDCEntries& ffdc)
{
    // Convert the map into a vector of "key=value" strings
    std::vector<std::string> ad;
    metadata::associations::combine(additionalData, ad);

    createEntry(message, severity, ad, ffdc);
}

} // namespace internal
} // namespace logging
} // namespace phosphor
