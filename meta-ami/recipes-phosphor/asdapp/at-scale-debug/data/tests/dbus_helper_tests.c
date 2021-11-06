/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fcntl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "../dbus_helper.h"
#include "../logging.h"
#include "../mem_helper.h"
#include "cmocka.h"
#define MAX_SDBUS_RET_VALUES 20

static bool malloc_fail = false;
void* __real_malloc(size_t size);
void __wrap_ASD_log(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const char* format, ...)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)format;
    //	va_list args;
    //	va_start(args, format);
    //	 vsnprintf(temporary_log_buffer, sizeof(temporary_log_buffer),
    // format, 	 args); 	 fprintf(stderr, "%s\n",
    // temporary_log_buffer); 	 va_end(args);
}
void* __wrap_malloc(size_t size)
{
    if (malloc_fail)
    {
        // put the flag back. This behavior can be changed later if
        // needed.
        malloc_fail = false;
        check_expected(size);
        return NULL;
    }
    return __real_malloc(size);
}

int SD_BUS_ADD_MATCH_RESULT;
int __wrap_sd_bus_add_match(sd_bus* bus, sd_bus_slot** slot, const char* match,
                            sd_bus_message_handler_t callback, void* userdata)
{
    check_expected(bus);
    check_expected(slot);
    check_expected(match);
    check_expected(callback);
    check_expected(userdata);
    return SD_BUS_ADD_MATCH_RESULT;
}

int SD_BUS_MESSAGE_SKIP_RESULT = 0;
int __wrap_sd_bus_message_skip(sd_bus_message* m, const char* types)
{
    return SD_BUS_MESSAGE_SKIP_RESULT;
}

unsigned int SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
int SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[MAX_SDBUS_RET_VALUES] = {0};
int __wrap_sd_bus_message_enter_container(sd_bus_message* m, const char* types,
                                          const char* str)
{
    if (SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX >= MAX_SDBUS_RET_VALUES)
    {
        SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    }
    return SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT
        [SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX++];
}

int SD_BUS_MESSAGE_EXIT_CONTAINER_RESULT = 0;
int __wrap_sd_bus_message_exit_container(sd_bus_message* m)
{
    return SD_BUS_MESSAGE_EXIT_CONTAINER_RESULT;
}

int SD_BUS_OPEN_SYSTEM_RESULT = 0;
int __wrap_sd_bus_open_system(sd_bus** bus)
{
    check_expected_ptr(bus);
    return SD_BUS_OPEN_SYSTEM_RESULT;
}

sd_bus* __wrap_sd_bus_unref(sd_bus* bus)
{
    check_expected_ptr(bus);
    return NULL;
}
int SD_BUS_GET_FD_RESULT = 0;
int __wrap_sd_bus_get_fd(sd_bus* bus)
{
    check_expected_ptr(bus);
    return SD_BUS_GET_FD_RESULT;
}

unsigned int SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
int SD_BUS_MESSAGE_READ_RESULT[MAX_SDBUS_RET_VALUES] = {0};
char FAKE_READ_VALUE[256];
bool SD_BUS_MESSAGE_READ_CHUNK = false;
char FAKE_READ_VALUES[MAX_SDBUS_RET_VALUES][256];
int __wrap_sd_bus_message_read(sd_bus_message* m, const char* types,
                               char* value)
{
    check_expected_ptr(m);
    check_expected_ptr(types);
    if (SD_BUS_MESSAGE_READ_RESULT_INDEX >= MAX_SDBUS_RET_VALUES)
    {
        SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    }
    if (SD_BUS_MESSAGE_READ_CHUNK == false)
    {
        *(const char**)value = (const char*)&FAKE_READ_VALUE;
    }
    else
    {
        *(const char**)value =
            (const char*)&FAKE_READ_VALUES[SD_BUS_MESSAGE_READ_RESULT_INDEX];
    }
    return SD_BUS_MESSAGE_READ_RESULT[SD_BUS_MESSAGE_READ_RESULT_INDEX++];
}

void __wrap_sd_bus_error_free(sd_bus_error* error)
{
    check_expected_ptr(error);
}

sd_bus_message* SD_BUS_MESSAGE_UNREF_RESULT = NULL;
sd_bus_message* __wrap_sd_bus_message_unref(sd_bus_message* m)
{
    check_expected_ptr(m);
    return SD_BUS_MESSAGE_UNREF_RESULT;
}

int SD_BUS_GET_PROPERTY_RESULT = 0;
int __wrap_sd_bus_get_property(sd_bus* bus, const char* destination,
                               const char* path, const char* interface,
                               const char* member, sd_bus_error* ret_error,
                               sd_bus_message** reply, const char* type)
{
    check_expected_ptr(bus);
    check_expected(destination);
    check_expected(path);
    check_expected(interface);
    check_expected(member);
    check_expected(type);
    return SD_BUS_GET_PROPERTY_RESULT;
}

bool callb = false;
Power_State* callb_ptr = NULL;
Power_State callb_state = STATE_UNKNOWN;
int SD_BUS_PROCESS_RESULT;
int __wrap_sd_bus_process(sd_bus* bus, sd_bus_message* m)
{
    check_expected_ptr(bus);

    if (callb)
    {
        Dbus_Handle* handle = (Dbus_Handle*)&bus;
        if (callb_ptr != NULL)
            *callb_ptr = callb_state;
    }
    return SD_BUS_PROCESS_RESULT;
}

int SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
int __wrap_sd_bus_message_new_method_call(sd_bus* bus, sd_bus_message** m,
                                          const char* destination,
                                          const char* path,
                                          const char* interface,
                                          const char* member)
{
    return SD_BUS_MESSAGE_NEW_METHOD_RESULT;
}

int SD_BUS_MESSAGE_APPEND_RESULT = 0;
int __wrap_sd_bus_message_append(sd_bus* bus, const char* types, ...)
{
    return SD_BUS_MESSAGE_APPEND_RESULT;
}

int SD_BUS_MESSAGE_CLOSE_CONTAINER_RESULT = 0;
int __wrap_sd_bus_message_close_container(sd_bus_message* m)
{
    return SD_BUS_MESSAGE_CLOSE_CONTAINER_RESULT;
}

int SD_BUS_MESSAGE_OPEN_CONTAINER_RESULT = 0;
int __wrap_sd_bus_message_open_container(sd_bus* bus, const char type,
                                         const char* contents)
{
    return SD_BUS_MESSAGE_OPEN_CONTAINER_RESULT;
}

int SD_BUS_ERROR_SET_ERRNO_RESULT = 0;
int __wrap_sd_bus_error_set_errno(sd_bus_error* e, int error)
{
    return SD_BUS_ERROR_SET_ERRNO_RESULT;
}

int SD_BUS_CALL_ASYNC_RESULT = 0;
int __wrap_sd_bus_call_async(sd_bus* bus, sd_bus_slot** slot, sd_bus_message* m,
                             sd_bus_message_handler_t callback, void* userdata,
                             uint64_t usec)
{
    return SD_BUS_CALL_ASYNC_RESULT;
}
void expect_get_power_state(const char* state)
{
    expect_any(__wrap_sd_bus_get_property, bus);
    expect_string(__wrap_sd_bus_get_property, destination,
                  POWER_SERVICE_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, path, POWER_OBJECT_PATH_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, interface,
                  POWER_INTERFACE_NAME_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, member,
                  GET_POWER_STATE_PROPERTY_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, type, "s");

    memset(&FAKE_READ_VALUE, 0, sizeof(FAKE_READ_VALUE));
    memcpy(&FAKE_READ_VALUE, state, strlen(state));
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");
}

void expect_power_on()
{
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
    expect_any(__wrap_sd_bus_error_free, error);
}

void expect_power_off()
{
    expect_any(__wrap_sd_bus_error_free, error);
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
}

void expect_power_reset()
{
    expect_any(__wrap_sd_bus_error_free, error);
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
}

void expect_power_reboot()
{
    expect_any(__wrap_sd_bus_error_free, error);
    SD_BUS_PROCESS_RESULT = 0;
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
}

void expect_power_reboot_failure1()
{
    expect_any(__wrap_sd_bus_error_free, error);
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = -1;
}

void expect_power_reboot_failure2()
{
    expect_any(__wrap_sd_bus_error_free, error);
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = -1;
}

void dbus_helper_malloc_fail_test(void** state)
{
    (void)state; /* unused */
    expect_value(__wrap_malloc, size, sizeof(Dbus_Handle));
    malloc_fail = true;
    assert_null(dbus_helper());
    malloc_fail = false;
}

void dbus_helper_success_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle* handle;
    malloc_fail = false;
    handle = dbus_helper();
    assert_non_null(handle);
    assert_null(handle->bus);
    free(handle);
}

void dbus_initialize_null_param_test(void** state)
{
    (void)state; /* unused */
    assert_int_equal(ST_ERR, dbus_initialize(NULL));
}

void dbus_initialize_fail_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    expect_any(__wrap_sd_bus_open_system, bus);
    SD_BUS_OPEN_SYSTEM_RESULT = -1;
    assert_int_equal(ST_ERR, dbus_initialize(&handle));
}

void dbus_initialize_fail_to_get_fd_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    SD_BUS_OPEN_SYSTEM_RESULT = 0;
    expect_any(__wrap_sd_bus_open_system, bus);
    SD_BUS_GET_FD_RESULT = -2;
    expect_any(__wrap_sd_bus_get_fd, bus);
    expect_any(__wrap_sd_bus_unref, bus); // for deinitialize
    assert_int_equal(ST_ERR, dbus_initialize(&handle));
}

void dbus_initialize_fail_to_dbus_gethotstate(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.power_state = STATE_UNKNOWN;
    expect_any(__wrap_sd_bus_open_system, bus);
    SD_BUS_GET_FD_RESULT = 9;
    expect_any(__wrap_sd_bus_get_fd, bus);
    SD_BUS_OPEN_SYSTEM_RESULT = 0;

    SD_BUS_ADD_MATCH_RESULT = 0;
    expect_any(__wrap_sd_bus_add_match, bus);
    expect_any(__wrap_sd_bus_add_match, slot);
    expect_string(__wrap_sd_bus_add_match, match, MATCH_STRING_CHASSIS);
    expect_any(__wrap_sd_bus_add_match, callback);
    expect_any(__wrap_sd_bus_add_match, userdata);

    SD_BUS_GET_PROPERTY_RESULT = -1;
    expect_any(__wrap_sd_bus_get_property, bus);
    expect_string(__wrap_sd_bus_get_property, destination,
                  POWER_SERVICE_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, path, POWER_OBJECT_PATH_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, interface,
                  POWER_INTERFACE_NAME_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, member,
                  GET_POWER_STATE_PROPERTY_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, type, "s");

    expect_any(__wrap_sd_bus_error_free, error);
    expect_any(__wrap_sd_bus_message_unref, m);

    expect_any(__wrap_sd_bus_unref, bus);

    assert_int_equal(ST_ERR, dbus_initialize(&handle));
}

void dbus_initialize_fail_to_sd_bus_add_match_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    expect_any(__wrap_sd_bus_open_system, bus);
    SD_BUS_GET_FD_RESULT = 9;
    expect_any(__wrap_sd_bus_get_fd, bus);
    SD_BUS_OPEN_SYSTEM_RESULT = 0;

    SD_BUS_ADD_MATCH_RESULT = -1;
    expect_any(__wrap_sd_bus_add_match, bus);
    expect_any(__wrap_sd_bus_add_match, slot);
    expect_string(__wrap_sd_bus_add_match, match, MATCH_STRING_CHASSIS);
    expect_any(__wrap_sd_bus_add_match, callback);
    expect_any(__wrap_sd_bus_add_match, userdata);

    expect_any(__wrap_sd_bus_unref, bus);

    assert_int_equal(ST_ERR, dbus_initialize(&handle));
}

void dbus_dbus_gethotstate_on_state_unknown_success_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.power_state = STATE_UNKNOWN;
    int value = 0;
    SD_BUS_GET_PROPERTY_RESULT = 0;

    expect_any(__wrap_sd_bus_get_property, bus);
    expect_string(__wrap_sd_bus_get_property, destination,
                  POWER_SERVICE_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, path, POWER_OBJECT_PATH_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, interface,
                  POWER_INTERFACE_NAME_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, member,
                  GET_POWER_STATE_PROPERTY_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, type, "s");

    memcpy_safe(FAKE_READ_VALUE, sizeof(FAKE_READ_VALUE),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    expect_any(__wrap_sd_bus_error_free, error);
    expect_any(__wrap_sd_bus_message_unref, m);

    assert_int_equal(ST_OK, dbus_get_powerstate(&handle, &value));
}

void dbus_dbus_gethotstate_off_state_unknown_success_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.power_state = STATE_UNKNOWN;
    int value = 0;
    SD_BUS_GET_PROPERTY_RESULT = 0;

    expect_any(__wrap_sd_bus_get_property, bus);
    expect_string(__wrap_sd_bus_get_property, destination,
                  POWER_SERVICE_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, path, POWER_OBJECT_PATH_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, interface,
                  POWER_INTERFACE_NAME_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, member,
                  GET_POWER_STATE_PROPERTY_CHASSIS);
    expect_string(__wrap_sd_bus_get_property, type, "s");

    memcpy_safe(FAKE_READ_VALUE, sizeof(FAKE_READ_VALUE),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    expect_any(__wrap_sd_bus_error_free, error);
    expect_any(__wrap_sd_bus_message_unref, m);

    assert_int_equal(ST_OK, dbus_get_powerstate(&handle, &value));
}

void dbus_initialize_success_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    expect_any(__wrap_sd_bus_open_system, bus);
    SD_BUS_GET_FD_RESULT = 9;
    expect_any(__wrap_sd_bus_get_fd, bus);
    SD_BUS_OPEN_SYSTEM_RESULT = 0;
    SD_BUS_GET_PROPERTY_RESULT = 0;

    // sd_bus_add_match
    SD_BUS_ADD_MATCH_RESULT = 0;
    expect_any(__wrap_sd_bus_add_match, bus);
    expect_any(__wrap_sd_bus_add_match, slot);
    expect_string(__wrap_sd_bus_add_match, match, MATCH_STRING_CHASSIS);
    expect_any(__wrap_sd_bus_add_match, callback);
    expect_any(__wrap_sd_bus_add_match, userdata);

    assert_int_equal(ST_OK, dbus_initialize(&handle));
    assert_int_equal(SD_BUS_GET_FD_RESULT, handle.fd);
}

void dbus_deinitialize_null_param_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.bus = NULL;
    assert_int_equal(ST_ERR, dbus_deinitialize(NULL));
    assert_int_equal(ST_ERR, dbus_deinitialize(&handle));
}

void dbus_deinitialize_success_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_any(__wrap_sd_bus_unref, bus);
    assert_int_equal(ST_OK, dbus_deinitialize(&handle));
}

void dbus_power_reset_null_param_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.bus = NULL;
    assert_int_equal(ST_ERR, dbus_power_reset(NULL));
    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_success_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;

    expect_power_reset();

    assert_int_equal(ST_OK, dbus_power_reset(&handle));
}

void dbus_power_toggle_null_param_test(void** state)
{
    (void)state; /* unused */
    Dbus_Handle handle;
    handle.bus = NULL;
    assert_int_equal(ST_ERR, dbus_power_toggle(NULL));
    assert_int_equal(ST_ERR, dbus_power_toggle(&handle));
}

void dbus_power_toggle_on_success_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_get_power_state(GET_POWER_STATE_PROPERTY_CHASSIS);
    expect_power_on();
    expect_any(__wrap_sd_bus_error_free, error);
    expect_any(__wrap_sd_bus_message_unref, m);

    assert_int_equal(ST_OK, dbus_power_toggle(&handle));
}

void dbus_power_toggle_off_success_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_get_power_state(POWER_ON_PROPERTY_CHASSIS);
    expect_power_off();
    expect_any(__wrap_sd_bus_error_free, error);
    expect_any(__wrap_sd_bus_message_unref, m);

    assert_int_equal(ST_OK, dbus_power_toggle(&handle));
}

void dbus_power_reset_callback_success_test(void** state)
{
    sd_bus_message* reply;
    sd_bus_error* error;
    assert_int_equal(ST_OK, sdbus_callback(reply, (void*)&callb, error));
}

void dbus_power_reset_process_failure1_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure1();

    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_process_wait_success_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot();
    SD_BUS_PROCESS_RESULT = 0;

    assert_int_equal(ST_OK, dbus_power_reset(&handle));
}

void dbus_power_reset_wait_failure_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();
    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_async_failure1_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = -1;
    SD_BUS_ERROR_SET_ERRNO_RESULT = -1;
    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_async_failure2_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
    SD_BUS_MESSAGE_APPEND_RESULT = -1;
    SD_BUS_ERROR_SET_ERRNO_RESULT = -1;
    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_async_failure3_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();

    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_async_failure4_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
    SD_BUS_MESSAGE_APPEND_RESULT = 0;
    SD_BUS_MESSAGE_CLOSE_CONTAINER_RESULT = -1;
    SD_BUS_ERROR_SET_ERRNO_RESULT = -1;
    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_power_reset_async_failure5_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    expect_power_reboot_failure2();
    SD_BUS_MESSAGE_NEW_METHOD_RESULT = 0;
    SD_BUS_MESSAGE_APPEND_RESULT = 0;
    SD_BUS_MESSAGE_OPEN_CONTAINER_RESULT = -1;
    SD_BUS_ERROR_SET_ERRNO_RESULT = -1;

    assert_int_equal(ST_ERR, dbus_power_reset(&handle));
}

void dbus_process_event_state_equal_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;

    expect_any(__wrap_sd_bus_process, bus);
    assert_int_equal(ST_OK, dbus_process_event(&handle, &event));
}

void dbus_process_event_state_off_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    SD_BUS_PROCESS_RESULT = ST_OK;
    handle.power_state = STATE_ON;
    handle.bus = (sd_bus*)&dummy;
    callb = true;
    callb_ptr = &handle.power_state;
    callb_state = STATE_OFF;
    expect_any(__wrap_sd_bus_process, bus);
    assert_int_equal(ST_OK, dbus_process_event(&handle, &event));
}

void dbus_process_event_state_on_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    SD_BUS_PROCESS_RESULT = ST_OK;
    handle.power_state = STATE_OFF;
    handle.bus = (sd_bus*)&dummy;
    callb = true;
    callb_ptr = &handle.power_state;
    callb_state = STATE_ON;
    expect_any(__wrap_sd_bus_process, bus);
    assert_int_equal(ST_OK, dbus_process_event(&handle, &event));
}

void dbus_process_event_fail_sd_bus_process_test(void** state)
{
    (void)state; /* unused */
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_PROCESS_RESULT = -1;

    expect_any(__wrap_sd_bus_process, bus);
    assert_int_equal(ST_ERR, dbus_process_event(&handle, &event));
}

void match_callback_fail_to_sd_bus_message_skip_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = -1;

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
}

void match_callback_fail_to_sd_bus_message_enter_container_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = -1;

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
}

void match_callback_fail_to_sd_bus_message_read_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = 0;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = -1;
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
}

void match_callback_discard_non_get_power_messages_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = -1;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                SET_POWER_STATE_METHOD_CHASSIS,
                sizeof(SET_POWER_STATE_METHOD_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

void match_callback_fail_to_sd_bus_message_enter_container_variant_test(
    void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = -1;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

void match_callback_fail_to_sd_bus_read_variant_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = 0;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_RESULT[1] = -1;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

void match_callback_fail_to_sd_bus_message_exit_container_test(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = 0;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_RESULT[1] = 0;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    SD_BUS_MESSAGE_EXIT_CONTAINER_RESULT = -1;

    assert_int_equal(ST_ERR, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

void match_callback_test_power_on(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_EXIT_CONTAINER_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = 0;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_RESULT[1] = 0;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_ON_PROPERTY_CHASSIS,
                sizeof(POWER_ON_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_OK, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

void match_callback_test_power_off(void** state)
{
    (void)state; /* unused */
    int retcode = 0;
    int dummy;
    ASD_EVENT event;
    Dbus_Handle handle;
    sd_bus_message* msg;
    sd_bus_error* error;
    handle.bus = (sd_bus*)&dummy;
    SD_BUS_MESSAGE_SKIP_RESULT = 0;
    SD_BUS_MESSAGE_EXIT_CONTAINER_RESULT = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[0] = 2;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[1] = 1;
    SD_BUS_MESSAGE_ENTER_CONTAINER_RESULT[2] = 0;

    SD_BUS_MESSAGE_READ_RESULT_INDEX = 0;
    SD_BUS_MESSAGE_READ_RESULT[0] = 0;
    SD_BUS_MESSAGE_READ_RESULT[1] = 0;
    SD_BUS_MESSAGE_READ_CHUNK = true;
    memset(&FAKE_READ_VALUES[0], 0, sizeof(FAKE_READ_VALUES[0]));
    memset(&FAKE_READ_VALUES[1], 0, sizeof(FAKE_READ_VALUES[1]));
    memcpy_safe(&FAKE_READ_VALUES[0], sizeof(FAKE_READ_VALUES[0]),
                GET_POWER_STATE_PROPERTY_CHASSIS,
                sizeof(GET_POWER_STATE_PROPERTY_CHASSIS) - 1);
    memcpy_safe(&FAKE_READ_VALUES[1], sizeof(FAKE_READ_VALUES[1]),
                POWER_OFF_PROPERTY_CHASSIS,
                sizeof(POWER_OFF_PROPERTY_CHASSIS) - 1);
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");
    expect_any(__wrap_sd_bus_message_read, m);
    expect_string(__wrap_sd_bus_message_read, types, "s");

    assert_int_equal(ST_OK, match_callback(msg, &handle, error));
    SD_BUS_MESSAGE_READ_CHUNK = false;
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(dbus_helper_malloc_fail_test),
        cmocka_unit_test(dbus_helper_success_test),
        cmocka_unit_test(dbus_initialize_null_param_test),
        cmocka_unit_test(dbus_initialize_fail_test),
        cmocka_unit_test(dbus_initialize_fail_to_sd_bus_add_match_test),
        cmocka_unit_test(dbus_dbus_gethotstate_on_state_unknown_success_test),
        cmocka_unit_test(dbus_dbus_gethotstate_off_state_unknown_success_test),
        cmocka_unit_test(dbus_initialize_fail_to_dbus_gethotstate),
        cmocka_unit_test(dbus_initialize_fail_to_get_fd_test),
        cmocka_unit_test(dbus_initialize_success_test),
        cmocka_unit_test(dbus_deinitialize_null_param_test),
        cmocka_unit_test(dbus_deinitialize_success_test),
        cmocka_unit_test(dbus_power_reset_null_param_test),
        cmocka_unit_test(dbus_power_toggle_null_param_test),
        cmocka_unit_test(dbus_power_toggle_on_success_test),
        cmocka_unit_test(dbus_power_toggle_off_success_test),
        cmocka_unit_test(dbus_power_reset_success_test),
        cmocka_unit_test(dbus_power_reset_callback_success_test),
        cmocka_unit_test(dbus_power_reset_process_failure1_test),
        cmocka_unit_test(dbus_power_reset_process_wait_success_test),
        cmocka_unit_test(dbus_power_reset_wait_failure_test),
        cmocka_unit_test(dbus_power_reset_async_failure1_test),
        cmocka_unit_test(dbus_power_reset_async_failure2_test),
        cmocka_unit_test(dbus_power_reset_async_failure3_test),
        cmocka_unit_test(dbus_power_reset_async_failure4_test),
        cmocka_unit_test(dbus_power_reset_async_failure5_test),
        cmocka_unit_test(dbus_process_event_state_equal_test),
        cmocka_unit_test(dbus_process_event_fail_sd_bus_process_test),
        cmocka_unit_test(dbus_process_event_state_off_test),
        cmocka_unit_test(dbus_process_event_state_on_test),
        cmocka_unit_test(match_callback_fail_to_sd_bus_message_skip_test),
        cmocka_unit_test(match_callback_discard_non_get_power_messages_test),
        cmocka_unit_test(
            match_callback_fail_to_sd_bus_message_enter_container_test),
        cmocka_unit_test(match_callback_fail_to_sd_bus_message_read_test),
        cmocka_unit_test(
            match_callback_fail_to_sd_bus_message_enter_container_variant_test),
        cmocka_unit_test(match_callback_fail_to_sd_bus_read_variant_test),
        cmocka_unit_test(
            match_callback_fail_to_sd_bus_message_exit_container_test),
        cmocka_unit_test(match_callback_test_power_on),
        cmocka_unit_test(match_callback_test_power_off),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
