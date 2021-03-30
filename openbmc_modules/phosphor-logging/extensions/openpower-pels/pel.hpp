#pragma once

#include "additional_data.hpp"
#include "data_interface.hpp"
#include "private_header.hpp"
#include "registry.hpp"
#include "src.hpp"
#include "user_data.hpp"
#include "user_data_formats.hpp"
#include "user_header.hpp"

#include <memory>
#include <vector>

namespace openpower
{
namespace pels
{

/**
 * @brief Contains information about an FFDC file.
 */
struct PelFFDCfile
{
    UserDataFormat format;
    uint8_t subType;
    uint8_t version;
    int fd;
};

using PelFFDC = std::vector<PelFFDCfile>;

/** @class PEL
 *
 * @brief This class represents a specific event log format referred to as a
 * Platform Event Log.
 *
 * Every field in a PEL are in structures call sections, of which there are
 * several types.  Some sections are required, and some are optional.  In some
 * cases there may be more than one instance of a section type.
 *
 * The only two required sections for every type of PEL are the Private Header
 * section and User Header section, which must be in the first and second
 * positions, respectively.
 *
 * Every section starts with an 8 byte section header, which has the section
 * size and type, among other things.
 *
 * This class represents all sections with objects.
 *
 * The class can be constructed:
 * - From a full formed flattened PEL.
 * - From scratch based on an OpenBMC event and its corresponding PEL message
 *   registry entry.
 *
 * The data() method allows one to retrieve the PEL as a vector<uint8_t>.  This
 * is the format in which it is stored and transmitted.
 */
class PEL
{
  public:
    PEL() = delete;
    ~PEL() = default;
    PEL(const PEL&) = delete;
    PEL& operator=(const PEL&) = delete;
    PEL(PEL&&) = delete;
    PEL& operator=(PEL&&) = delete;

    /**
     * @brief Constructor
     *
     * Build a PEL from raw data.
     *
     * Note: Neither this nor the following constructor can take a const vector&
     * because the Stream class that is used to read from the vector cannot take
     * a const.  The alternative is to make a copy of the data, but as PELs can
     * be up to 16KB that is undesireable.
     *
     * @param[in] data - The PEL data
     */
    PEL(std::vector<uint8_t>& data);

    /**
     * @brief Constructor
     *
     * Build a PEL from the raw data.
     *
     * @param[in] data - the PEL data
     * @param[in] obmcLogID - the corresponding OpenBMC event log ID
     */
    PEL(std::vector<uint8_t>& data, uint32_t obmcLogID);

    /**
     * @brief Constructor
     *
     * Creates a PEL from an OpenBMC event log and its message
     * registry entry.
     *
     * @param[in] entry - The message registry entry for this error
     * @param[in] obmcLogID - ID of corresponding OpenBMC event log
     * @param[in] timestamp - Timestamp from the event log
     * @param[in] severity - Severity from the event log
     * @param[in] additionalData - The AdditionalData contents
     * @param[in] ffdcFiles - FFCD files that go into UserData sections
     * @param[in] dataIface - The data interface object
     */
    PEL(const openpower::pels::message::Entry& entry, uint32_t obmcLogID,
        uint64_t timestamp, phosphor::logging::Entry::Level severity,
        const AdditionalData& additionalData, const PelFFDC& ffdcFiles,
        const DataInterfaceBase& dataIface);

    /**
     * @brief Convenience function to return the log ID field from the
     *        Private Header section.
     *
     * @return uint32_t - the ID
     */
    uint32_t id() const
    {
        return _ph->id();
    }

    /**
     * @brief Convenience function to return the PLID field from the
     *        Private Header section.
     *
     * @return uint32_t - the PLID
     */
    uint32_t plid() const
    {
        return _ph->plid();
    }

    /**
     * @brief Convenience function to return the OpenBMC event log ID field
     * from the Private Header section.
     *
     * @return uint32_t - the OpenBMC event log ID
     */
    uint32_t obmcLogID() const
    {
        return _ph->obmcLogID();
    }

    /**
     * @brief Convenience function to return the commit time field from
     *        the Private Header section.
     *
     * @return BCDTime - the timestamp
     */
    BCDTime commitTime() const
    {
        return _ph->commitTimestamp();
    }

    /**
     * @brief Convenience function to return the create time field from
     *        the Private Header section.
     *
     * @return BCDTime - the timestamp
     */
    BCDTime createTime() const
    {
        return _ph->createTimestamp();
    }

    /**
     * @brief Gives access to the Private Header section class
     *
     * @return const PrivateHeader& - the private header
     */
    const PrivateHeader& privateHeader() const
    {
        return *_ph;
    }

    /**
     * @brief Gives access to the User Header section class
     *
     * @return const UserHeader& - the user header
     */
    const UserHeader& userHeader() const
    {
        return *_uh;
    }

    /**
     * @brief Gives access to the primary SRC's section class
     *
     * This is technically an optional section, so the return
     * value is an std::optional<SRC*>.
     *
     * @return std::optional<SRC*> - the SRC section object
     */
    std::optional<SRC*> primarySRC() const;

    /**
     * @brief Returns the optional sections, which is everything but
     *        the Private and User Headers.
     *
     * @return const std::vector<std::unique_ptr<Section>>&
     */
    const std::vector<std::unique_ptr<Section>>& optionalSections() const
    {
        return _optionalSections;
    }

    /**
     * @brief Returns the PEL data.
     *
     * @return std::vector<uint8_t> - the raw PEL data
     */
    std::vector<uint8_t> data() const;

    /**
     * @brief Returns the size of the PEL
     *
     * @return size_t The PEL size in bytes
     */
    size_t size() const;

    /**
     * @brief Says if the PEL is valid (the sections are all valid)
     *
     * @return bool - if the PEL is valid
     */
    bool valid() const;

    /**
     * @brief Sets the commit timestamp to the current time
     */
    void setCommitTime();

    /**
     * @brief Sets the error log ID field to a unique ID.
     */
    void assignID();

    /**
     * @brief Output a PEL in JSON.
     * @param[in] registry - Registry object reference
     */
    void toJSON(message::Registry& registry) const;

    /**
     * @brief Sets the host transmission state in the User Header
     *
     * @param[in] state - The state value
     */
    void setHostTransmissionState(TransmissionState state)
    {
        _uh->setHostTransmissionState(static_cast<uint8_t>(state));
    }

    /**
     * @brief Returns the host transmission state
     *
     * @return HostTransmissionState - The state
     */
    TransmissionState hostTransmissionState() const
    {
        return static_cast<TransmissionState>(_uh->hostTransmissionState());
    }

    /**
     * @brief Sets the HMC transmission state in the User Header
     *
     * @param[in] state - The state value
     */
    void setHMCTransmissionState(TransmissionState state)
    {
        _uh->setHMCTransmissionState(static_cast<uint8_t>(state));
    }

    /**
     * @brief Returns the HMC transmission state
     *
     * @return HMCTransmissionState - The state
     */
    TransmissionState hmcTransmissionState() const
    {
        return static_cast<TransmissionState>(_uh->hmcTransmissionState());
    }

  private:
    /**
     * @brief Builds the section objects from a PEL data buffer
     *
     * Note: The data parameter cannot be const for the same reasons
     * as listed in the constructor.
     *
     * @param[in] data - The PEL data
     * @param[in] obmcLogID - The OpenBMC event log ID to use for that
     *                        field in the Private Header.
     */
    void populateFromRawData(std::vector<uint8_t>& data, uint32_t obmcLogID);

    /**
     * @brief Flattens the PEL objects into the buffer
     *
     * @param[out] pelBuffer - What the data will be written to
     */
    void flatten(std::vector<uint8_t>& pelBuffer) const;

    /**
     * @brief Check that the PEL fields that need to be in agreement
     *        with each other are, and fix them up if necessary.
     */
    void checkRulesAndFix();

    /**
     * @brief Returns a map of the section IDs that appear more than once
     *        in the PEL.  The data value for each entry will be set to 0.
     *
     * @return std::map<uint16_t, size_t>
     */
    std::map<uint16_t, size_t> getPluralSections() const;

    /**
     * @brief Adds the UserData section to this PEL object,
     *        shrinking it if necessary
     *
     * @param[in] userData - The section to add
     *
     * @return bool - If the section was added or not.
     */
    bool addUserDataSection(std::unique_ptr<UserData> userData);

    /**
     * @brief helper function for printing PELs.
     * @param[in] Section& - section object reference
     * @param[in] std::string - PEL string
     * @param[in|out] pluralSections - Map used to track sections counts for
     *                                 when there is more than 1.
     * @param[in] registry - Registry object reference
     */
    void printSectionInJSON(const Section& section, std::string& buf,
                            std::map<uint16_t, size_t>& pluralSections,
                            message::Registry& registry) const;

    /**
     * @brief The PEL Private Header section
     */
    std::unique_ptr<PrivateHeader> _ph;

    /**
     * @brief The PEL User Header section
     */
    std::unique_ptr<UserHeader> _uh;

    /**
     * @brief Holds all sections by the PH and UH.
     */
    std::vector<std::unique_ptr<Section>> _optionalSections;

    /**
     * @brief The maximum size a PEL can be in bytes.
     */
    static constexpr size_t _maxPELSize = 16384;
};

namespace util
{

/**
 * @brief Creates a UserData section object that contains JSON.
 *
 * @param[in] json - The JSON contents
 *
 * @return std::unique_ptr<UserData> - The UserData object
 */
std::unique_ptr<UserData> makeJSONUserDataSection(const nlohmann::json& json);

/**
 * @brief Create a UserData section containing the AdditionalData
 *        contents as a JSON string.
 *
 * @param[in] ad - The AdditionalData contents
 *
 * @return std::unique_ptr<UserData> - The section
 */
std::unique_ptr<UserData> makeADUserDataSection(const AdditionalData& ad);

/**
 * @brief Create a UserData section containing various useful pieces
 *        of system information as a JSON string.
 *
 * @param[in] ad - The AdditionalData contents
 * @param[in] dataIface - The data interface object
 *
 * @return std::unique_ptr<UserData> - The section
 */
std::unique_ptr<UserData>
    makeSysInfoUserDataSection(const AdditionalData& ad,
                               const DataInterfaceBase& dataIface);

/**
 * @brief Create a UserData section that contains the data in the file
 *        pointed to by the file descriptor passed in.
 *
 * @param[in] componentID - The component ID of the PEL creator
 * @param[in] file - The FFDC file information
 */
std::unique_ptr<UserData> makeFFDCuserDataSection(uint16_t componentID,
                                                  const PelFFDCfile& file);
} // namespace util

} // namespace pels
} // namespace openpower
