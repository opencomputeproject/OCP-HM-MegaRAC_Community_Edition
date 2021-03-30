#include <memory>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/types.hpp>
#include <utility>

namespace sdeventplus
{
namespace source
{

void EventBase::set_callback(Callback&& callback)
{
    get_userdata().callback = std::move(callback);
}

EventBase::EventBase(const EventBase& other, sdeventplus::internal::NoOwn) :
    Base(other, sdeventplus::internal::NoOwn())
{
}

EventBase::EventBase(const char* name, CreateFunc create, const Event& event,
                     Callback&& callback) :
    Base(event, create_source(name, create, event), std::false_type())
{
    set_userdata(
        std::make_unique<detail::EventBaseData>(*this, std::move(callback)));
}

detail::EventBaseData& EventBase::get_userdata() const
{
    return static_cast<detail::EventBaseData&>(Base::get_userdata());
}

EventBase::Callback& EventBase::get_callback()
{
    return get_userdata().callback;
}

sd_event_source* EventBase::create_source(const char* name, CreateFunc create,
                                          const Event& event)
{
    sd_event_source* source;
    internal::callCheck(name, create, event.getSdEvent(), event.get(), &source,
                        eventCallback, nullptr);
    return source;
}

int EventBase::eventCallback(sd_event_source* source, void* userdata)
{
    return sourceCallback<Callback, detail::EventBaseData,
                          &EventBase::get_callback>("eventCallback", source,
                                                    userdata);
}

namespace detail
{

EventBaseData::EventBaseData(const EventBase& base,
                             EventBase::Callback&& callback) :
    EventBase(base, sdeventplus::internal::NoOwn()),
    BaseData(base), callback(std::move(callback))
{
}

} // namespace detail

Defer::Defer(const Event& event, Callback&& callback) :
    EventBase("sd_event_add_defer", &internal::SdEvent::sd_event_add_defer,
              event, std::move(callback))
{
}

Post::Post(const Event& event, Callback&& callback) :
    EventBase("sd_event_add_post", &internal::SdEvent::sd_event_add_post, event,
              std::move(callback))
{
}

Exit::Exit(const Event& event, Callback&& callback) :
    EventBase("sd_event_add_exit", &internal::SdEvent::sd_event_add_exit, event,
              std::move(callback))
{
}

} // namespace source
} // namespace sdeventplus
