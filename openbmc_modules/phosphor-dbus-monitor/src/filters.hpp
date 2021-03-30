#pragma once

#include "data_types.hpp"

#include <algorithm>
#include <functional>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @class Filters
 *  @brief Filter interface
 *
 *  Filters of any type can be applied to property value changes.
 */
class Filters
{
  public:
    Filters() = default;
    Filters(const Filters&) = delete;
    Filters(Filters&&) = default;
    Filters& operator=(const Filters&) = delete;
    Filters& operator=(Filters&&) = default;
    virtual ~Filters() = default;

    /** @brief Apply filter operations to a property value. */
    virtual bool operator()(const any_ns::any& value) = 0;
};

/** @class OperandFilters
 *  @brief Filter property values utilzing operand based functions.
 *
 *  When configured, an operand filter is applied to a property value each
 *  time it changes to determine if that property value should be filtered
 *  from being stored or used within a given callback function.
 */
template <typename T>
class OperandFilters : public Filters
{
  public:
    OperandFilters() = delete;
    OperandFilters(const OperandFilters&) = delete;
    OperandFilters(OperandFilters&&) = default;
    OperandFilters& operator=(const OperandFilters&) = delete;
    OperandFilters& operator=(OperandFilters&&) = default;
    virtual ~OperandFilters() = default;
    explicit OperandFilters(const std::vector<std::function<bool(T)>>& _ops) :
        Filters(), ops(std::move(_ops))
    {
    }

    bool operator()(const any_ns::any& value) override
    {
        for (const auto& filterOps : ops)
        {
            try
            {
                // Apply filter operand to property value
                if (!filterOps(any_ns::any_cast<T>(value)))
                {
                    // Property value should be filtered
                    return true;
                }
            }
            catch (const any_ns::bad_any_cast& bac)
            {
                // Unable to cast property value to filter value type
                // to check filter, continue to next filter op
                continue;
            }
        }

        // Property value should not be filtered
        return false;
    }

  private:
    /** @brief List of operand based filter functions. */
    const std::vector<std::function<bool(T)>> ops;
};

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
