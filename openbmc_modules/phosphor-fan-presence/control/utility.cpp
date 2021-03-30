#include "utility.hpp"

#include <algorithm>
#include <stdexcept>

namespace phosphor
{
namespace fan
{
namespace control
{
namespace utility
{

int64_t getMedian(std::vector<int64_t>& values)
{
    if (values.empty())
    {
        throw std::out_of_range("getMedian(): Empty list of values");
    }
    const auto oddIt = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), oddIt, values.end());
    auto median = *oddIt;
    // Determine median for even number of values
    if (values.size() % 2 == 0)
    {
        // Use average of middle 2 values for median
        const auto evenIt = values.begin() + values.size() / 2 - 1;
        std::nth_element(values.begin(), evenIt, values.end());
        median = (median + *evenIt) / 2;
    }

    return median;
}

} // namespace utility
} // namespace control
} // namespace fan
} // namespace phosphor
