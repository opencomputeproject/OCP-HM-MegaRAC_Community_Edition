#pragma once

#include "stream.hpp"

#include <cstdint>

namespace openpower
{
namespace pels
{

/**
 * @class SectionHeader
 *
 * This header is at the start of every PEL section.  It has a size
 * of 8 bytes.
 */
struct SectionHeader
{
  public:
    /**
     * @brief Constructor
     */
    SectionHeader() : id(0), size(0), version(0), subType(0), componentID(0)
    {
    }

    /**
     * @brief Constructor
     *
     * @param[in] id - the ID field
     * @param[in] size - the size field
     * @param[in] version - the version field
     * @param[in] subType - the sub-type field
     * @param[in] componentID - the component ID field
     */
    SectionHeader(uint16_t id, uint16_t size, uint8_t version, uint8_t subType,
                  uint16_t componentID) :
        id(id),
        size(size), version(version), subType(subType), componentID(componentID)
    {
    }

    /**
     * @brief A two character ASCII field which identifies the section type.
     */
    uint16_t id;

    /**
     * @brief The size of the section in bytes, including this section header.
     */
    uint16_t size;

    /**
     * @brief The section format version.
     */
    uint8_t version;

    /**
     * @brief The section sub-type.
     */
    uint8_t subType;

    /**
     * @brief The component ID, which has various meanings depending on the
     * section.
     */
    uint16_t componentID;

    /**
     * @brief Returns the size of header when flattened into a PEL.
     *
     * @return size_t - the size of the header
     */
    static constexpr size_t flattenedSize()
    {
        return sizeof(id) + sizeof(size) + sizeof(version) + sizeof(subType) +
               sizeof(componentID);
    }
};

/**
 * @brief Stream extraction operator for the SectionHeader
 *
 * @param[in] s - the stream
 * @param[out] header - the SectionHeader object
 */
inline Stream& operator>>(Stream& s, SectionHeader& header)
{
    s >> header.id >> header.size >> header.version >> header.subType >>
        header.componentID;
    return s;
}

/**
 * @brief Stream insertion operator for the section header
 *
 * @param[out] s - the stream
 * @param[in] header - the SectionHeader object
 */
inline Stream& operator<<(Stream& s, const SectionHeader& header)
{
    s << header.id << header.size << header.version << header.subType
      << header.componentID;
    return s;
}

} // namespace pels
} // namespace openpower
