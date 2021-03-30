#pragma once

#include "bcd_time.hpp"
#include "section.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{

struct CreatorVersion
{
    uint8_t version[8];

    CreatorVersion()
    {
        memset(version, '\0', sizeof(version));
    }
};

static constexpr uint8_t privateHeaderVersion = 0x01;
static constexpr uint8_t minSectionCount = 2;

/**
 * @class PrivateHeader
 *
 * This represents the Private Header section in a PEL.  It is required,
 * and it is always the first section.
 *
 * The Section base class handles the section header structure that every
 * PEL section has at offset zero.
 *
 * The fields in this class directly correspond to the order and sizes of
 * the fields in the section.
 */
class PrivateHeader : public Section
{
  public:
    PrivateHeader() = delete;
    ~PrivateHeader() = default;
    PrivateHeader(const PrivateHeader&) = default;
    PrivateHeader& operator=(const PrivateHeader&) = default;
    PrivateHeader(PrivateHeader&&) = default;
    PrivateHeader& operator=(PrivateHeader&&) = default;

    /**
     * @brief Constructor
     *
     * Creates a valid PrivateHeader with the passed in data
     *
     * @param[in] componentID - the creator's component ID
     * @param[in] obmcLogID - the corresponding OpenBMC event log ID
     * @param[in] timestamp - the creation timestamp, in epoch milliseconds
     */
    PrivateHeader(uint16_t componentID, uint32_t obmcLogID, uint64_t timestamp);

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     *
     */
    explicit PrivateHeader(Stream& pel);

    /**
     * @brief Flatten the section into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& stream) const override;

    /**
     * @brief Returns the creation timestamp
     *
     * @return const BCDTime& - the timestamp
     */
    const BCDTime& createTimestamp() const
    {
        return _createTimestamp;
    }

    /**
     * @brief Returns the commit time timestamp
     *
     * @return const BCDTime& - the timestamp
     */
    const BCDTime& commitTimestamp() const
    {
        return _commitTimestamp;
    }

    /**
     * @brief Sets the commit timestamp
     *
     * @param[in] time - the new timestamp
     */
    void setCommitTimestamp(const BCDTime& time)
    {
        _commitTimestamp = time;
    }

    /**
     * @brief Returns the creator ID field
     *
     * @return uint8_t - the creator ID
     */
    uint8_t creatorID() const
    {
        return _creatorID;
    }

    /**
     * @brief Returns the log type field
     *
     * @return uint8_t - the log type
     */
    uint8_t logType() const
    {
        return _logType;
    }

    /**
     * @brief Returns the section count field
     *
     * @return uint8_t - the section count
     */
    uint8_t sectionCount() const
    {
        return _sectionCount;
    }

    /**
     * @brief Sets the section count field
     *
     * @param[in] count - the new section count
     */
    void setSectionCount(uint8_t count)
    {
        _sectionCount = count;
    }

    /**
     * @brief Returns the OpenBMC log ID field
     *
     * This is the ID the OpenBMC event log that corresponds
     * to this PEL.
     *
     * @return uint32_t - the OpenBMC event log ID
     */
    uint32_t obmcLogID() const
    {
        return _obmcLogID;
    }

    /**
     * @brief Sets the OpenBMC log ID field
     *
     * @param[in] id - the new ID
     */
    void setOBMCLogID(uint32_t id)
    {
        _obmcLogID = id;
    }

    /**
     * @brief Returns the Creator Version field
     *
     * @return CreatorVersion& - the creator version
     */
    const CreatorVersion& creatorVersion() const
    {
        return _creatorVersion;
    }

    /**
     * @brief Returns the error log ID field
     *
     * @return uint32_t - the error log ID
     */
    uint32_t id() const
    {
        return _id;
    }

    /**
     * @brief Sets the ID field
     *
     * @param[in] id - the new ID
     */
    void setID(uint32_t id)
    {
        _id = id;
    }

    /**
     * @brief Returns the platform log ID field
     *
     * @return uint32_t - the platform log ID
     */
    uint32_t plid() const
    {
        return _plid;
    }

    /**
     * @brief Returns the size of this section when flattened into a PEL
     *
     * @return size_t - the size of the section
     */
    static constexpr size_t flattenedSize()
    {
        return Section::flattenedSize() + sizeof(_createTimestamp) +
               sizeof(_commitTimestamp) + sizeof(_creatorID) +
               sizeof(_logType) + sizeof(_reservedByte) +
               sizeof(_sectionCount) + sizeof(_obmcLogID) +
               sizeof(_creatorVersion) + sizeof(_plid) + sizeof(_id);
    }

    /**
     * @brief Get section in JSON.
     * @return std::optional<std::string> - Private header section's JSON
     */
    std::optional<std::string> getJSON() const override;

  private:
    /**
     * @brief Fills in the object from the stream data
     *
     * @param[in] stream - The stream to read from
     */
    void unflatten(Stream& stream);

    /**
     * @brief Validates the section contents
     *
     * Updates _valid (in Section) with the results.
     */
    void validate() override;

    /**
     * @brief The creation time timestamp
     */
    BCDTime _createTimestamp;

    /**
     * @brief The commit time timestamp
     */
    BCDTime _commitTimestamp;

    /**
     * @brief The creator ID field
     */
    uint8_t _creatorID;

    /**
     * @brief The log type field
     */
    uint8_t _logType;

    /**
     * @brief A reserved byte.
     */
    uint8_t _reservedByte;

    /**
     * @brief The section count field, which is the total number
     * of sections in the PEL.
     */
    uint8_t _sectionCount;

    /**
     * @brief The OpenBMC event log ID that corresponds to this PEL.
     */
    uint32_t _obmcLogID;

    /**
     * @brief The creator subsystem version field
     */
    CreatorVersion _creatorVersion;

    /**
     * @brief The platform log ID field
     */
    uint32_t _plid;

    /**
     * @brief The log entry ID field
     */
    uint32_t _id;
};

/**
 * @brief Stream extraction operator for the CreatorVersion
 *
 * @param[in] s - the stream
 * @param[out] cv - the CreatorVersion object
 */
Stream& operator>>(Stream& s, CreatorVersion& cv);

/**
 * @brief Stream insertion operator for the CreatorVersion
 *
 * @param[out] s - the stream
 * @param[in] cv - the CreatorVersion object
 */
Stream& operator<<(Stream& s, const CreatorVersion& cv);

} // namespace pels
} // namespace openpower
