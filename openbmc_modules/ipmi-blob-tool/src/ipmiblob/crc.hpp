#pragma once

#include <cstdint>
#include <vector>

namespace ipmiblob
{

/**
 * Generate the CRC for a payload (really any bytes).
 *
 * This is meant to only be called on the payload and not the CRC or the OEM
 * header, etc.
 *
 * @param[in] data - the bytes against to run the CRC
 * @return the CRC value
 */
std::uint16_t generateCrc(const std::vector<std::uint8_t>& data);

} // namespace ipmiblob
