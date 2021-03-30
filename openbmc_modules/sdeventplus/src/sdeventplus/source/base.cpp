#include <functional>
#include <sdeventplus/internal/sdevent.hpp>
#include <sdeventplus/internal/utils.hpp>
#include <sdeventplus/source/base.hpp>
#include <sdeventplus/types.hpp>
#include <utility>

namespace sdeventplus
{
namespace source
{

sd_event_source* Base::get() const
{
    return source.value();
}

const Event& Base::get_event() const
{
    return event;
}

const char* Base::get_description() const
{
    const char* description;
    internal::callCheck("sd_event_source_get_description",
                        &internal::SdEvent::sd_event_source_get_description,
                        event.getSdEvent(), get(), &description);
    return description;
}

void Base::set_description(const char* description) const
{
    internal::callCheck("sd_event_source_set_description",
                        &internal::SdEvent::sd_event_source_set_description,
                        event.getSdEvent(), get(), description);
}

void Base::set_prepare(Callback&& callback)
{
    try
    {
        internal::callCheck("sd_event_source_set_prepare",
                            &internal::SdEvent::sd_event_source_set_prepare,
                            event.getSdEvent(), get(),
                            callback ? prepareCallback : nullptr);
        get_userdata().prepare = std::move(callback);
    }
    catch (...)
    {
        get_userdata().prepare = nullptr;
        throw;
    }
}

bool Base::get_pending() const
{
    return internal::callCheck("sd_event_source_get_pending",
                               &internal::SdEvent::sd_event_source_get_pending,
                               event.getSdEvent(), get());
}

int64_t Base::get_priority() const
{
    int64_t priority;
    internal::callCheck("sd_event_source_get_priority",
                        &internal::SdEvent::sd_event_source_get_priority,
                        event.getSdEvent(), get(), &priority);
    return priority;
}

void Base::set_priority(int64_t priority) const
{
    internal::callCheck("sd_event_source_set_priority",
                        &internal::SdEvent::sd_event_source_set_priority,
                        event.getSdEvent(), get(), priority);
}

Enabled Base::get_enabled() const
{
    int enabled;
    internal::callCheck("sd_event_source_get_enabled",
                        &internal::SdEvent::sd_event_source_get_enabled,
                        event.getSdEvent(), get(), &enabled);
    return static_cast<Enabled>(enabled);
}

void Base::set_enabled(Enabled enabled) const
{
    internal::callCheck("sd_event_source_set_enabled",
                        &internal::SdEvent::sd_event_source_set_enabled,
                        event.getSdEvent(), get(), static_cast<int>(enabled));
}

bool Base::get_floating() const
{
    return internal::callCheck("sd_event_source_get_floating",
                               &internal::SdEvent::sd_event_source_get_floating,
                               event.getSdEvent(), get());
}

void Base::set_floating(bool b) const
{
    internal::callCheck("sd_event_source_set_floating",
                        &internal::SdEvent::sd_event_source_set_floating,
                        event.getSdEvent(), get(), static_cast<int>(b));
}

Base::Base(const Event& event, sd_event_source* source, std::false_type) :
    event(event), source(std::move(source), event.getSdEvent(), true)
{
}

Base::Base(const Base& other, sdeventplus::internal::NoOwn) :
    event(other.get_event(), sdeventplus::internal::NoOwn()),
    source(other.get(), event.getSdEvent(), false)
{
}

void Base::set_userdata(std::unique_ptr<detail::BaseData> data) const
{
    internal::callCheck(
        "sd_event_source_set_destroy_callback",
        &internal::SdEvent::sd_event_source_set_destroy_callback,
        event.getSdEvent(), get(), &Base::destroy_userdata);
    event.getSdEvent()->sd_event_source_set_userdata(get(), data.release());
}

detail::BaseData& Base::get_userdata() const
{
    return *reinterpret_cast<detail::BaseData*>(
        event.getSdEvent()->sd_event_source_get_userdata(get()));
}

Base::Callback& Base::get_prepare()
{
    return get_userdata().prepare;
}

sd_event_source* Base::ref(sd_event_source* const& source,
                           const internal::SdEvent*& sdevent, bool& owned)
{
    owned = true;
    return sdevent->sd_event_source_ref(source);
}

void Base::drop(sd_event_source*&& source, const internal::SdEvent*& sdevent,
                bool& owned)
{
    if (owned)
    {
        sdevent->sd_event_source_unref(source);
    }
}

void Base::destroy_userdata(void* userdata)
{
    delete static_cast<Base*>(userdata);
}

int Base::prepareCallback(sd_event_source* source, void* userdata)
{
    return sourceCallback<Callback, Base, &Base::get_prepare>("prepareCallback",
                                                              source, userdata);
}

namespace detail
{

BaseData::BaseData(const Base& base) :
    Base(base, sdeventplus::internal::NoOwn())
{
}

} // namespace detail

} // namespace source
} // namespace sdeventplus
