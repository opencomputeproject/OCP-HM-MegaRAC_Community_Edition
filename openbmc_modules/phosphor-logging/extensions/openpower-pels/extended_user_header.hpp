#pragma once

#include "bcd_time.hpp"
#include "data_interface.hpp"
#include "elog_entry.hpp"
#include "mtms.hpp"
#include "registry.hpp"
#include "section.hpp"
#include "src.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{

constexpr uint8_t extendedUserHeaderVersion = 0x01;
constexpr size_t firmwareVersionSize = 16;

/**
 * @class ExtendedUserHeader
 *
 * This represents the Extended User Header section in a PEL.  It is  a required
 * section.  It contains code versions, an MTMS subsection, and a string called
 * a symptom ID.
 *
 * The Section base class handles the section header structure that every
 * PEL section has at offset zero.
 */
class ExtendedUserHeader : public Section
{
  public:
    ExtendedUserHeader() = delete;
    ~ExtendedUserHeader() = default;
    ExtendedUserHeader(const ExtendedUserHeader&) = default;
    ExtendedUserHeader& operator=(const ExtendedUserHeader&) = default;
    ExtendedUserHeader(ExtendedUserHeader&&) = default;
    ExtendedUserHeader& operator=(ExtendedUserHeader&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit ExtendedUserHeader(Stream& pel);

    /**
     * @brief Constructor
     *
     * @param[in] dataIface - The DataInterface object
     * @param[in] regEntry - The message registry entry for this event
     * @param[in] src - The SRC section object for this event
     */
    ExtendedUserHeader(const DataInterfaceBase& dataIface,
                       const message::Entry& regEntry, const SRC& src);

    /**
     * @brief Flatten the section into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& stream) const override;

    /**
     * @brief Returns the size of this section when flattened into a PEL
     *
     * @return size_t - the size of the section
     */
    size_t flattenedSize()
    {
        return Section::flattenedSize() + _mtms.flattenedSize() +
               _serverFWVersion.size() + _subsystemFWVersion.size() +
               sizeof(_reserved4B) + sizeof(_refTime) + sizeof(_reserved1B1) +
               sizeof(_reserved1B2) + sizeof(_reserved1B3) +
               sizeof(_symptomIDSize) + _symptomIDSize;
    }

    /**
     * @brief Returns the server firmware version
     *
     * @return std::string - The version
     */
    std::string serverFWVersion() const
    {
        std::string version;
        for (size_t i = 0;
             i < _serverFWVersion.size() && _serverFWVersion[i] != '\0'; i++)
        {
            version.push_back(_serverFWVersion[i]);
        }
        return version;
    }

    /**
     * @brief Returns the subsystem firmware version
     *
     * @return std::string - The version
     */
    std::string subsystemFWVersion() const
    {
        std::string version;
        for (size_t i = 0;
             i < _subsystemFWVersion.size() && _subsystemFWVersion[i] != '\0';
             i++)
        {
            version.push_back(_subsystemFWVersion[i]);
        }
        return version;
    }

    /**
     * @brief Returns the machine type+model
     *
     * @return std::string - The MTM
     */
    std::string machineTypeModel() const
    {
        return _mtms.machineTypeAndModel();
    }

    /**
     * @brief Returns the machine serial number
     *
     * @return std::string - The serial number
     */
    std::string machineSerialNumber() const
    {
        return _mtms.machineSerialNumber();
    }

    /**
     * @brief Returns the Event Common Reference Time field
     *
     * @return BCDTime - The time value
     */
    const BCDTime& refTime() const
    {
        return _refTime;
    }

    /**
     * @brief Returns the symptom ID
     *
     * @return std::string - The symptom ID
     */
    std::string symptomID() const
    {
        std::string symptom;
        for (size_t i = 0; i < _symptomID.size() && _symptomID[i] != '\0'; i++)
        {
            symptom.push_back(_symptomID[i]);
        }
        return symptom;
    }

    /**
     * @brief Get section in JSON.
     * @return std::optional<std::string> - ExtendedUserHeader section's JSON
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
     * @brief Builds the symptom ID
     *
     * Uses the message registry to say which SRC fields to add
     * to the SRC's ASCII string to make the ID, and uses a smart
     * default if not specified in the registry.
     *
     * @param[in] regEntry - The message registry entry for this event
     * @param[in] src - The SRC section object for this event
     */
    void createSymptomID(const message::Entry& regEntry, const SRC& src);

    /**
     * @brief The structure that holds the machine TM and SN fields.
     */
    MTMS _mtms;

    /**
     * @brief The server firmware version
     *
     * NULL terminated.
     *
     * The release version of the full firmware image.
     */
    std::array<uint8_t, firmwareVersionSize> _serverFWVersion;

    /**
     * @brief The subsystem firmware version
     *
     * NULL terminated.
     *
     * On PELs created on the BMC, this will be the BMC code version.
     */
    std::array<uint8_t, firmwareVersionSize> _subsystemFWVersion;

    /**
     * @brief Reserved
     */
    uint32_t _reserved4B = 0;

    /**
     * @brief Event Common Reference Time
     *
     * This is not used by PELs created on the BMC.
     */
    BCDTime _refTime;

    /**
     * @brief Reserved
     */
    uint8_t _reserved1B1 = 0;

    /**
     * @brief Reserved
     */
    uint8_t _reserved1B2 = 0;

    /**
     * @brief Reserved
     */
    uint8_t _reserved1B3 = 0;

    /**
     * @brief The size of the symptom ID field
     */
    uint8_t _symptomIDSize;

    /**
     * @brief The symptom ID field
     *
     * Describes a unique event signature for the log.
     * Required for serviceable events, otherwise optional.
     * When present, must start with the first 8 characters
     * of the ASCII string field from the SRC.
     *
     * NULL terminated.
     */
    std::vector<uint8_t> _symptomID;
};

} // namespace pels
} // namespace openpower
