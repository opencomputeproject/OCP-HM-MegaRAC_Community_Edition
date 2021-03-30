#pragma once

#include "types.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace precondition
{

/**
 * @brief A precondition to compare a group of property values and
 * subscribe/unsubscribe a set speed event group
 * @details Compares each entry within the precondition group to a given value
 * that when each entry's property value matches the given value, the set speed
 * event is then initialized. At any point a precondition entry's value no
 * longer matches, the set speed event is removed from being active and fans
 * are set to full speed.
 *
 * @param[in] pg - Precondition property group of property values
 * @param[in] sse - Set speed event definition
 *
 * @return Lambda function
 *     A lambda function to compare precondition property value states
 *     and either subscribe or unsubscribe a set speed event group.
 */
Action property_states_match(std::vector<PrecondGroup>&& pg,
                             std::vector<SetSpeedEvent>&& sse);

/**
 * @brief A precondition to determine if there are any missing owners
 * of the services for the group to init/remove a set speed event
 * @details Checks each service associated with a group has an owner and
 * if any of the services are missing an owner, the precondition passes
 * and the events are initialized. Once all services associated with a
 * group have an owner, the events are removed from being active.
 *
 * @param[in] sse - Set speed event definitions
 *
 * @return Lambda function
 *     A lambda function precondition to check for group member services
 *     that are not owned to either initialize or remove set speed events.
 */
Action services_missing_owner(std::vector<SetSpeedEvent>&& sse);

} // namespace precondition
} // namespace control
} // namespace fan
} // namespace phosphor
