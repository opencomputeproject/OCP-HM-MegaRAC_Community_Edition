#pragma once

#include "registry.hpp"
#include "stream.hpp"

#include <string>

namespace openpower
{
namespace pels
{
namespace src
{

const size_t asciiStringSize = 32;

/**
 * @class AsciiString
 *
 * This represents the ASCII string field in the SRC PEL section.
 * This 32 character string shows up on the panel on a function 11.
 *
 * The first 2 characters are the SRC type, like 'BD' or '11'.
 * Next is the subsystem, like '8D', if a BD SRC, otherwise '00'.
 * Next is the reason code, like 'AAAA'.
 * The rest is filled in with spaces.
 */
class AsciiString
{
  public:
    AsciiString() = delete;
    ~AsciiString() = default;
    AsciiString(const AsciiString&) = default;
    AsciiString& operator=(const AsciiString&) = default;
    AsciiString(AsciiString&&) = default;
    AsciiString& operator=(AsciiString&&) = default;

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit AsciiString(Stream& stream);

    /**
     * @brief Constructor
     *
     * Fills in the class from the registry entry
     */
    explicit AsciiString(const message::Entry& entry);

    /**
     * @brief Flatten the object into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& stream) const;

    /**
     * @brief Fills in the object from the stream data
     *
     * @param[in] stream - The stream to read from
     */
    void unflatten(Stream& stream);

    /**
     * @brief Return the 32 character ASCII string data
     *
     * @return std::string - The data
     */
    std::string get() const;

  private:
    /**
     * @brief Converts a byte of raw data to 2 characters
     *        and writes it to the offset.
     *
     * For example, if string is: "AABBCCDD"
     *
     * setByte(0, 0x11);
     * setByte(1, 0x22);
     * setByte(2, 0x33);
     * setByte(3, 0x44);
     *
     * results in "11223344"
     *
     * @param[in] offset - The offset into the ascii string
     * @param[in] value - The value to write (0x55 -> "55")
     */
    void setByte(size_t offset, uint8_t value);

    /**
     * @brief The ASCII string itself
     */
    std::array<char, asciiStringSize> _string;
};

} // namespace src

} // namespace pels
} // namespace openpower
