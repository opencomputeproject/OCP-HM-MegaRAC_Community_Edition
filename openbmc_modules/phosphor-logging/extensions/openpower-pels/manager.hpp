#pragma once

#include "config.h"

#include "data_interface.hpp"
#include "event_logger.hpp"
#include "host_notifier.hpp"
#include "log_manager.hpp"
#include "paths.hpp"
#include "pel.hpp"
#include "registry.hpp"
#include "repository.hpp"

#include <org/open_power/Logging/PEL/server.hpp>
#include <sdbusplus/server.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>

namespace openpower
{
namespace pels
{

using PELInterface = sdbusplus::server::object::object<
    sdbusplus::org::open_power::Logging::server::PEL>;

/**
 * @brief PEL manager object
 */
class Manager : public PELInterface
{
  public:
    Manager() = delete;
    Manager(const Manager&) = default;
    Manager& operator=(const Manager&) = default;
    Manager(Manager&&) = default;
    Manager& operator=(Manager&&) = default;

    /**
     * @brief constructor
     *
     * @param[in] logManager - internal::Manager object
     * @param[in] dataIface - The data interface object
     * @param[in] creatorFunc - The function that EventLogger will
     *                          use for creating event logs
     */
    Manager(phosphor::logging::internal::Manager& logManager,
            std::unique_ptr<DataInterfaceBase> dataIface,
            EventLogger::LogFunction creatorFunc) :
        PELInterface(logManager.getBus(), OBJ_LOGGING),
        _logManager(logManager), _eventLogger(std::move(creatorFunc)),
        _repo(getPELRepoPath()),
        _registry(getPELReadOnlyDataPath() / message::registryFileName),
        _event(sdeventplus::Event::get_default()),
        _dataIface(std::move(dataIface))
    {
        setupPELDeleteWatch();
    }

    /**
     * @brief constructor that enables host notification
     *
     * @param[in] logManager - internal::Manager object
     * @param[in] dataIface - The data interface object
     * @param[in] creatorFunc - The function that EventLogger will
     *                          use for creating event logs
     * @param[in] hostIface - The hostInterface object
     */
    Manager(phosphor::logging::internal::Manager& logManager,
            std::unique_ptr<DataInterfaceBase> dataIface,
            EventLogger::LogFunction creatorFunc,
            std::unique_ptr<HostInterface> hostIface) :
        Manager(logManager, std::move(dataIface), std::move(creatorFunc))
    {
        _hostNotifier = std::make_unique<HostNotifier>(
            _repo, *(_dataIface.get()), std::move(hostIface));
    }

    /**
     * @brief Destructor
     */
    ~Manager();

    /**
     * @brief Creates a PEL based on the OpenBMC event log contents.  If
     *        a PEL was passed in via the RAWPEL specifier in the
     *        additionalData parameter, use that instead.
     *
     * @param[in] message - the event log message property
     * @param[in] obmcLogID - the corresponding OpenBMC event log id
     * @param[in] timestamp - the Timestamp property
     * @param[in] severity - the event log severity
     * @param[in] additionalData - the AdditionalData property
     * @param[in] associations - the Associations property
     * @param[in] ffdc - A vector of FFDC file information
     */
    void create(const std::string& message, uint32_t obmcLogID,
                uint64_t timestamp, phosphor::logging::Entry::Level severity,
                const std::vector<std::string>& additionalData,
                const std::vector<std::string>& associations,
                const phosphor::logging::FFDCEntries& ffdc =
                    phosphor::logging::FFDCEntries{});

    /**
     * @brief Erase a PEL based on its OpenBMC event log ID
     *
     * @param[in] obmcLogID - the corresponding OpenBMC event log id
     */
    void erase(uint32_t obmcLogID);

    /** @brief Says if an OpenBMC event log may not be manually deleted at this
     *         time because its corresponding PEL cannot be.
     *
     * There are PEL retention policies that can prohibit the manual deletion
     * of PELs (and therefore OpenBMC event logs).
     *
     * @param[in] obmcLogID - the OpenBMC event log ID
     * @return bool - true if prohibited
     */
    bool isDeleteProhibited(uint32_t obmcLogID);

    /**
     * @brief Return a file descriptor to the raw PEL data
     *
     * Throws InvalidArgument if the PEL ID isn't found,
     * and InternalFailure if anything else fails.
     *
     * @param[in] pelID - The PEL ID to get the data for
     *
     * @return unix_fd - File descriptor to the file that contains the PEL
     */
    sdbusplus::message::unix_fd getPEL(uint32_t pelID) override;

    /**
     * @brief Returns data for the PEL corresponding to an OpenBMC
     *        event log.
     *
     * @param[in] obmcLogID - The OpenBMC event log ID
     *
     * @return vector<uint8_t> - The raw PEL data
     */
    std::vector<uint8_t> getPELFromOBMCID(uint32_t obmcLogID) override;

    /**
     * @brief The D-Bus method called when a host successfully processes
     *        a PEL.
     *
     * This D-Bus method is called from the PLDM daemon when they get an
     * 'Ack PEL' PLDM message from the host, which indicates the host
     * firmware successfully sent it to the OS and this code doesn't need
     * to send it to the host again.
     *
     * @param[in] pelID - The PEL ID
     */
    void hostAck(uint32_t pelID) override;

    /**
     * @brief D-Bus method called when the host rejects a PEL.
     *
     * This D-Bus method is called from the PLDM daemon when they get an
     * 'Ack PEL' PLDM message from the host with a payload that says
     * something when wrong.
     *
     * The choices are either:
     *  * Host Full - The host's staging area is full - try again later
     *  * Malrformed PEL - The host received an invalid PEL
     *
     * @param[in] pelID - The PEL ID
     * @param[in] reason - One of the above two reasons
     */
    void hostReject(uint32_t pelID, RejectionReason reason) override;

    /**
     * @brief Converts the ESEL field in an OpenBMC event log to a
     *        vector of uint8_ts that just contains the PEL data.
     *
     * That data string looks like: "50 48 00 ab ..."
     *
     * Throws an exception on any failures.
     *
     * @param[in] esel - The ESEL string
     *
     * @return std::vector<uint8_t> - The contained PEL data
     */
    static std::vector<uint8_t> eselToRawData(const std::string& esel);

  private:
    /**
     * @brief Adds a received raw PEL to the PEL repository
     *
     * @param[in] rawPelPath - The path to the file that contains the
     *                         raw PEL.
     * @param[in] obmcLogID - the corresponding OpenBMC event log id
     */
    void addRawPEL(const std::string& rawPelPath, uint32_t obmcLogID);

    /**
     * @brief Creates a PEL based on the OpenBMC event log contents.
     *
     * @param[in] message - The event log message property
     * @param[in] obmcLogID - the corresponding OpenBMC event log id
     * @param[in] timestamp - The timestamp property
     * @param[in] severity - The event log severity
     * @param[in] additionalData - The AdditionalData property
     * @param[in] associations - The associations property
     * @param[in] ffdc - A vector of FFDC file information
     */
    void createPEL(const std::string& message, uint32_t obmcLogID,
                   uint64_t timestamp, phosphor::logging::Entry::Level severity,
                   const std::vector<std::string>& additionalData,
                   const std::vector<std::string>& associations,
                   const phosphor::logging::FFDCEntries& ffdc);

    /**
     * @brief Schedules a close of the file descriptor to occur from
     *        the event loop.
     *
     * Uses sd_event_add_defer
     *
     * @param[in] fd - The file descriptor to close
     */
    void scheduleFDClose(int fd);

    /**
     * @brief Closes the file descriptor passed in.
     *
     * This is called from the event loop to close FDs returned
     * from getPEL().
     *
     * @param[in] fd - The file descriptor to close
     * @param[in] source - The event source object used
     */
    void closeFD(int fd, sdeventplus::source::EventBase& source);

    /**
     * @brief Adds a PEL to the repository given its data
     *
     * @param[in] pelData - The PEL to add as a vector of uint8_ts
     * @param[in] obmcLogID - the OpenBMC event log ID
     */
    void addPEL(std::vector<uint8_t>& pelData, uint32_t obmcLogID);

    /**
     * @brief Adds the PEL stored in the ESEL field of the AdditionalData
     *        property of an OpenBMC event log to the repository.
     *
     * @param[in] esel - The ESEL AdditionalData contents
     * @param[in] obmcLogID - The OpenBMC event log ID
     */
    void addESELPEL(const std::string& esel, uint32_t obmcLogID);

    /**
     * @brief Converts the D-Bus FFDC method argument into a data
     *        structure understood by the PEL code.
     *
     * @param[in] ffdc - A vector of FFDC file information
     *
     * @return PelFFDC - The PEL FFDC data structure
     */
    PelFFDC convertToPelFFDC(const phosphor::logging::FFDCEntries& ffdc);

    /**
     * @brief Schedules a PEL repository prune to occur from
     *        the event loop.
     *
     * Uses sd_event_add_defer
     */
    void scheduleRepoPrune();

    /**
     * @brief Prunes old PELs out of the repository to save space.
     *
     * This is called from the event loop.
     *
     * @param[in] source - The event source object used
     */
    void pruneRepo(sdeventplus::source::EventBase& source);

    /**
     * @brief Sets up an inotify watch to watch for deleted PEL
     *        files.  Calls pelFileDeleted() when that occurs.
     */
    void setupPELDeleteWatch();

    /**
     * @brief Called when the inotify watch put on the repository directory
     *        detects a PEL file was deleted.
     *
     * Will tell the Repository class about the deleted PEL, and then tell
     * the log manager class to delete the corresponding OpenBMC event log.
     */
    void pelFileDeleted(sdeventplus::source::IO& io, int fd, uint32_t revents);

    /**
     * @brief Reference to phosphor-logging's Manager class
     */
    phosphor::logging::internal::Manager& _logManager;

    /**
     * @brief Handles creating event logs/PELs from within
     *        the PEL extension code
     */
    EventLogger _eventLogger;

    /**
     * @brief The PEL repository object
     */
    Repository _repo;

    /**
     * @brief The PEL message registry object
     */
    message::Registry _registry;

    /**
     * @brief The Event object this class uses
     */
    sdeventplus::Event _event;

    /**
     * @brief The API the PEL sections use to gather data
     */
    std::unique_ptr<DataInterfaceBase> _dataIface;

    /**
     * @brief The HostNotifier object used for telling the
     *        host about new PELs
     */
    std::unique_ptr<HostNotifier> _hostNotifier;

    /**
     * @brief The event source for closing a PEL file descriptor after
     *        it has been returned from the getPEL D-Bus method.
     */
    std::unique_ptr<sdeventplus::source::Defer> _fdCloserEventSource;

    /**
     * @brief The even source for removing old PELs when the repo is
     *        running out of space to make room for new ones.
     */
    std::unique_ptr<sdeventplus::source::Defer> _repoPrunerEventSource;

    /**
     * @brief The even source for watching for deleted PEL files.
     */
    std::unique_ptr<sdeventplus::source::IO> _pelFileDeleteEventSource;

    /**
     * @brief The file descriptor returned by inotify_init1() used
     *        for watching for deleted PEL files.
     */
    int _pelFileDeleteFD = -1;

    /**
     * @brief The file descriptor returned by inotify_add_watch().
     */
    int _pelFileDeleteWatchFD = -1;
};

} // namespace pels
} // namespace openpower
