#pragma once

#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace internal
{

/** @class SdEvent
 *  @brief Overridable direct sd_event interface
 */
class SdEvent
{
  public:
    virtual ~SdEvent() = default;

    virtual int sd_event_default(sd_event** event) const = 0;
    virtual int sd_event_new(sd_event** event) const = 0;
    virtual sd_event* sd_event_ref(sd_event* event) const = 0;
    virtual sd_event* sd_event_unref(sd_event* event) const = 0;

    virtual int sd_event_add_io(sd_event* event, sd_event_source** source,
                                int fd, uint32_t events, sd_event_io_handler_t,
                                void* userdata) const = 0;
    virtual int sd_event_add_time(sd_event* event, sd_event_source** source,
                                  clockid_t clock, uint64_t usec,
                                  uint64_t accuracy,
                                  sd_event_time_handler_t callback,
                                  void* userdata) const = 0;
    virtual int sd_event_add_signal(sd_event* event, sd_event_source** source,
                                    int sig, sd_event_signal_handler_t callback,
                                    void* userdata) const = 0;
    virtual int sd_event_add_child(sd_event* event, sd_event_source** source,
                                   pid_t, int options,
                                   sd_event_child_handler_t callback,
                                   void* userdata) const = 0;
    virtual int sd_event_add_defer(sd_event* event, sd_event_source** source,
                                   sd_event_handler_t callback,
                                   void* userdata) const = 0;
    virtual int sd_event_add_post(sd_event* event, sd_event_source** source,
                                  sd_event_handler_t callback,
                                  void* userdata) const = 0;
    virtual int sd_event_add_exit(sd_event* event, sd_event_source** source,
                                  sd_event_handler_t callback,
                                  void* userdata) const = 0;

    virtual int sd_event_prepare(sd_event* event) const = 0;
    virtual int sd_event_wait(sd_event* event, uint64_t usec) const = 0;
    virtual int sd_event_dispatch(sd_event* event) const = 0;
    virtual int sd_event_run(sd_event* event, uint64_t usec) const = 0;
    virtual int sd_event_loop(sd_event* event) const = 0;
    virtual int sd_event_exit(sd_event* event, int code) const = 0;

    virtual int sd_event_now(sd_event* event, clockid_t clock,
                             uint64_t* usec) const = 0;

    virtual int sd_event_get_exit_code(sd_event* event, int* code) const = 0;
    virtual int sd_event_get_watchdog(sd_event* event) const = 0;
    virtual int sd_event_set_watchdog(sd_event* event, int b) const = 0;

    virtual sd_event_source*
        sd_event_source_ref(sd_event_source* source) const = 0;
    virtual sd_event_source*
        sd_event_source_unref(sd_event_source* source) const = 0;

    virtual void*
        sd_event_source_get_userdata(sd_event_source* source) const = 0;
    virtual void* sd_event_source_set_userdata(sd_event_source* source,
                                               void* userdata) const = 0;

    virtual int
        sd_event_source_get_description(sd_event_source* source,
                                        const char** description) const = 0;
    virtual int
        sd_event_source_set_description(sd_event_source* source,
                                        const char* description) const = 0;
    virtual int
        sd_event_source_set_prepare(sd_event_source* source,
                                    sd_event_handler_t callback) const = 0;
    virtual int sd_event_source_get_pending(sd_event_source* source) const = 0;
    virtual int sd_event_source_get_priority(sd_event_source* source,
                                             int64_t* priority) const = 0;
    virtual int sd_event_source_set_priority(sd_event_source* source,
                                             int64_t priority) const = 0;
    virtual int sd_event_source_get_enabled(sd_event_source* source,
                                            int* enabled) const = 0;
    virtual int sd_event_source_set_enabled(sd_event_source* source,
                                            int enabled) const = 0;
    virtual int sd_event_source_get_io_fd(sd_event_source* source) const = 0;
    virtual int sd_event_source_set_io_fd(sd_event_source* source,
                                          int fd) const = 0;
    virtual int sd_event_source_get_io_events(sd_event_source* source,
                                              uint32_t* events) const = 0;
    virtual int sd_event_source_set_io_events(sd_event_source* source,
                                              uint32_t events) const = 0;
    virtual int sd_event_source_get_io_revents(sd_event_source* source,
                                               uint32_t* revents) const = 0;
    virtual int sd_event_source_get_time(sd_event_source* source,
                                         uint64_t* usec) const = 0;
    virtual int sd_event_source_set_time(sd_event_source* source,
                                         uint64_t usec) const = 0;
    virtual int sd_event_source_get_time_accuracy(sd_event_source* source,
                                                  uint64_t* usec) const = 0;
    virtual int sd_event_source_set_time_accuracy(sd_event_source* source,
                                                  uint64_t usec) const = 0;
    virtual int sd_event_source_get_signal(sd_event_source* source) const = 0;
    virtual int sd_event_source_get_child_pid(sd_event_source* source,
                                              pid_t* pid) const = 0;
    virtual int sd_event_source_set_destroy_callback(
        sd_event_source* source, sd_event_destroy_t callback) const = 0;
    virtual int sd_event_source_get_destroy_callback(
        sd_event_source* source, sd_event_destroy_t* callback) const = 0;
    virtual int sd_event_source_set_floating(sd_event_source* source,
                                             int b) const = 0;
    virtual int sd_event_source_get_floating(sd_event_source* source) const = 0;
};

/** @class SdEventImpl
 *  @brief sd_event concrete implementation
 *  @details Uses libsystemd to handle all sd_event calls
 */
class SdEventImpl : public SdEvent
{
  public:
    int sd_event_default(sd_event** event) const override;
    int sd_event_new(sd_event** event) const override;
    sd_event* sd_event_ref(sd_event* event) const override;
    sd_event* sd_event_unref(sd_event* event) const override;

    int sd_event_add_io(sd_event* event, sd_event_source** source, int fd,
                        uint32_t events, sd_event_io_handler_t callback,
                        void* userdata) const override;
    int sd_event_add_time(sd_event* event, sd_event_source** source,
                          clockid_t clock, uint64_t usec, uint64_t accuracy,
                          sd_event_time_handler_t callback,
                          void* userdata) const override;

    int sd_event_add_signal(sd_event* event, sd_event_source** source, int sig,
                            sd_event_signal_handler_t callback,
                            void* userdata) const override
    {
        return ::sd_event_add_signal(event, source, sig, callback, userdata);
    }

    int sd_event_add_child(sd_event* event, sd_event_source** source, pid_t pid,
                           int options, sd_event_child_handler_t callback,
                           void* userdata) const override
    {
        return ::sd_event_add_child(event, source, pid, options, callback,
                                    userdata);
    }

    int sd_event_add_defer(sd_event* event, sd_event_source** source,
                           sd_event_handler_t callback,
                           void* userdata) const override;
    int sd_event_add_post(sd_event* event, sd_event_source** source,
                          sd_event_handler_t callback,
                          void* userdata) const override;
    int sd_event_add_exit(sd_event* event, sd_event_source** source,
                          sd_event_handler_t callback,
                          void* userdata) const override;

    int sd_event_prepare(sd_event* event) const override;
    int sd_event_wait(sd_event* event, uint64_t usec) const override;
    int sd_event_dispatch(sd_event* event) const override;
    int sd_event_run(sd_event* event, uint64_t usec) const override;
    int sd_event_loop(sd_event* event) const override;
    int sd_event_exit(sd_event* event, int code) const override;

    int sd_event_now(sd_event* event, clockid_t clock,
                     uint64_t* usec) const override;

    int sd_event_get_exit_code(sd_event* event, int* code) const override;
    int sd_event_get_watchdog(sd_event* event) const override;
    int sd_event_set_watchdog(sd_event* event, int b) const override;

    sd_event_source*
        sd_event_source_ref(sd_event_source* source) const override;
    sd_event_source*
        sd_event_source_unref(sd_event_source* source) const override;

    void* sd_event_source_get_userdata(sd_event_source* source) const override;
    void* sd_event_source_set_userdata(sd_event_source* source,
                                       void* userdata) const override;

    int sd_event_source_get_description(
        sd_event_source* source, const char** description) const override;
    int sd_event_source_set_description(sd_event_source* source,
                                        const char* description) const override;
    int sd_event_source_set_prepare(sd_event_source* source,
                                    sd_event_handler_t callback) const override;
    int sd_event_source_get_pending(sd_event_source* source) const override;
    int sd_event_source_get_priority(sd_event_source* source,
                                     int64_t* priority) const override;
    int sd_event_source_set_priority(sd_event_source* source,
                                     int64_t priority) const override;
    int sd_event_source_get_enabled(sd_event_source* source,
                                    int* enabled) const override;
    int sd_event_source_set_enabled(sd_event_source* source,
                                    int enabled) const override;
    int sd_event_source_get_io_fd(sd_event_source* source) const override;
    int sd_event_source_set_io_fd(sd_event_source* source,
                                  int fd) const override;
    int sd_event_source_get_io_events(sd_event_source* source,
                                      uint32_t* events) const override;
    int sd_event_source_set_io_events(sd_event_source* source,
                                      uint32_t events) const override;
    int sd_event_source_get_io_revents(sd_event_source* source,
                                       uint32_t* revents) const override;
    int sd_event_source_get_time(sd_event_source* source,
                                 uint64_t* usec) const override;
    int sd_event_source_set_time(sd_event_source* source,
                                 uint64_t usec) const override;
    int sd_event_source_get_time_accuracy(sd_event_source* source,
                                          uint64_t* usec) const override;
    int sd_event_source_set_time_accuracy(sd_event_source* source,
                                          uint64_t usec) const override;
    int sd_event_source_get_signal(sd_event_source* source) const override;
    int sd_event_source_get_child_pid(sd_event_source* source,
                                      pid_t* pid) const override;
    int sd_event_source_set_destroy_callback(
        sd_event_source* source, sd_event_destroy_t callback) const override;
    int sd_event_source_get_destroy_callback(
        sd_event_source* source, sd_event_destroy_t* callback) const override;
    int sd_event_source_set_floating(sd_event_source* source,
                                     int b) const override;
    int sd_event_source_get_floating(sd_event_source* source) const override;
};

/** @brief Default instantiation of sd_event
 */
extern SdEventImpl sdevent_impl;

} // namespace internal
} // namespace sdeventplus
