#pragma once

#include "stream.hpp"

namespace openpower
{
namespace pels
{
namespace src
{

/**
 * @class MRU
 *
 * This represents the MRU (Manufacturing Replaceable Unit)
 * substructure in the callout subsection of the SRC PEL section.
 *
 * Manufacturing replaceable units have a finer granularity than
 * a field replaceable unit, such as a chip on a card, and are
 * intended to be used during manufacturing.
 *
 * This substructure can contain up to 128 MRU callouts, each
 * containing a MRU ID and a callout priority value.
 */
class MRU
{
  public:
    /**
     * @brief A single MRU callout, which contains a priority
     *        and a MRU ID.
     *
     * The priority value is the same priority type character
     * value as in the parent callout structure. For alignment
     * purposes it is a 4 byte field, though only the LSB contains
     * the priority value.
     */
    struct MRUCallout
    {
        uint32_t priority;
        uint32_t id;
    };

    MRU() = delete;
    ~MRU() = default;
    MRU(const MRU&) = default;
    MRU& operator=(const MRU&) = default;
    MRU(MRU&&) = default;
    MRU& operator=(MRU&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit MRU(Stream& pel);

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
    size_t flattenedSize() const
    {
        return _size;
    }

    /**
     * @brief Returns the contained MRU callouts.
     *
     * @return const std::vector<MRUCallout>& - The MRUs
     */
    const std::vector<MRUCallout>& mrus() const
    {
        return _mrus;
    }

    /**
     * @brief The type identifier value of this structure.
     */
    static const uint16_t substructureType = 0x4D52; // "MR"

  private:
    /**
     * @brief The callout substructure type field. Will be 'MR'.
     */
    uint16_t _type;

    /**
     * @brief The size of this callout structure.
     */
    uint8_t _size;

    /**
     * @brief The flags byte of this substructure.
     *
     * 0x0Y: Y = number of MRU callouts
     */
    uint8_t _flags;

    /**
     * @brief Reserved 4 bytes
     */
    uint32_t _reserved4B;

    /*
     * @brief The MRU callouts
     */
    std::vector<MRUCallout> _mrus;
};

} // namespace src
} // namespace pels
} // namespace openpower
