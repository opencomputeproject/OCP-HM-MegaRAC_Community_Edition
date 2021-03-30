#pragma once

#include "mtms.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{
namespace src
{

/**
 * @class PCEIdentity
 *
 * This represents the PCE (Power Controlling Enclosure) Identity
 * substructure in the callout subsection of the SRC PEL section.
 *
 * It contains the name and machine type/model/SN of that enclosure.
 *
 * It is only used for errors in an I/O drawer where another enclosure
 * may control its power.  It is not used in BMC errors and so will
 * never be created by the BMC, but only unflattened in errors it
 * receives from the host.
 */
class PCEIdentity
{
  public:
    PCEIdentity() = delete;
    ~PCEIdentity() = default;
    PCEIdentity(const PCEIdentity&) = default;
    PCEIdentity& operator=(const PCEIdentity&) = default;
    PCEIdentity(PCEIdentity&&) = default;
    PCEIdentity& operator=(PCEIdentity&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit PCEIdentity(Stream& pel);

    /**
     * @brief Flatten the object into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& pel) const;

    /**
     * @brief Returns the size of this structure when flattened into a PEL
     *
     * @return size_t - The size of the structure
     */
    size_t flattenedSize()
    {
        return _size;
    }

    /**
     * @brief The type identifier value of this structure.
     */
    static const uint16_t substructureType = 0x5045; // "PE"

    /**
     * @brief Returns the enclosure name
     *
     * @return std::string - The enclosure name
     */
    std::string enclosureName() const
    {
        // _pceName is NULL terminated
        std::string name{static_cast<const char*>(_pceName.data())};
        return name;
    }

    /**
     * @brief Returns the MTMS sub structure
     *
     * @return const MTMS& - The machine type/model/SN structure.
     */
    const MTMS& mtms() const
    {
        return _mtms;
    }

  private:
    /**
     * @brief The callout substructure type field. Will be 'PE'.
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
     * Always 0 for this structure.
     */
    uint8_t _flags;

    /**
     * @brief The structure that holds the power controlling enclosure's
     *        machine type, model, and serial number.
     */
    MTMS _mtms;

    /**
     * @brief The name of the power controlling enclosure.
     *
     * Null terminated and padded with NULLs to a 4 byte boundary.
     */
    std::vector<char> _pceName;
};

} // namespace src
} // namespace pels
} // namespace openpower
