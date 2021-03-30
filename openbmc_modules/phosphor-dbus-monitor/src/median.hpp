#pragma once

#include "callback.hpp"
#include "data_types.hpp"

#include <algorithm>
#include <functional>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class MedianCondition
 *  @brief Determine a median from properties and apply a condition.
 *
 *  When invoked, a median class instance performs its condition
 *  test against a median value that has been determined from a set
 *  of configured properties.
 *
 *  Once the median value is determined, a C++ relational operator
 *  is applied to it and a value provided by the configuration file,
 *  which determines if the condition passes or not.
 *
 *  Where no property values configured are found to determine a median from,
 *  the condition defaults to `true` and passes.
 *
 *  If the oneshot parameter is true, then this condition won't pass
 *  again until it fails at least once.
 */
template <typename T>
class MedianCondition : public IndexedConditional
{
  public:
    MedianCondition() = delete;
    MedianCondition(const MedianCondition&) = default;
    MedianCondition(MedianCondition&&) = default;
    MedianCondition& operator=(const MedianCondition&) = default;
    MedianCondition& operator=(MedianCondition&&) = default;
    ~MedianCondition() = default;

    MedianCondition(const PropertyIndex& conditionIndex,
                    const std::function<bool(T)>& _medianOp,
                    bool oneshot = false) :
        IndexedConditional(conditionIndex),
        medianOp(_medianOp), oneshot(oneshot)
    {
    }

    bool operator()() override
    {
        // Default the condition result to true
        // if no property values are found to produce a median.
        auto result = true;
        std::vector<T> values;
        for (const auto& item : index)
        {
            const auto& storage = std::get<storageIndex>(item.second);
            // Don't count properties that don't exist.
            if (std::get<valueIndex>(storage.get()).empty())
            {
                continue;
            }
            values.emplace_back(
                any_ns::any_cast<T>(std::get<valueIndex>(storage.get())));
        }

        if (!values.empty())
        {
            auto median = values.front();
            // Get the determined median value
            if (values.size() == 2)
            {
                // For 2 values, use the highest instead of the average
                // for a worst case median value
                median = *std::max_element(values.begin(), values.end());
            }
            else if (values.size() > 2)
            {
                const auto oddIt = values.begin() + values.size() / 2;
                std::nth_element(values.begin(), oddIt, values.end());
                median = *oddIt;
                // Determine median for even number of values
                if (index.size() % 2 == 0)
                {
                    // Use average of middle 2 values for median
                    const auto evenIt = values.begin() + values.size() / 2 - 1;
                    std::nth_element(values.begin(), evenIt, values.end());
                    median = (median + *evenIt) / 2;
                }
            }

            // Now apply the condition to the median value.
            result = medianOp(median);
        }

        // If this was a oneshot and the the condition has already
        // passed, then don't let it pass again until the condition
        // has gone back to false.
        if (oneshot && result && lastResult)
        {
            return false;
        }

        lastResult = result;
        return result;
    }

  private:
    /** @brief The comparison to perform on the median value. */
    std::function<bool(T)> medianOp;
    /** @brief If the condition can be allowed to pass again
               on subsequent checks that are also true. */
    const bool oneshot;
    /** @brief The result of the previous check. */
    bool lastResult = false;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
