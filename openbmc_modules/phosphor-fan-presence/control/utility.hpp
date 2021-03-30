#pragma once

#include "types.hpp"

#include <vector>

namespace phosphor
{
namespace fan
{
namespace control
{
namespace utility
{

/**
 * @brief A utility function to return a median value
 * @details A median value is determined from a set of values where the middle
 * value is returned from an odd set of values and an average of the middle
 * two values for an even set of values.
 *
 * @param[in] values - Set of values to determine the median from
 *
 * @return A median value
 *
 * @throw std::out_of_range Empty list of values given
 *
 * Note: The set of values will be partially re-ordered
 * https://en.cppreference.com/w/cpp/algorithm/nth_element
 */
int64_t getMedian(std::vector<int64_t>& values);

} // namespace utility
} // namespace control
} // namespace fan
} // namespace phosphor
