#include <functional>
#include <sdeventplus/event.hpp>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <systemd/sd-event.h>
#include <type_traits>
#include <utility>

namespace sdeventplus
{

Event::Event(sd_event* event, const internal::SdEvent* sdevent) :
    sdevent(sdevent), event(event, sdevent, true)
{
}

Event::Event(sd_event* event, std::false_type,
             const internal::SdEvent* sdevent) :
    sdevent(sdevent),
    event(std::move(event), sdevent, true)
{
}

Event::Event(const Event& other, sdeventplus::internal::NoOwn) :
    sdevent(other.sdevent), event(other.get(), other.getSdEvent(), false)
{
}

Event Event::get_new(const internal::SdEvent* sdevent)
{
    sd_event* event = nullptr;
    internal::callCheck("sd_event_new", &internal::SdEvent::sd_event_new,
                        sdevent, &event);
    return Event(event, std::false_type(), sdevent);
}

Event Event::get_default(const internal::SdEvent* sdevent)
{
    sd_event* event = nullptr;
    internal::callCheck("sd_event_default",
                        &internal::SdEvent::sd_event_default, sdevent, &event);
    return Event(event, std::false_type(), sdevent);
}

sd_event* Event::get() const
{
    return event.value();
}

const internal::SdEvent* Event::getSdEvent() const
{
    return sdevent;
}

int Event::prepare() const
{
    return internal::callCheck("sd_event_prepare",
                               &internal::SdEvent::sd_event_prepare, sdevent,
                               get());
}

int Event::wait(MaybeTimeout timeout) const
{
    // An unsigned -1 timeout value means infinity in sd_event
    uint64_t timeout_usec = timeout ? timeout->count() : -1;
    return internal::callCheck("sd_event_wait",
                               &internal::SdEvent::sd_event_wait, sdevent,
                               get(), timeout_usec);
}

int Event::dispatch() const
{
    return internal::callCheck("sd_event_dispatch",
                               &internal::SdEvent::sd_event_dispatch, sdevent,
                               get());
}

int Event::run(MaybeTimeout timeout) const
{
    // An unsigned -1 timeout value means infinity in sd_event
    uint64_t timeout_usec = timeout ? timeout->count() : -1;
    return internal::callCheck("sd_event_run", &internal::SdEvent::sd_event_run,
                               sdevent, get(), timeout_usec);
}

int Event::loop() const
{
    return internal::callCheck(
        "sd_event_loop", &internal::SdEvent::sd_event_loop, sdevent, get());
}

void Event::exit(int code) const
{
    internal::callCheck("sd_event_exit", &internal::SdEvent::sd_event_exit,
                        sdevent, get(), code);
}

int Event::get_exit_code() const
{
    int code;
    internal::callCheck("sd_event_get_exit_code",
                        &internal::SdEvent::sd_event_get_exit_code, sdevent,
                        get(), &code);
    return code;
}

bool Event::get_watchdog() const
{
    return internal::callCheck("sd_event_get_watchdog",
                               &internal::SdEvent::sd_event_get_watchdog,
                               sdevent, get());
}

bool Event::set_watchdog(bool b) const
{
    return internal::callCheck("sd_event_set_watchdog",
                               &internal::SdEvent::sd_event_set_watchdog,
                               sdevent, get(), b);
}

sd_event* Event::ref(sd_event* const& event, const internal::SdEvent*& sdevent,
                     bool& owned)
{
    owned = true;
    return sdevent->sd_event_ref(event);
}

void Event::drop(sd_event*&& event, const internal::SdEvent*& sdevent,
                 bool& owned)
{
    if (owned)
    {
        sdevent->sd_event_unref(event);
    }
}

} // namespace sdeventplus
