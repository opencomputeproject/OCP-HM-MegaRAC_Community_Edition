#pragma once

#include <cstdint>
#include <tuple>

namespace openpower
{
namespace pels
{
namespace pel_rules
{

/**
 * @brief Ensure certain PEL fields are in agreement, and fix them if they
 *        aren't.  These rules are documented in the README.md in this
 *        directory.
 *
 * Note: The message registry schema enforces that there are no undefined
 *       bits set in these fields.
 *
 * @param[in] actionFlags - The current Action Flags value
 * @param[in] eventType - The current Event Type value
 * @param[in] severity - The current Severity value
 *
 * @return std::tuple<actionFlags, eventType> - The corrected values.
 */
std::tuple<uint16_t, uint8_t> check(uint16_t actionFlags, uint8_t eventType,
                                    uint8_t severity);

} // namespace pel_rules
} // namespace pels
} // namespace openpower
