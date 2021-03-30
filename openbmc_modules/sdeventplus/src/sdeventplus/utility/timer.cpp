#include <memory>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/types.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <stdexcept>
#include <utility>

namespace sdeventplus
{
namespace utility
{

template <ClockId Id>
Timer<Id>::Timer(const Event& event, Callback&& callback,
                 std::optional<Duration> interval,
                 typename source::Time<Id>::Accuracy accuracy) :
    userdata(nullptr),
    timeSource(event,
               Clock<Id>(event).now() + interval.value_or(Duration::zero()),
               accuracy, nullptr)
{
    auto timerData = std::make_unique<detail::TimerData<Id>>(
        *this, std::move(callback), interval);
    userdata = timerData.get();
    timerData->userdata = timerData.get();
    auto cb = [timerData = std::move(timerData)](
                  source::Time<Id>&, typename source::Time<Id>::TimePoint) {
        timerData->internalCallback();
    };
    timeSource.set_callback(std::move(cb));
    setEnabled(interval.has_value());
}

template <ClockId Id>
Timer<Id>::Timer(const Timer<Id>& other, sdeventplus::internal::NoOwn) :
    userdata(other.userdata),
    timeSource(other.timeSource, sdeventplus::internal::NoOwn())
{
}

template <ClockId Id>
void Timer<Id>::set_callback(Callback&& callback)
{
    userdata->callback = std::move(callback);
}

template <ClockId Id>
const Event& Timer<Id>::get_event() const
{
    return timeSource.get_event();
}

template <ClockId Id>
bool Timer<Id>::get_floating() const
{
    return timeSource.get_floating();
}

template <ClockId Id>
void Timer<Id>::set_floating(bool b)
{
    return timeSource.set_floating(b);
}

template <ClockId Id>
bool Timer<Id>::hasExpired() const
{
    return userdata->expired;
}

template <ClockId Id>
bool Timer<Id>::isEnabled() const
{
    return timeSource.get_enabled() != source::Enabled::Off;
}

template <ClockId Id>
std::optional<typename Timer<Id>::Duration> Timer<Id>::getInterval() const
{
    return userdata->interval;
}

template <ClockId Id>
typename Timer<Id>::Duration Timer<Id>::getRemaining() const
{
    if (!isEnabled())
    {
        throw std::runtime_error("Timer not running");
    }

    auto end = timeSource.get_time();
    auto now = userdata->clock.now();
    if (end < now)
    {
        return Duration{0};
    }
    return end - now;
}

template <ClockId Id>
void Timer<Id>::setEnabled(bool enabled)
{
    if (enabled && !userdata->initialized)
    {
        throw std::runtime_error("Timer was never initialized");
    }
    timeSource.set_enabled(enabled ? source::Enabled::On
                                   : source::Enabled::Off);
}

template <ClockId Id>
void Timer<Id>::setRemaining(Duration remaining)
{
    timeSource.set_time(userdata->clock.now() + remaining);
    userdata->initialized = true;
}

template <ClockId Id>
void Timer<Id>::resetRemaining()
{
    setRemaining(userdata->interval.value());
}

template <ClockId Id>
void Timer<Id>::setInterval(std::optional<Duration> interval)
{
    userdata->interval = interval;
}

template <ClockId Id>
void Timer<Id>::clearExpired()
{
    userdata->expired = false;
}

template <ClockId Id>
void Timer<Id>::restart(std::optional<Duration> interval)
{
    clearExpired();
    userdata->initialized = false;
    setInterval(interval);
    if (userdata->interval)
    {
        resetRemaining();
    }
    setEnabled(userdata->interval.has_value());
}

template <ClockId Id>
void Timer<Id>::restartOnce(Duration remaining)
{
    clearExpired();
    userdata->initialized = false;
    setInterval(std::nullopt);
    setRemaining(remaining);
    setEnabled(true);
}

template <ClockId Id>
void Timer<Id>::internalCallback()
{
    userdata->expired = true;
    userdata->initialized = false;
    if (userdata->interval)
    {
        resetRemaining();
    }
    else
    {
        setEnabled(false);
    }

    if (userdata->callback)
    {
        userdata->callback(*userdata);
    }
}

template class Timer<ClockId::RealTime>;
template class Timer<ClockId::Monotonic>;
template class Timer<ClockId::BootTime>;
template class Timer<ClockId::RealTimeAlarm>;
template class Timer<ClockId::BootTimeAlarm>;

namespace detail
{

template <ClockId Id>
TimerData<Id>::TimerData(const Timer<Id>& base,
                         typename Timer<Id>::Callback&& callback,
                         std::optional<typename Timer<Id>::Duration> interval) :
    Timer<Id>(base, sdeventplus::internal::NoOwn()),
    expired(false), initialized(interval.has_value()),
    callback(std::move(callback)),
    clock(Event(base.timeSource.get_event(), sdeventplus::internal::NoOwn())),
    interval(interval)
{
}

} // namespace detail

} // namespace utility
} // namespace sdeventplus
