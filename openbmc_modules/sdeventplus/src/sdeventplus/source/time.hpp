#pragma once

#include <cstdint>
#include <function2/function2.hpp>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/types.hpp>
#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace source
{

namespace detail
{
template <ClockId Id>
class TimeData;
} // namespace detail

/** @class Time<ClockId>
 *  @brief A wrapper around the sd_event_source time type
 *         See sd_event_add_time(3) for more information
 */
template <ClockId Id>
class Time : public Base
{
  public:
    /** @brief Type used as the basis for absolute clock times */
    using TimePoint = typename Clock<Id>::time_point;
    /** @brief Type used to define the accuracy of the time source */
    using Accuracy = SdEventDuration;
    /** @brief Type of the user provided callback function */
    using Callback = fu2::unique_function<void(Time& source, TimePoint time)>;

    /** @brief Creates a new time event source on the provided event loop
     *         This type of source defaults to Enabled::Oneshot, and needs to be
     *         reconfigured upon each callback.
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] time     - Absolute time when the callback should be executed
     *  @param[in] accuracy - Optional amount of error tolerable in time source
     *  @param[in] callback - The function executed on event dispatch
     *  @throws SdEventError for underlying sd_event errors
     */
    Time(const Event& event, TimePoint time, Accuracy accuracy,
         Callback&& callback);

    /** @brief Constructs a non-owning time source handler
     *         Does not own the passed reference to the source because
     *         this is meant to be used only as a reference inside an event
     *         source.
     *  @internal
     *
     *  @param[in] other - The source wrapper to copy
     *  @param[in]       - Signifies that this new copy is non-owning
     *  @throws SdEventError for underlying sd_event errors
     */
    Time(const Time& other, sdeventplus::internal::NoOwn);

    /** @brief Sets the callback
     *
     *  @param[in] callback - The function executed on event dispatch
     */
    void set_callback(Callback&& callback);

    /** @brief Gets the absolute time when the time source expires
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return Absolute time as an std::chrono::time_point
     */
    TimePoint get_time() const;

    /** @brief Sets the absolute time when the time source will expire
     *
     *  @param[in] time - Absolute time as an std::chrono::time_point
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_time(TimePoint time) const;

    /** @brief Gets the accuracy of the time source
     *
     *  @throws SdEventError for underlying sd_event errors
     *  @return Accuracy as an std::chrono::duration
     */
    Accuracy get_accuracy() const;

    /** @brief Sets the accuracy of the time source
     *
     *  @param[in] accuracy - Accuracy as std::chrono::duration
     *  @throws SdEventError for underlying sd_event errors
     */
    void set_accuracy(Accuracy accuracy) const;

  private:
    /** @brief Returns a reference to the source owned time
     *
     *  @return A reference to the time
     */
    detail::TimeData<Id>& get_userdata() const;

    /** @brief Returns a reference to the callback executed for this source
     *
     *  @return A reference to the callback
     */
    Callback& get_callback();

    /** @brief Creates a new time source attached to the Event
     *
     *  @param[in] event    - The event to attach the handler
     *  @param[in] time     - Absolute time when the callback should be executed
     *  @param[in] accuracy - Optional amount of error tolerable in time source
     *  @throws SdEventError for underlying sd_event errors
     *  @return A new sd_event_source
     */
    static sd_event_source* create_source(const Event& event, TimePoint time,
                                          Accuracy accuracy);

    /** @brief A wrapper around the callback that can be called from sd-event
     *
     *  @param[in] source   - The sd_event_source associated with the call
     *  @param[in] userdata - The provided userdata for the source
     *  @return 0 on success or a negative errno otherwise
     */
    static int timeCallback(sd_event_source* source, uint64_t usec,
                            void* userdata);
};

namespace detail
{

template <ClockId Id>
class TimeData : public Time<Id>, public BaseData
{
  private:
    typename Time<Id>::Callback callback;

  public:
    TimeData(const Time<Id>& base, typename Time<Id>::Callback&& callback);

    friend Time<Id>;
};

} // namespace detail

} // namespace source
} // namespace sdeventplus
