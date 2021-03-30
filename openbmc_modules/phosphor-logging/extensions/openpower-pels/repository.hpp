#pragma once
#include "bcd_time.hpp"
#include "paths.hpp"
#include "pel.hpp"

#include <algorithm>
#include <bitset>
#include <filesystem>
#include <map>

namespace openpower
{
namespace pels
{

/**
 * @class Repository
 *
 * The class handles saving and retrieving PELs on the BMC.
 */
class Repository
{
  public:
    /**
     * @brief Structure of commonly used PEL attributes.
     */
    struct PELAttributes
    {
        std::filesystem::path path;
        size_t sizeOnDisk;
        uint8_t creator;
        uint8_t severity;
        std::bitset<16> actionFlags;
        TransmissionState hostState;
        TransmissionState hmcState;

        PELAttributes() = delete;

        PELAttributes(const std::filesystem::path& p, size_t size,
                      uint8_t creator, uint8_t sev, uint16_t flags,
                      TransmissionState hostState, TransmissionState hmcState) :
            path(p),
            sizeOnDisk(size), creator(creator), severity(sev),
            actionFlags(flags), hostState(hostState), hmcState(hmcState)
        {
        }
    };

    /**
     * @brief A structure that holds both the PEL and corresponding
     *        OpenBMC IDs.
     * Used for correlating the IDs with their data files for quick
     * lookup.  To find a PEL based on just one of the IDs, just use
     * the constructor that takes that ID.
     */
    struct LogID
    {
        struct Pel
        {
            uint32_t id;
            explicit Pel(uint32_t i) : id(i)
            {
            }
        };
        struct Obmc
        {
            uint32_t id;
            explicit Obmc(uint32_t i) : id(i)
            {
            }
        };

        Pel pelID;

        Obmc obmcID;

        LogID(Pel pel, Obmc obmc) : pelID(pel), obmcID(obmc)
        {
        }

        explicit LogID(Pel id) : pelID(id), obmcID(0)
        {
        }

        explicit LogID(Obmc id) : pelID(0), obmcID(id)
        {
        }

        LogID() = delete;

        /**
         * @brief A == operator that will match on either ID
         *        being equal if the other is zero, so that
         *        one can look up a PEL with just one of the IDs.
         */
        bool operator==(const LogID& id) const
        {
            if (id.pelID.id != 0)
            {
                return id.pelID.id == pelID.id;
            }
            if (id.obmcID.id != 0)
            {
                return id.obmcID.id == obmcID.id;
            }
            return false;
        }

        bool operator<(const LogID& id) const
        {
            return pelID.id < id.pelID.id;
        }
    };

    using AttributesReference =
        std::reference_wrapper<const std::pair<const LogID, PELAttributes>>;

    /**
     * @brief A structure for keeping a breakdown of the sizes of PELs
     *        of different types in the repository.
     */
    struct SizeStats
    {
        uint64_t total;
        uint64_t bmc;
        uint64_t nonBMC;
        uint64_t bmcServiceable;
        uint64_t bmcInfo;
        uint64_t nonBMCServiceable;
        uint64_t nonBMCInfo;

        SizeStats() :
            total(0), bmc(0), nonBMC(0), bmcServiceable(0), bmcInfo(0),
            nonBMCServiceable(0), nonBMCInfo(0)
        {
        }
    };

    Repository() = delete;
    ~Repository() = default;
    Repository(const Repository&) = default;
    Repository& operator=(const Repository&) = default;
    Repository(Repository&&) = default;
    Repository& operator=(Repository&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] basePath - the base filesystem path for the repository
     */
    Repository(const std::filesystem::path& basePath) :
        Repository(basePath, getPELRepoSize(), getMaxNumPELs())
    {
    }

    /**
     * @brief Constructor that takes the repository size
     *
     * @param[in] basePath - the base filesystem path for the repository
     * @param[in] repoSize - The maximum amount of space to use for PELs,
     *                       in bytes
     * @param[in] maxNumPELs - The maximum number of PELs to allow
     */
    Repository(const std::filesystem::path& basePath, size_t repoSize,
               size_t maxNumPELs);

    /**
     * @brief Adds a PEL to the repository
     *
     * Throws File.Error.Open or File.Error.Write exceptions on failure
     *
     * @param[in] pel - the PEL to add
     */
    void add(std::unique_ptr<PEL>& pel);

    /**
     * @brief Removes a PEL from the repository
     *
     * Note that the returned LogID is the fully filled in LogID, i.e.
     * it has both the PEL and OpenBMC IDs, unlike the passed in LogID
     * which can just have one or the other.
     *
     * @param[in] id - the ID (either the pel ID, OBMC ID, or both) to remove
     *
     * @return std::optional<LogID> - The LogID of the removed PEL
     */
    std::optional<LogID> remove(const LogID& id);

    /**
     * @brief Generates the filename to use for the PEL ID and BCDTime.
     *
     * @param[in] pelID - the PEL ID
     * @param[in] time - the BCD time
     *
     * @return string - A filename string of <BCD_time>_<pelID>
     */
    static std::string getPELFilename(uint32_t pelID, const BCDTime& time);

    /**
     * @brief Returns true if the PEL with the specified ID is in the repo.
     *
     * @param[in] id - the ID (either the pel ID, OBMC ID, or both)
     * @return bool - true if that PEL is present
     */
    inline bool hasPEL(const LogID& id)
    {
        return findPEL(id) != _pelAttributes.end();
    }

    /**
     * @brief Returns the PEL data based on its ID.
     *
     * If the data can't be found for that ID, then the optional object
     * will be empty.
     *
     * @param[in] id - the LogID to get the PEL for, which can be either a
     *                 PEL ID or OpenBMC log ID.
     * @return std::optional<std::vector<uint8_t>> - the PEL data
     */
    std::optional<std::vector<uint8_t>> getPELData(const LogID& id);

    /**
     * @brief Get a file descriptor to the PEL data
     *
     * @param[in] id - The ID to get the FD for
     *
     * @return std::optional<sdbusplus::message::unix_fd> -
     *         The FD, or an empty optional object.
     */
    std::optional<sdbusplus::message::unix_fd> getPELFD(const LogID& id);

    using ForEachFunc = std::function<bool(const PEL&)>;

    /**
     * @brief Run a user defined function on every PEL in the repository.
     *
     * ForEachFunc takes a const PEL reference, and should return
     * true to stop iterating and return out of for_each.
     *
     * For example, to save up to 100 IDs in the repo into a vector:
     *
     *     std::vector<uint32_t> ids;
     *     ForEachFunc f = [&ids](const PEL& pel) {
     *         ids.push_back(pel.id());
     *         return ids.size() == 100 ? true : false;
     *     };
     *
     * @param[in] func - The function to run.
     */
    void for_each(ForEachFunc func) const;

    using AddCallback = std::function<void(const PEL&)>;

    /**
     * @brief Subscribe to PELs being added to the repository.
     *
     * Every time a PEL is added to the repository, the provided
     * function will be called with the new PEL as the argument.
     *
     * The function must be of type void(const PEL&).
     *
     * @param[in] name - The subscription name
     * @param[in] func - The callback function
     */
    void subscribeToAdds(const std::string& name, AddCallback func)
    {
        if (_addSubscriptions.find(name) == _addSubscriptions.end())
        {
            _addSubscriptions.emplace(name, func);
        }
    }

    /**
     * @brief Unsubscribe from new PELs.
     *
     * @param[in] name - The subscription name
     */
    void unsubscribeFromAdds(const std::string& name)
    {
        _addSubscriptions.erase(name);
    }

    using DeleteCallback = std::function<void(uint32_t)>;

    /**
     * @brief Subscribe to PELs being deleted from the repository.
     *
     * Every time a PEL is deleted from the repository, the provided
     * function will be called with the PEL ID as the argument.
     *
     * The function must be of type void(const uint32_t).
     *
     * @param[in] name - The subscription name
     * @param[in] func - The callback function
     */
    void subscribeToDeletes(const std::string& name, DeleteCallback func)
    {
        if (_deleteSubscriptions.find(name) == _deleteSubscriptions.end())
        {
            _deleteSubscriptions.emplace(name, func);
        }
    }

    /**
     * @brief Unsubscribe from deleted PELs.
     *
     * @param[in] name - The subscription name
     */
    void unsubscribeFromDeletes(const std::string& name)
    {
        _deleteSubscriptions.erase(name);
    }

    /**
     * @brief Get the PEL attributes for a PEL
     *
     * @param[in] id - The ID to find the attributes for
     *
     * @return The attributes or an empty optional if not found
     */
    std::optional<std::reference_wrapper<const PELAttributes>>
        getPELAttributes(const LogID& id) const;

    /**
     * @brief Sets the host transmission state on a PEL file
     *
     * Writes the host transmission state field in the User Header
     * section in the PEL data specified by the ID.
     *
     * @param[in] pelID - The PEL ID
     * @param[in] state - The state to write
     */
    void setPELHostTransState(uint32_t pelID, TransmissionState state);

    /**
     * @brief Sets the HMC transmission state on a PEL file
     *
     * Writes the HMC transmission state field in the User Header
     * section in the PEL data specified by the ID.
     *
     * @param[in] pelID - The PEL ID
     * @param[in] state - The state to write
     */
    void setPELHMCTransState(uint32_t pelID, TransmissionState state);

    /**
     * @brief Returns the size stats structure
     *
     * @return const SizeStats& - The stats structure
     */
    const SizeStats& getSizeStats() const
    {
        return _sizes;
    }

    /**
     * @brief Says if the PEL is considered serviceable (not just
     *        informational) as determined by its severity.
     *
     * @param[in] pel - The PELAttributes entry for the PEL
     * @return bool - If serviceable or not
     */
    static bool isServiceableSev(const PELAttributes& pel);

    /**
     * @brief Returns true if the total amount of disk space occupied
     *        by the PELs in the repo is over 95% of the maximum
     *        size, or if there are over the maximum number of
     *        PELs allowed.
     *
     * @return bool - true if repo is > 95% full or too many PELs
     */
    bool sizeWarning() const;

    /**
     * @brief Deletes PELs to bring the repository size down
     *        to at most 90% full by placing PELs into 4 different
     *        catogories and then removing PELs until those catogories
     *        only take up certain percentages of the allowed space.
     *
     * This does not delete the corresponding OpenBMC event logs, which
     * is why those IDs are returned, so they can be deleted later.
     *
     * The categories and their rules are:
     *  1) Informational BMC PELs cannot take up more than 15% of
     *     the allocated space.
     *  2) Non-informational BMC PELs cannot take up more than 30%
     *     of the allocated space.
     *  3) Informational non-BMC PELs cannot take up more than 15% of
     *     the allocated space.
     *  4) Non-informational non-BMC PELs cannot take up more than 30%
     *     of the allocated space.
     *
     *  While removing PELs in a category, 4 passes will be made, with
     *  PELs being removed oldest first during each pass.
     *
     *   Pass 1: only delete HMC acked PELs
     *   Pass 2: only delete OS acked PELs
     *   Pass 3: only delete PHYP sent PELs
     *   Pass 4: delete all PELs
     *
     * @return std::vector<uint32_t> - The OpenBMC event log IDs of
     *                                 the PELs that were deleted.
     */
    std::vector<uint32_t> prune();

    /**
     * @brief Returns the path to the directory where the PEL
     *        files are stored.
     *
     * @return std::filesystem::path - The directory path
     */
    const std::filesystem::path& repoPath() const
    {
        return _logPath;
    }

  private:
    using PELUpdateFunc = std::function<void(PEL&)>;

    /**
     * @brief Lets a function modify a PEL and saves the results
     *
     * Runs updateFunc (a void(PEL&) function) on the PEL data
     * on the file specified, and writes the results back to the file.
     *
     * @param[in] path - The file path to use
     * @param[in] updateFunc - The function to run to update the PEL.
     */
    void updatePEL(const std::filesystem::path& path, PELUpdateFunc updateFunc);

    /**
     * @brief Finds an entry in the _pelAttributes map.
     *
     * @param[in] id - the ID (either the pel ID, OBMC ID, or both)
     *
     * @return an iterator to the entry
     */
    std::map<LogID, PELAttributes>::const_iterator
        findPEL(const LogID& id) const
    {
        return std::find_if(_pelAttributes.begin(), _pelAttributes.end(),
                            [&id](const auto& a) { return a.first == id; });
    }

    /**
     * @brief Call any subscribed functions for new PELs
     *
     * @param[in] pel - The new PEL
     */
    void processAddCallbacks(const PEL& pel) const;

    /**
     * @brief Call any subscribed functions for deleted PELs
     *
     * @param[in] id - The ID of the deleted PEL
     */
    void processDeleteCallbacks(uint32_t id) const;

    /**
     * @brief Restores the _pelAttributes map on startup based on the existing
     *        PEL data files.
     */
    void restore();

    /**
     * @brief Stores a PEL object in the filesystem.
     *
     * @param[in] pel - The PEL to write
     * @param[in] path - The file to write to
     *
     * Throws exceptions on failures.
     */
    void write(const PEL& pel, const std::filesystem::path& path);

    /**
     * @brief Updates the repository statistics after a PEL is
     *        added or removed.
     *
     * @param[in] pel - The PELAttributes entry for the PEL
     * @param[in] pelAdded - true if the PEL was added, false if removed
     */
    void updateRepoStats(const PELAttributes& pel, bool pelAdded);

    enum class SortOrder
    {
        ascending,
        descending
    };

    /**
     * @brief Returns a vector of all the _pelAttributes entries sorted
     *        as specified
     *
     * @param[in] order - If the PELs should be returned in ascending
     *                    (oldest first) or descending order.
     *
     * @return std::vector<AttributesReference> - The sorted vector of
     *         references to the pair<LogID, PELAttributes> entries of
     *         _pelAttributes.
     */
    std::vector<AttributesReference> getAllPELAttributes(SortOrder order) const;

    using IsOverLimitFunc = std::function<bool()>;
    using IsPELTypeFunc = std::function<bool(const PELAttributes&)>;

    /**
     * @brief Makes 4 passes on the PELs that meet the IsPELTypeFunc
     *        criteria removing PELs until IsOverLimitFunc returns false.
     *
     *   Pass 1: only delete HMC acked PELs
     *   Pass 2: only delete Os acked PELs
     *   Pass 3: only delete PHYP sent PELs
     *   Pass 4: delete all PELs
     *
     * @param[in] isOverLimit - The bool(void) function that should
     *                          return true if PELs still need to be
     *                           removed.
     * @param[in] isPELType - The bool(const PELAttributes&) function
     *                         used to select the PELs to operate on.
     *
     * @param[out] removedBMCLogIDs - The OpenBMC event log IDs of the
     *                                removed PELs.
     */
    void removePELs(IsOverLimitFunc& isOverLimit, IsPELTypeFunc& isPELType,
                    std::vector<uint32_t>& removedBMCLogIDs);
    /**
     * @brief The filesystem path to the PEL logs.
     */
    const std::filesystem::path _logPath;

    /**
     * @brief A map of the PEL/OBMC IDs to PEL attributes.
     */
    std::map<LogID, PELAttributes> _pelAttributes;

    /**
     * @brief Subcriptions for new PELs.
     */
    std::map<std::string, AddCallback> _addSubscriptions;

    /**
     * @brief Subscriptions for deleted PELs.
     */
    std::map<std::string, DeleteCallback> _deleteSubscriptions;

    /**
     * @brief The maximum amount of space that the PELs in the
     *        repository can occupy.
     */
    const uint64_t _maxRepoSize;

    /**
     * @brief The maximum number of PELs to allow in the repo
     *        before pruning.
     */
    const size_t _maxNumPELs;

    /**
     * @brief Statistics on the sizes of the stored PELs.
     */
    SizeStats _sizes;
};

} // namespace pels
} // namespace openpower
