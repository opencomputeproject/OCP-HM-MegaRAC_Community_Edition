#pragma once

#include "callback.hpp"
#include "data_types.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class CountCondition
 *  @brief Count properties that satisfy a condition.
 *
 *  When invoked, a count class instance performs its condition
 *  test in two passes.
 *
 *  In pass one, apply a C++ relational operator to the value of
 *  each property in the index and a value provided by the
 *  configuration file.
 *
 *  Count the number of properties that pass the test in pass
 *  one.  In pass two, apply a second C++ relational operator
 *  to the number of properties that pass the test from pass one
 *  to a count provided by the configuration file.
 *
 *  If the oneshot parameter is true, then this condition won't pass
 *  again until it fails at least once.
 */
template <typename T>
class CountCondition : public IndexedConditional
{
  public:
    CountCondition() = delete;
    CountCondition(const CountCondition&) = default;
    CountCondition(CountCondition&&) = default;
    CountCondition& operator=(const CountCondition&) = default;
    CountCondition& operator=(CountCondition&&) = default;
    ~CountCondition() = default;

    CountCondition(const PropertyIndex& conditionIndex,
                   const std::function<bool(size_t)>& _countOp,
                   const std::function<bool(T)>& _propertyOp,
                   bool oneshot = false) :
        IndexedConditional(conditionIndex),
        countOp(_countOp), propertyOp(_propertyOp), oneshot(oneshot)
    {
    }

    bool operator()() override
    {
        // Count the number of properties in the index that
        // pass the condition specified in the config file.
        auto count = std::count_if(
            index.cbegin(), index.cend(),
            [this](const auto& item)
            // *INDENT-OFF*
            {
                // Get the property value from storage[0],
                // and save the op result in storage[1].
                const auto& storage = std::get<storageIndex>(item.second);
                // Don't count properties that don't exist.
                if (std::get<valueIndex>(storage.get()).empty())
                {
                    return false;
                }
                const auto& value =
                    any_ns::any_cast<T>(std::get<valueIndex>(storage.get()));
                auto r = propertyOp(value);

                std::get<resultIndex>(storage.get()) = r;

                return r;
            });
        // *INDENT-ON*

        // Now apply the count condition to the count.
        auto result = countOp(count);

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
    /** @brief The comparison to perform on the count. */
    std::function<bool(size_t)> countOp;
    /** @brief The comparison to perform on each property. */
    std::function<bool(T)> propertyOp;
    /** @brief If the condition can be allowed to pass again
               on subsequent checks that are also true. */
    const bool oneshot;
    /** @brief The result of the previous check. */
    bool lastResult = false;
};
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
