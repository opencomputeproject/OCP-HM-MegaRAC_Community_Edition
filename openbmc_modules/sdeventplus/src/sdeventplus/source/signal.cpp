#include <memory>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/types.hpp>
#include <type_traits>
#include <utility>

namespace sdeventplus
{
namespace source
{

Signal::Signal(const Event& event, int sig, Callback&& callback) :
    Base(event, create_source(event, sig), std::false_type())
{
    set_userdata(
        std::make_unique<detail::SignalData>(*this, std::move(callback)));
}

Signal::Signal(const Signal& other, sdeventplus::internal::NoOwn) :
    Base(other, sdeventplus::internal::NoOwn())
{
}

void Signal::set_callback(Callback&& callback)
{
    get_userdata().callback = std::move(callback);
}

int Signal::get_signal() const
{
    return internal::callCheck("sd_event_source_get_signal",
                               &internal::SdEvent::sd_event_source_get_signal,
                               event.getSdEvent(), get());
}

detail::SignalData& Signal::get_userdata() const
{
    return static_cast<detail::SignalData&>(Base::get_userdata());
}

Signal::Callback& Signal::get_callback()
{
    return get_userdata().callback;
}

sd_event_source* Signal::create_source(const Event& event, int sig)
{
    sd_event_source* source;
    internal::callCheck(
        "sd_event_add_signal", &internal::SdEvent::sd_event_add_signal,
        event.getSdEvent(), event.get(), &source, sig, signalCallback, nullptr);
    return source;
}

int Signal::signalCallback(sd_event_source* source,
                           const struct signalfd_siginfo* si, void* userdata)
{
    return sourceCallback<Callback, detail::SignalData, &Signal::get_callback>(
        "signalCallback", source, userdata, si);
}

namespace detail
{

SignalData::SignalData(const Signal& base, Signal::Callback&& callback) :
    Signal(base, sdeventplus::internal::NoOwn()), BaseData(base),
    callback(std::move(callback))
{
}

} // namespace detail

} // namespace source
} // namespace sdeventplus
