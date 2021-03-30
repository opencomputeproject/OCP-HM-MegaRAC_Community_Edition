#pragma once

#include <gmock/gmock.h>
#include <sdeventplus/internal/sdevent.hpp>
#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace test
{

/** @class SdEventMock
 *  @brief sd_event mocked implementation
 *  @details Uses googlemock to handle all sd_event calls
 */
class SdEventMock : public internal::SdEvent
{
  public:
    MOCK_CONST_METHOD1(sd_event_default, int(sd_event**));
    MOCK_CONST_METHOD1(sd_event_new, int(sd_event**));
    MOCK_CONST_METHOD1(sd_event_ref, sd_event*(sd_event*));
    MOCK_CONST_METHOD1(sd_event_unref, sd_event*(sd_event*));

    MOCK_CONST_METHOD6(sd_event_add_io,
                       int(sd_event*, sd_event_source**, int, uint32_t,
                           sd_event_io_handler_t, void*));
    MOCK_CONST_METHOD7(sd_event_add_time,
                       int(sd_event*, sd_event_source**, clockid_t, uint64_t,
                           uint64_t, sd_event_time_handler_t, void*));
    MOCK_CONST_METHOD5(sd_event_add_signal,
                       int(sd_event*, sd_event_source**, int,
                           sd_event_signal_handler_t, void*));
    MOCK_CONST_METHOD6(sd_event_add_child,
                       int(sd_event*, sd_event_source**, pid_t, int,
                           sd_event_child_handler_t, void*));
    MOCK_CONST_METHOD4(sd_event_add_defer, int(sd_event*, sd_event_source**,
                                               sd_event_handler_t, void*));
    MOCK_CONST_METHOD4(sd_event_add_post, int(sd_event*, sd_event_source**,
                                              sd_event_handler_t, void*));
    MOCK_CONST_METHOD4(sd_event_add_exit, int(sd_event*, sd_event_source**,
                                              sd_event_handler_t, void*));

    MOCK_CONST_METHOD1(sd_event_prepare, int(sd_event*));
    MOCK_CONST_METHOD2(sd_event_wait, int(sd_event*, uint64_t));
    MOCK_CONST_METHOD1(sd_event_dispatch, int(sd_event*));
    MOCK_CONST_METHOD2(sd_event_run, int(sd_event*, uint64_t));
    MOCK_CONST_METHOD1(sd_event_loop, int(sd_event*));
    MOCK_CONST_METHOD2(sd_event_exit, int(sd_event*, int));

    MOCK_CONST_METHOD3(sd_event_now, int(sd_event*, clockid_t, uint64_t*));

    MOCK_CONST_METHOD2(sd_event_get_exit_code, int(sd_event*, int*));
    MOCK_CONST_METHOD1(sd_event_get_watchdog, int(sd_event*));
    MOCK_CONST_METHOD2(sd_event_set_watchdog, int(sd_event*, int b));

    MOCK_CONST_METHOD1(sd_event_source_ref, sd_event_source*(sd_event_source*));
    MOCK_CONST_METHOD1(sd_event_source_unref,
                       sd_event_source*(sd_event_source*));

    MOCK_CONST_METHOD1(sd_event_source_get_userdata, void*(sd_event_source*));
    MOCK_CONST_METHOD2(sd_event_source_set_userdata,
                       void*(sd_event_source*, void*));

    MOCK_CONST_METHOD2(sd_event_source_get_description,
                       int(sd_event_source*, const char**));
    MOCK_CONST_METHOD2(sd_event_source_set_description,
                       int(sd_event_source*, const char*));
    MOCK_CONST_METHOD2(sd_event_source_set_prepare,
                       int(sd_event_source*, sd_event_handler_t));
    MOCK_CONST_METHOD1(sd_event_source_get_pending, int(sd_event_source*));
    MOCK_CONST_METHOD2(sd_event_source_get_priority,
                       int(sd_event_source*, int64_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_priority,
                       int(sd_event_source*, int64_t));
    MOCK_CONST_METHOD2(sd_event_source_get_enabled,
                       int(sd_event_source*, int*));
    MOCK_CONST_METHOD2(sd_event_source_set_enabled, int(sd_event_source*, int));
    MOCK_CONST_METHOD1(sd_event_source_get_io_fd, int(sd_event_source*));
    MOCK_CONST_METHOD2(sd_event_source_set_io_fd, int(sd_event_source*, int));
    MOCK_CONST_METHOD2(sd_event_source_get_io_events,
                       int(sd_event_source*, uint32_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_io_events,
                       int(sd_event_source*, uint32_t));
    MOCK_CONST_METHOD2(sd_event_source_get_io_revents,
                       int(sd_event_source*, uint32_t*));
    MOCK_CONST_METHOD2(sd_event_source_get_time,
                       int(sd_event_source*, uint64_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_time,
                       int(sd_event_source*, uint64_t));
    MOCK_CONST_METHOD2(sd_event_source_get_time_accuracy,
                       int(sd_event_source*, uint64_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_time_accuracy,
                       int(sd_event_source*, uint64_t));
    MOCK_CONST_METHOD1(sd_event_source_get_signal, int(sd_event_source*));
    MOCK_CONST_METHOD2(sd_event_source_get_child_pid,
                       int(sd_event_source*, pid_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_destroy_callback,
                       int(sd_event_source*, sd_event_destroy_t));
    MOCK_CONST_METHOD2(sd_event_source_get_destroy_callback,
                       int(sd_event_source*, sd_event_destroy_t*));
    MOCK_CONST_METHOD2(sd_event_source_set_floating,
                       int(sd_event_source*, int));
    MOCK_CONST_METHOD1(sd_event_source_get_floating, int(sd_event_source*));
};

} // namespace test
} // namespace sdeventplus
