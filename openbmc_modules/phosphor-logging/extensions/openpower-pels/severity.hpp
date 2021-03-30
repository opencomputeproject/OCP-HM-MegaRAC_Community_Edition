#pragma once

#include "elog_entry.hpp"

namespace openpower
{
namespace pels
{

/**
 * @brief Convert an OpenBMC event log severity to a PEL severity
 *
 * @param[in] severity - The OpenBMC event log severity
 *
 * @return uint8_t - The PEL severity value
 */
uint8_t convertOBMCSeverityToPEL(phosphor::logging::Entry::Level severity);

} // namespace pels
} // namespace openpower
