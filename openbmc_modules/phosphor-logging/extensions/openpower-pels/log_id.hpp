#pragma once

#include <cstdint>

namespace openpower
{
namespace pels
{

namespace detail
{

/**
 * @brief Adds the 1 byte log creator prefix to the log ID
 *
 * @param[in] id - the ID to add it to
 *
 * @return - the full log ID
 */
uint32_t addLogIDPrefix(uint32_t id);

/**
 * @brief Generates a PEL ID based on the current time.
 *
 * Used for error scenarios where the normal method doesn't
 * work in order to get a unique ID still.
 *
 * @return A unique log ID.
 */
uint32_t getTimeBasedLogID();

} // namespace detail

/**
 * @brief Generates a unique PEL log entry ID every time
 *        it is called.
 *
 * This ID is used at offset 0x2C in the Private Header
 * section of a PEL.  For single BMC systems, it must
 * start with 0x50.
 *
 * @return uint32_t - The log ID
 */
uint32_t generatePELID();

} // namespace pels
} // namespace openpower
