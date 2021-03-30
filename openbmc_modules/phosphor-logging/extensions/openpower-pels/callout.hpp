#pragma once

#include "fru_identity.hpp"
#include "mru.hpp"
#include "pce_identity.hpp"
#include "pel_types.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{
namespace src
{

/**
 * @class Callout
 *
 * Represents a single FRU callout in the SRC's FRU callout
 * subsection.
 *
 * The 'Callouts' class holds a list of these objects.
 *
 * The callout priority and location code are in this structure.
 *
 * There can also be up to one each of three types of substructures
 * in a single callout:
 *  * FRU Identity  (must be first if present)
 *  * Power Controlling Enclosure (PCE)
 *  * Manufacturing Replaceable Unit (MRU)
 *
 * These substructures have their own objects managed by unique_ptrs
 * which will only be allocated if those substructures exist.
 */
class Callout
{
  public:
    /**
     * @brief Which callout substructures are included.
     */
    enum calloutFlags
    {
        calloutType = 0b0010'0000,
        fruIdentIncluded = 0b0000'1000,
        mruIncluded = 0b0000'0100

        // Leaving out the various PCE identity ones since
        // we don't use them.
    };

    Callout() = delete;
    ~Callout() = default;
    Callout(const Callout&) = delete;
    Callout& operator=(const Callout&) = delete;
    Callout(Callout&&) = delete;
    Callout& operator=(Callout&&) = delete;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit Callout(Stream& pel);

    /**
     * @brief Constructor
     *
     * Creates the objects with a FRUIdentity substructure that calls
     * out a normal hardware FRU.
     *
     * @param[in] priority - The priority of the callout
     * @param[in] locationCode - The location code of the callout
     * @param[in] partNumber - The part number of the callout
     * @param[in] ccin - The CCIN of the callout
     * @param[in] serialNumber - The serial number of the callout
     */
    Callout(CalloutPriority priority, const std::string& locationCode,
            const std::string& partNumber, const std::string& ccin,
            const std::string& serialNumber);

    /**
     * @brief Constructor
     *
     * Creates the objects with a FRUIdentity substructure that calls
     * out a maintenance procedure.
     *
     * @param[in] priority - The priority of the callout
     * @param[in] procedureFromRegistry - The maintenance procedure name
     *                                    as defined in the message registry.
     */
    Callout(CalloutPriority priority, const std::string& procedureFromRegistry);

    /**
     * @brief Constructor
     *
     * Creates the objects with a FRUIdentity substructure that calls
     * out a symbolic FRU.
     *
     * @param[in] priority - The priority of the callout
     * @param[in] symbolicFRUFromRegistry - The symbolic FRU name as
     *                                      defined in the message registry.
     * @param[in] locationCode - The location code of the callout
     * @param[in] trustedLocationCode - If the location is trusted
     */
    Callout(CalloutPriority priority,
            const std::string& symbolicFRUFromRegistry,
            const std::string& locationCode, bool trustedLocationCode);

    /**
     * @brief Returns the size of this object when flattened into a PEL
     *
     * @return size_t - The size of the section
     */
    size_t flattenedSize() const;

    /**
     * @brief Flatten the object into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& pel) const;

    /**
     * @brief Returns the flags field of a callout
     *
     * @return uint8_t - The flags
     */
    uint8_t flags() const
    {
        return _flags;
    }

    /**
     * @brief Returns the priority field of a callout
     *
     * @return uint8_t - The priority
     */
    uint8_t priority() const
    {
        return _priority;
    }

    /**
     * @brief Returns the location code of the callout
     *
     * @return std::string - The location code
     */
    std::string locationCode() const
    {
        std::string lc;
        if (!_locationCode.empty())
        {
            // NULL terminated
            lc = static_cast<const char*>(_locationCode.data());
        }
        return lc;
    }

    /**
     * @brief Returns the location code size
     *
     * @return size_t - The size, including the terminating null.
     */
    size_t locationCodeSize() const
    {
        return _locationCodeSize;
    }

    /**
     * @brief Returns the FRU identity substructure
     *
     * @return const std::unique_ptr<FRUIdentity>&
     */
    const std::unique_ptr<FRUIdentity>& fruIdentity() const
    {
        return _fruIdentity;
    }

    /**
     * @brief Returns the PCE identity substructure
     *
     * @return const std::unique_ptr<PCEIdentity>&
     */
    const std::unique_ptr<PCEIdentity>& pceIdentity() const
    {
        return _pceIdentity;
    }

    /**
     * @brief Returns the MRU identity substructure
     *
     * @return const std::unique_ptr<MRU>&
     */
    const std::unique_ptr<MRU>& mru() const
    {
        return _mru;
    }

  private:
    /**
     * @brief Sets the location code field
     *
     * @param[in] locationCode - The location code string
     */
    void setLocationCode(const std::string& locationCode);

    /**
     * @brief The size of this structure in the PEL
     */
    uint8_t _size;

    /**
     * @brief The flags byte of this structure
     */
    uint8_t _flags;

    /**
     * @brief The replacement priority
     */
    uint8_t _priority;

    /**
     * @brief The length of the location code field.
     *
     * Includes the NULL termination, and must be a
     * multiple of 4 (padded with zeros)
     */
    uint8_t _locationCodeSize = 0;

    /**
     * @brief NULL terminated location code
     *
     * Includes the NULL termination, and must be a
     * multiple of 4 (padded with zeros)
     */
    std::vector<char> _locationCode;

    /**
     * @brief FRU (Field Replaceable Unit) Identity substructure
     */
    std::unique_ptr<FRUIdentity> _fruIdentity;

    /**
     * @brief PCE (Power Controlling Enclosure) Identity substructure
     */
    std::unique_ptr<PCEIdentity> _pceIdentity;

    /**
     * @brief MRU (Manufacturing Replaceable Unit) substructure
     */
    std::unique_ptr<MRU> _mru;
};

} // namespace src
} // namespace pels
} // namespace openpower
