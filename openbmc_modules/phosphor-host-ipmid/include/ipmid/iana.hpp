#pragma once

#include <cstdint>

namespace oem
{
using Number = std::uint32_t; // smallest standard size >= 24.

/*
 * This is the OpenBMC IANA OEM Number
 */
constexpr Number obmcOemNumber = 49871;

/*
 * This is the Google IANA OEM Number
 */
constexpr Number googOemNumber = 11129;

} // namespace oem
