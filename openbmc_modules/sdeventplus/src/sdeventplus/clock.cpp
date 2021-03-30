#include <sdeventplus/clock.hpp>
#include <sdeventplus/exception.hpp>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <utility>

namespace sdeventplus
{

template <ClockId Id>
Clock<Id>::Clock(const Event& event) : event(event)
{
}

template <ClockId Id>
Clock<Id>::Clock(Event&& event) : event(std::move(event))
{
}

template <ClockId Id>
typename Clock<Id>::time_point Clock<Id>::now() const
{
    uint64_t now;
    internal::callCheck("sd_event_now", &internal::SdEvent::sd_event_now,
                        event.getSdEvent(), event.get(),
                        static_cast<clockid_t>(Id), &now);
    return time_point(SdEventDuration(now));
}

template class Clock<ClockId::RealTime>;
template class Clock<ClockId::Monotonic>;
template class Clock<ClockId::BootTime>;
template class Clock<ClockId::RealTimeAlarm>;
template class Clock<ClockId::BootTimeAlarm>;

} // namespace sdeventplus
