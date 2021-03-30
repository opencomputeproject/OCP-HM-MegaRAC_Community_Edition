#pragma once

#include "section.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{

/**
 * @class Generic
 *
 * This class is used for a PEL section when there is no other class to use.
 * It just contains a vector of the raw data.  Its purpose is so that a PEL
 * can be completely unflattened even if the code doesn't have a class for
 * every section type.
 */
class Generic : public Section
{
  public:
    Generic() = delete;
    ~Generic() = default;
    Generic(const Generic&) = default;
    Generic& operator=(const Generic&) = default;
    Generic(Generic&&) = default;
    Generic& operator=(Generic&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit Generic(Stream& pel);

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
        return Section::flattenedSize() + _data.size();
    }

    /**
     * @brief Returns the raw section data
     *
     * @return std::vector<uint8_t>&
     */
    const std::vector<uint8_t>& data() const
    {
        return _data;
    }

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
     * @brief The section data
     */
    std::vector<uint8_t> _data;
};

} // namespace pels
} // namespace openpower
