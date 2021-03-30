#pragma once

#include "pel_types.hpp"
#include "stream.hpp"

#include <optional>

namespace openpower
{
namespace pels
{
namespace src
{

/**
 * @class FRUIdentity
 *
 * This represents the FRU Identity substructure in the
 * callout subsection of the SRC PEL section.
 *
 * It provides information about the FRU being called out,
 * such as serial number and part number.  A maintenance
 * procedure name may be used instead of the part number,
 * and this would be indicated in the flags field.
 */
class FRUIdentity
{
  public:
    /**
     * @brief The failing component type
     *
     * Upper nibble of the flags byte
     */
    enum FailingComponentType
    {
        hardwareFRU = 0x10,
        codeFRU = 0x20,
        configError = 0x30,
        maintenanceProc = 0x40,
        externalFRU = 0x90,
        externalCodeFRU = 0xA0,
        toolFRU = 0xB0,
        symbolicFRU = 0xC0,
        symbolicFRUTrustedLocCode = 0xE0
    };

    /**
     * @brief The lower nibble of the flags byte
     */
    enum Flags
    {
        pnSupplied = 0x08,
        ccinSupplied = 0x04,
        maintProcSupplied = 0x02,
        snSupplied = 0x01
    };

    FRUIdentity() = delete;
    ~FRUIdentity() = default;
    FRUIdentity(const FRUIdentity&) = default;
    FRUIdentity& operator=(const FRUIdentity&) = default;
    FRUIdentity(FRUIdentity&&) = default;
    FRUIdentity& operator=(FRUIdentity&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit FRUIdentity(Stream& pel);

    /**
     * Constructor
     *
     * Creates the object as a hardware callout with the part number,
     * CCIN, and serial number fields supplied.
     *
     * @param[in] partNumber - The part number of the FRU
     * @param[in] ccin - The CCIN of the FRU
     * @param[in] serialNumber - The serial number of the FRU
     */
    FRUIdentity(const std::string& partNumber, const std::string& ccin,
                const std::string& serialNumber);

    /**
     * @brief Constructor
     *
     * Creates the object with a maintenance procedure callout.
     *
     * @param[in] procedureFromRegistry - The maintenance procedure name
     *                                    as defined in the message registry.
     */
    FRUIdentity(const std::string& procedureFromRegistry);

    /**
     * @brief Constructor
     *
     * Creates the object with a symbolic FRU callout.
     *
     * @param[in] symbolicFRUFromRegistry - The symbolic FRU name as
     *                                      defined in the message registry.
     * @param[in] trustedLocationCode - If this FRU callout's location code
     *                                  can be trusted to be correct.
     */
    FRUIdentity(const std::string& symbolicFRUFromRegistry,
                bool trustedLocationCode);

    /**
     * @brief Flatten the object into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& pel) const;

    /**
     * @brief Returns the size of this structure when flattened into a PEL
     *
     * @return size_t - The size of the section
     */
    size_t flattenedSize() const;

    /**
     * @brief Returns the type field
     *
     * @return uint16_t - The type, always 0x4944 "ID".
     */
    uint16_t type() const
    {
        return _type;
    }
    /**
     * @brief The failing component type for this FRU callout.
     *
     * @return FailingComponentType
     */
    FailingComponentType failingComponentType() const
    {
        return static_cast<FailingComponentType>(_flags & 0xF0);
    }

    /**
     * @brief Returns the part number, if supplied
     *
     * @return std::optional<std::string>
     */
    std::optional<std::string> getPN() const;

    /**
     * @brief Returns the maintenance procedure, if supplied
     *
     * @return std::optional<std::string>
     */
    std::optional<std::string> getMaintProc() const;

    /**
     * @brief Returns the CCIN, if supplied
     *
     * @return std::optional<std::string>
     */
    std::optional<std::string> getCCIN() const;

    /**
     * @brief Returns the serial number, if supplied
     *
     * @return std::optional<std::string>
     */
    std::optional<std::string> getSN() const;

    /**
     * @brief The type identifier value of this structure.
     */
    static const uint16_t substructureType = 0x4944; // "ID"

  private:
    /**
     * @brief If the part number is contained in this structure.
     *
     * It takes the place of the maintenance procedure ID.
     *
     * @return bool
     */
    bool hasPN() const
    {
        return _flags & pnSupplied;
    }

    /**
     * @brief If the CCIN is contained in this structure.
     *
     * @return bool
     */
    bool hasCCIN() const
    {
        return _flags & ccinSupplied;
    }

    /**
     * @brief If a maintenance procedure is contained in this structure.
     *
     * It takes the place of the part number.
     *
     * @return bool
     */
    bool hasMP() const
    {
        return _flags & maintProcSupplied;
    }

    /**
     * @brief If the serial number is contained in this structure.
     *
     * @return bool
     */
    bool hasSN() const
    {
        return _flags & snSupplied;
    }

    /**
     * @brief Sets the 8 character null terminated part
     *        number field to the string passed in.
     *
     * @param[in] partNumber - The part number string.
     */
    void setPartNumber(const std::string& partNumber);

    /**
     * @brief Sets the 4 character CCIN field.
     *
     * @param[in] ccin - The CCIN string
     */
    void setCCIN(const std::string& ccin);

    /**
     * @brief Sets the 12 character serial number field.
     *
     * @param[in] serialNumber - The serial number string
     */
    void setSerialNumber(const std::string& serialNumber);

    /**
     * @brief Sets the 8 character null terminated procedure
     *        field.  This is in the same field as the part
     *        number since they are mutually exclusive.
     *
     * @param procedureFromRegistry - The procedure name as defined in
     *                                the PEL message registry.
     */
    void setMaintenanceProcedure(const std::string& procedureFromRegistry);

    /**
     * @brief Sets the 8 character null terminated symbolic FRU
     *        field.  This is in the same field as the part
     *        number since they are mutually exclusive.
     *
     * @param[in] symbolicFRUFromRegistry - The symbolic FRU name as
     *                                      defined in the message registry.
     */
    void setSymbolicFRU(const std::string& symbolicFRUFromRegistry);

    /**
     * @brief The callout substructure type field. Will be "ID".
     */
    uint16_t _type;

    /**
     * @brief The size of this callout structure.
     *
     * Always a multiple of 4.
     */
    uint8_t _size;

    /**
     * @brief The flags byte of this substructure.
     *
     * See the FailingComponentType and Flags enums
     */
    uint8_t _flags;

    /**
     * @brief The part number OR maintenance procedure ID,
     *        depending on what the flags field specifies.
     *
     * A NULL terminated ASCII string.
     */
    std::array<char, 8> _pnOrProcedureID;

    /**
     * @brief The CCIN VPD keyword
     *
     * Four ASCII characters, not NULL terminated.
     */
    std::array<char, 4> _ccin;

    /**
     * @brief The serial number
     *
     * Twelve ASCII characters, not NULL terminated.
     */
    std::array<char, 12> _sn;
};

} // namespace src
} // namespace pels
} // namespace openpower
