#include <memory>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/source/time.hpp>
#include <sdeventplus/types.hpp>
#include <type_traits>
#include <utility>

namespace sdeventplus
{
namespace source
{

template <ClockId Id>
Time<Id>::Time(const Event& event, TimePoint time, Accuracy accuracy,
               Callback&& callback) :
    Base(event, create_source(event, time, accuracy), std::false_type())
{
    set_userdata(
        std::make_unique<detail::TimeData<Id>>(*this, std::move(callback)));
}

template <ClockId Id>
Time<Id>::Time(const Time<Id>& other, sdeventplus::internal::NoOwn) :
    Base(other, sdeventplus::internal::NoOwn())
{
}

template <ClockId Id>
void Time<Id>::set_callback(Callback&& callback)
{
    get_userdata().callback = std::move(callback);
}

template <ClockId Id>
typename Time<Id>::TimePoint Time<Id>::get_time() const
{
    uint64_t usec;
    internal::callCheck("sd_event_source_get_time",
                        &internal::SdEvent::sd_event_source_get_time,
                        event.getSdEvent(), get(), &usec);
    return Time<Id>::TimePoint(SdEventDuration(usec));
}

template <ClockId Id>
void Time<Id>::set_time(TimePoint time) const
{
    internal::callCheck("sd_event_source_set_time",
                        &internal::SdEvent::sd_event_source_set_time,
                        event.getSdEvent(), get(),
                        SdEventDuration(time.time_since_epoch()).count());
}

template <ClockId Id>
typename Time<Id>::Accuracy Time<Id>::get_accuracy() const
{
    uint64_t usec;
    internal::callCheck("sd_event_source_get_time_accuracy",
                        &internal::SdEvent::sd_event_source_get_time_accuracy,
                        event.getSdEvent(), get(), &usec);
    return SdEventDuration(usec);
}

template <ClockId Id>
void Time<Id>::set_accuracy(Accuracy accuracy) const
{
    internal::callCheck("sd_event_source_set_time_accuracy",
                        &internal::SdEvent::sd_event_source_set_time_accuracy,
                        event.getSdEvent(), get(),
                        SdEventDuration(accuracy).count());
}

template <ClockId Id>
detail::TimeData<Id>& Time<Id>::get_userdata() const
{
    return static_cast<detail::TimeData<Id>&>(Base::get_userdata());
}

template <ClockId Id>
typename Time<Id>::Callback& Time<Id>::get_callback()
{
    return get_userdata().callback;
}

template <ClockId Id>
sd_event_source* Time<Id>::create_source(const Event& event, TimePoint time,
                                         Accuracy accuracy)
{
    sd_event_source* source;
    internal::callCheck(
        "sd_event_add_time", &internal::SdEvent::sd_event_add_time,
        event.getSdEvent(), event.get(), &source, static_cast<clockid_t>(Id),
        SdEventDuration(time.time_since_epoch()).count(),
        SdEventDuration(accuracy).count(), timeCallback, nullptr);
    return source;
}

template <ClockId Id>
int Time<Id>::timeCallback(sd_event_source* source, uint64_t usec,
                           void* userdata)
{
    return sourceCallback<Callback, detail::TimeData<Id>, &Time::get_callback>(
        "timeCallback", source, userdata, TimePoint(SdEventDuration(usec)));
}

template class Time<ClockId::RealTime>;
template class Time<ClockId::Monotonic>;
template class Time<ClockId::BootTime>;
template class Time<ClockId::RealTimeAlarm>;
template class Time<ClockId::BootTimeAlarm>;

namespace detail
{

template <ClockId Id>
TimeData<Id>::TimeData(const Time<Id>& base,
                       typename Time<Id>::Callback&& callback) :
    Time<Id>(base, sdeventplus::internal::NoOwn()),
    BaseData(base), callback(std::move(callback))
{
}

} // namespace detail

} // namespace source
} // namespace sdeventplus
