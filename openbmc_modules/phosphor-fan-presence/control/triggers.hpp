#pragma once

#include "types.hpp"
#include "zone.hpp"

namespace phosphor
{
namespace fan
{
namespace control
{
namespace trigger
{

/**
 * @brief A trigger to start a timer for an event
 * @details Creates and starts a timer according to the configuration given
 * that will call an event's actions upon each timer expiration.
 *
 * @param[in] tConf - Timer configuration parameters
 *
 * @return Trigger lambda function
 *     A Trigger function that creates and starts a timer
 */
Trigger timer(TimerConf&& tConf);

/**
 * @brief A trigger of a signal for an event
 * @details Subscribes to the defined signal match.
 *
 * @param[in] match - Signal match to subscribe to
 * @param[in] handler - Handler function for the received signal
 *
 * @return Trigger lambda function
 *     A Trigger function that subscribes to a signal
 */
Trigger signal(const std::string& match, SignalHandler&& handler);

/**
 * @brief A trigger for actions to run at event init
 * @details Runs the event actions when the event is initialized. An event
 * is initialized at application start or each time an event's precondition
 * transitions to a valid state.
 *
 * @param[in] handler - Handler function to use for event init
 *
 * @return Trigger lambda function
 *     A Trigger function that runs actions at event init
 */
Trigger init(MethodHandler&& handler = nullptr);

} // namespace trigger
} // namespace control
} // namespace fan
} // namespace phosphor
