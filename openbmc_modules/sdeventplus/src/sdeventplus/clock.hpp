#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <sdeventplus/event.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <type_traits>

namespace sdeventplus
{

/** @brief Specifies the underlying clock implementation
 */
enum class ClockId : clockid_t
{
    RealTime = CLOCK_REALTIME,
    Monotonic = CLOCK_MONOTONIC,
    BootTime = CLOCK_BOOTTIME,
    RealTimeAlarm = CLOCK_REALTIME_ALARM,
    BootTimeAlarm = CLOCK_BOOTTIME_ALARM,
};

/** @class Clock<Id>
 *  @brief Implements a Clock to be used with std::time_point
 *         Based on the underlying sd_event time functions
 */
template <ClockId Id>
class Clock
{
  public:
    /** @brief Types needed by chrono functions to store time data
     */
    using rep = SdEventDuration::rep;
    using period = SdEventDuration::period;
    using duration = SdEventDuration;
    using time_point = std::chrono::time_point<Clock>;
    static constexpr bool is_steady = Id == ClockId::Monotonic;

    /** @brief Constructs a new Clock with time data from the Event
     *
     * @param[in] event - The event used as the basis for the clock
     */
    Clock(const Event& event);
    Clock(Event&& event);

    /** @brief Gets the current time of the clock
     *
     * @throws SdEventError for underlying sd_event errors
     * @return The std::chrono::time_point representing the current time
     */
    time_point now() const;

  private:
    Event event;
};

} // namespace sdeventplus
