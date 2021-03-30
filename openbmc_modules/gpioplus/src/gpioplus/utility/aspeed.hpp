#pragma once
#include <stdexcept>
#include <string_view>

namespace gpioplus
{
namespace utility
{
namespace aspeed
{

/** @brief Converts an aspeed gpio label name into an offset usable
 *         by a gpioplus line.
 *         Ex. GPION3 -> nameToOffset("N3") -> 107
 *
 *         NOTE: This function is constexpr, but uses exceptions for error
 *         cases. If you want compile time guarantees of validity, make sure
 *         you assign to a constexpr lvalue. Otherwise the error will be
 *         resolved at runtime.
 *         Ex. constexpr uint32_t offset = nameToOffset("A7");
 *
 *  @param[in] name - The gpio chip which provides the events
 *  @throws std::logic_error when provided an invalid name.
 *  @return
 */
constexpr uint32_t nameToOffset(std::string_view name)
{
    // Validate the name [A-Z]+[0-9]
    if (name.length() < 2)
    {
        throw std::logic_error("GPIO name is too short");
    }

    char octal = name[name.length() - 1];
    if (octal < '0' || octal > '7')
    {
        throw std::logic_error("GPIO name must end with an octal");
    }

    auto letters = name.substr(0, name.length() - 1);
    for (auto letter : letters)
    {
        if (letter < 'A' || letter > 'Z')
        {
            throw std::logic_error("GPIO name must start with letters");
        }
    }

    // Limit the gpio to reasonable values
    // Current chips have no more than AB7 or 224 gpios
    if (name.length() > 3 || (name.length() == 3 && name[0] != 'A'))
    {
        throw std::logic_error("GPIO name probably not valid");
    }

    uint32_t offset = 0;
    for (auto letter : letters)
    {
        offset = offset * 26 + (letter - 'A') + 1;
    }
    offset = (offset - 1) * 8 + (octal - '0');
    return offset;
}

} // namespace aspeed
} // namespace utility
} // namespace gpioplus
