#pragma once

#include "section.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{

/**
 * @class UserData
 *
 * This represents the User Data section in a PEL.  It is free form data
 * that the creator knows the contents of.  The component ID, version,
 * and sub-type fields in the section header are used to identify the
 * format.
 *
 * The Section base class handles the section header structure that every
 * PEL section has at offset zero.
 */
class UserData : public Section
{
  public:
    UserData() = delete;
    ~UserData() = default;
    UserData(const UserData&) = default;
    UserData& operator=(const UserData&) = default;
    UserData(UserData&&) = default;
    UserData& operator=(UserData&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit UserData(Stream& pel);

    /**
     * @brief Constructor
     *
     * Create a valid UserData object with the passed in data.
     *
     * The component ID, subtype, and version are used to identify
     * the data to know which parser to call.
     *
     * @param[in] componentID - Component ID of the creator
     * @param[in] subType - The type of user data
     * @param[in] version - The version of the data
     */
    UserData(uint16_t componentID, uint8_t subType, uint8_t version,
             const std::vector<uint8_t>& data);

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

    /**
     * @brief Get the section contents in JSON
     *
     * @return The JSON as a string if a parser was found,
     *         otherwise std::nullopt.
     */
    std::optional<std::string> getJSON() const override;

    /**
     * @brief Shrink the section
     *
     * The new size must be between the current size and the minimum
     * size, which is 12 bytes.  If it isn't a 4 byte aligned value
     * the code will do the aligning before the resize takes place.
     *
     * @param[in] newSize - The new size in bytes
     *
     * @return bool - true if successful, false else.
     */
    bool shrink(size_t newSize) override;

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
