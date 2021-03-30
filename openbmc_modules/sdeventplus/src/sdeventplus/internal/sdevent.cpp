#include <sdeventplus/internal/sdevent.hpp>
#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace internal
{

int SdEventImpl::sd_event_default(sd_event** event) const
{
    return ::sd_event_default(event);
}

int SdEventImpl::sd_event_new(sd_event** event) const
{
    return ::sd_event_default(event);
}

sd_event* SdEventImpl::sd_event_ref(sd_event* event) const
{
    return ::sd_event_ref(event);
}

sd_event* SdEventImpl::sd_event_unref(sd_event* event) const
{
    return ::sd_event_unref(event);
}

int SdEventImpl::sd_event_add_io(sd_event* event, sd_event_source** source,
                                 int fd, uint32_t events,
                                 sd_event_io_handler_t callback,
                                 void* userdata) const
{
    return ::sd_event_add_io(event, source, fd, events, callback, userdata);
}

int SdEventImpl::sd_event_add_time(sd_event* event, sd_event_source** source,
                                   clockid_t clock, uint64_t usec,
                                   uint64_t accuracy,
                                   sd_event_time_handler_t callback,
                                   void* userdata) const
{
    return ::sd_event_add_time(event, source, clock, usec, accuracy, callback,
                               userdata);
}

int SdEventImpl::sd_event_add_defer(sd_event* event, sd_event_source** source,
                                    sd_event_handler_t callback,
                                    void* userdata) const
{
    return ::sd_event_add_defer(event, source, callback, userdata);
}

int SdEventImpl::sd_event_add_post(sd_event* event, sd_event_source** source,
                                   sd_event_handler_t callback,
                                   void* userdata) const
{
    return ::sd_event_add_post(event, source, callback, userdata);
}

int SdEventImpl::sd_event_add_exit(sd_event* event, sd_event_source** source,
                                   sd_event_handler_t callback,
                                   void* userdata) const
{
    return ::sd_event_add_exit(event, source, callback, userdata);
}

int SdEventImpl::sd_event_prepare(sd_event* event) const
{
    return ::sd_event_prepare(event);
}

int SdEventImpl::sd_event_wait(sd_event* event, uint64_t usec) const
{
    return ::sd_event_wait(event, usec);
}

int SdEventImpl::sd_event_dispatch(sd_event* event) const
{
    return ::sd_event_dispatch(event);
}

int SdEventImpl::sd_event_run(sd_event* event, uint64_t usec) const
{
    return ::sd_event_run(event, usec);
}

int SdEventImpl::sd_event_loop(sd_event* event) const
{
    return ::sd_event_loop(event);
}

int SdEventImpl::sd_event_exit(sd_event* event, int code) const
{
    return ::sd_event_exit(event, code);
}

int SdEventImpl::sd_event_now(sd_event* event, clockid_t clock,
                              uint64_t* usec) const
{
    return ::sd_event_now(event, clock, usec);
}

int SdEventImpl::sd_event_get_exit_code(sd_event* event, int* code) const
{
    return ::sd_event_get_exit_code(event, code);
}

int SdEventImpl::sd_event_get_watchdog(sd_event* event) const
{
    return ::sd_event_get_watchdog(event);
}

int SdEventImpl::sd_event_set_watchdog(sd_event* event, int b) const
{
    return ::sd_event_set_watchdog(event, b);
}

sd_event_source* SdEventImpl::sd_event_source_ref(sd_event_source* source) const
{
    return ::sd_event_source_ref(source);
}

sd_event_source*
    SdEventImpl::sd_event_source_unref(sd_event_source* source) const
{
    return ::sd_event_source_unref(source);
}

void* SdEventImpl::sd_event_source_get_userdata(sd_event_source* source) const
{
    return ::sd_event_source_get_userdata(source);
}

void* SdEventImpl::sd_event_source_set_userdata(sd_event_source* source,
                                                void* userdata) const
{
    return ::sd_event_source_set_userdata(source, userdata);
}

int SdEventImpl::sd_event_source_get_description(sd_event_source* source,
                                                 const char** description) const
{
    return ::sd_event_source_get_description(source, description);
}

int SdEventImpl::sd_event_source_set_description(sd_event_source* source,
                                                 const char* description) const
{
    return ::sd_event_source_set_description(source, description);
}

int SdEventImpl::sd_event_source_set_prepare(sd_event_source* source,
                                             sd_event_handler_t callback) const
{
    return ::sd_event_source_set_prepare(source, callback);
}

int SdEventImpl::sd_event_source_get_pending(sd_event_source* source) const
{
    return ::sd_event_source_get_pending(source);
}

int SdEventImpl::sd_event_source_get_priority(sd_event_source* source,
                                              int64_t* priority) const
{
    return ::sd_event_source_get_priority(source, priority);
}

int SdEventImpl::sd_event_source_set_priority(sd_event_source* source,
                                              int64_t priority) const
{
    return ::sd_event_source_set_priority(source, priority);
}

int SdEventImpl::sd_event_source_get_enabled(sd_event_source* source,
                                             int* enabled) const
{
    return ::sd_event_source_get_enabled(source, enabled);
}

int SdEventImpl::sd_event_source_set_enabled(sd_event_source* source,
                                             int enabled) const
{
    return ::sd_event_source_set_enabled(source, enabled);
}

int SdEventImpl::sd_event_source_get_io_fd(sd_event_source* source) const
{
    return ::sd_event_source_get_io_fd(source);
}

int SdEventImpl::sd_event_source_set_io_fd(sd_event_source* source,
                                           int fd) const
{
    return ::sd_event_source_set_io_fd(source, fd);
}

int SdEventImpl::sd_event_source_get_io_events(sd_event_source* source,
                                               uint32_t* events) const
{
    return ::sd_event_source_get_io_events(source, events);
}

int SdEventImpl::sd_event_source_set_io_events(sd_event_source* source,
                                               uint32_t events) const
{
    return ::sd_event_source_set_io_events(source, events);
}

int SdEventImpl::sd_event_source_get_io_revents(sd_event_source* source,
                                                uint32_t* revents) const
{
    return ::sd_event_source_get_io_revents(source, revents);
}

int SdEventImpl::sd_event_source_get_time(sd_event_source* source,
                                          uint64_t* usec) const
{
    return ::sd_event_source_get_time(source, usec);
}

int SdEventImpl::sd_event_source_set_time(sd_event_source* source,
                                          uint64_t usec) const
{
    return ::sd_event_source_set_time(source, usec);
}

int SdEventImpl::sd_event_source_get_time_accuracy(sd_event_source* source,
                                                   uint64_t* usec) const
{
    return ::sd_event_source_get_time_accuracy(source, usec);
}

int SdEventImpl::sd_event_source_set_time_accuracy(sd_event_source* source,
                                                   uint64_t usec) const
{
    return ::sd_event_source_set_time_accuracy(source, usec);
}

int SdEventImpl::sd_event_source_get_signal(sd_event_source* source) const
{
    return ::sd_event_source_get_signal(source);
}

int SdEventImpl::sd_event_source_get_child_pid(sd_event_source* source,
                                               pid_t* pid) const
{
    return ::sd_event_source_get_child_pid(source, pid);
}

int SdEventImpl::sd_event_source_set_destroy_callback(
    sd_event_source* source, sd_event_destroy_t callback) const
{
    return ::sd_event_source_set_destroy_callback(source, callback);
}

int SdEventImpl::sd_event_source_get_destroy_callback(
    sd_event_source* source, sd_event_destroy_t* callback) const
{
    return ::sd_event_source_get_destroy_callback(source, callback);
}

int SdEventImpl::sd_event_source_set_floating(sd_event_source* source,
                                              int b) const
{
    return ::sd_event_source_set_floating(source, b);
}

int SdEventImpl::sd_event_source_get_floating(sd_event_source* source) const
{
    return ::sd_event_source_get_floating(source);
}

SdEventImpl sdevent_impl;

} // namespace internal
} // namespace sdeventplus
