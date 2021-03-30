#pragma once

#include "types.hpp"

#include <nlohmann/json.hpp>

namespace phosphor
{
namespace fan
{
namespace monitor
{

using json = nlohmann::json;

/**
 * @brief Create a condition function object
 *
 * @param[in] condition - The condition being created
 *
 * @return - The created condition function object
 */
template <typename T>
auto make_condition(T&& condition)
{
    return Condition(std::forward<T>(condition));
}

namespace condition
{

/**
 * @brief A condition that checks all properties match the given values
 * @details Checks each property entry against its given value where all
 * property values must match their given value for the condition to pass
 *
 * @param[in] propStates - List of property identifiers and their value
 *
 * @return Condition lambda function
 *     A Condition function that checks all properties match
 */
Condition propertiesMatch(std::vector<PropertyState>&& propStates);

/**
 * @brief Parse the propertiesMatch condition's parameters from the given JSON
 * configuration
 * @details Parses and verifies all the required parameters are given in the
 * JSON configuration to construct and return a function pointer to the
 * propertiesMatch condition function.
 *
 * @param[in] condParams - JSON object containing all the propertiesMatch
 *                         condition parameters
 *
 * @return Condition lambda function
 *     The propertiesMatch condition function that checks all properties match
 */
Condition getPropertiesMatch(const json& condParams);

} // namespace condition
} // namespace monitor
} // namespace fan
} // namespace phosphor
