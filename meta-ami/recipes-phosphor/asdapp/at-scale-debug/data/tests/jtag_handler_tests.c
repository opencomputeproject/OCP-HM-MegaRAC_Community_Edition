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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../jtag_handler.h"
#include "../logging.h"
#include "cmocka.h"

#define TEST_BUFFER_SIZE 50
#ifdef JTAG_LEGACY_DRIVER
#define JTAG_DEVICE_PATHNAME "/dev/jtag"
#else
#define JTAG_DEVICE_PATHNAME "/dev/jtag0"
#endif

static int FAKE_DRIVER_HANDLE = 4;

// static char temporary_log_buffer[512];
void __wrap_ASD_log(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const char* format, ...)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)format;
    // va_list args;
    // va_start(args, format);
    // vsnprintf(temporary_log_buffer, sizeof(temporary_log_buffer), format,
    // args);
    // fprintf(stderr, "%s\n", temporary_log_buffer);
    // va_end(args);
}

void __wrap_ASD_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                           ASD_LogOption options, const unsigned char* ptr,
                           size_t len, const char* prefixPtr)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)ptr;
    (void)len;
    (void)prefixPtr;
}

void __wrap_ASD_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                          ASD_LogOption options,
                          const unsigned int number_of_bits,
                          unsigned int size_bytes, const unsigned char* buffer,
                          const char* prefixPtr)
{
    (void)level;
    (void)stream;
    (void)options;
    (void)number_of_bits;
    (void)size_bytes;
    (void)buffer;
    (void)prefixPtr;
}

int __real_open(const char* pathname, int flags, int mode);
int __wrap_open(const char* pathname, int flags, int mode)
{
    if (strlen(pathname) > 5 &&
        !strcmp(pathname + strlen(pathname) - 5, ".gcda"))
        return __real_open(pathname, flags, mode);

    check_expected_ptr(pathname);
    check_expected(flags);
    check_expected(mode);
    return FAKE_DRIVER_HANDLE;
}

static bool malloc_fail = false;
void* __real_malloc(size_t size);
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

static unsigned int ioctl_args;
struct scan_xfer* ioctl_arg_scan_xfr;
struct controller_mode_param* ioctl_arg_controller_mode_param;
struct tap_state_param* ioctl_arg_tap_state_param;
struct set_tck_param* ioctl_arg_set_tck_param;
struct tck_bitbang* ioctl_arg_tck_bitbang;
typedef enum
{
    IoctlArgType_UInt = 0,
    IoctlArgType_scan_xfer,
    IoctlArgType_controller_mode_param,
    IoctlArgType_tap_state_param,
    IoctlArgType_set_tck_param,
    IoctlArgType_tck_bitbang
} IoctlArgType;
#define MAX_IOCTLS 15
static IoctlArgType ioctl_arg_types[MAX_IOCTLS] = {IoctlArgType_UInt};
static int FAKE_IOCTL_RESULT[MAX_IOCTLS];
static int ioctl_arg_index = 0;
static int test_ioctl_index = 0;
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    int index = ioctl_arg_index;
    check_expected(fd);
    check_expected(request);

    va_list args;
    va_start(args, request);
    if (ioctl_arg_types[index] == IoctlArgType_UInt)
    {
        ioctl_args = va_arg(args, unsigned int);
        check_expected(ioctl_args);
    }
    else if (ioctl_arg_types[index] == IoctlArgType_scan_xfer)
    {
        ioctl_arg_scan_xfr = va_arg(args, struct scan_xfer*);
        check_expected_ptr(ioctl_arg_scan_xfr);
    }
    else if (ioctl_arg_types[index] == IoctlArgType_controller_mode_param)
    {
        ioctl_arg_controller_mode_param =
            va_arg(args, struct controller_mode_param*);
        check_expected_ptr(ioctl_arg_controller_mode_param);
    }
    else if (ioctl_arg_types[index] == IoctlArgType_tap_state_param)
    {
        ioctl_arg_tap_state_param = va_arg(args, struct tap_state_param*);
        check_expected_ptr(ioctl_arg_tap_state_param);
    }
    else if (ioctl_arg_types[index] == IoctlArgType_set_tck_param)
    {
        ioctl_arg_set_tck_param = va_arg(args, struct set_tck_param*);
        check_expected_ptr(ioctl_arg_set_tck_param);
    }
    else if (ioctl_arg_types[index] == IoctlArgType_tck_bitbang)
    {
        ioctl_arg_tck_bitbang = va_arg(args, struct tck_bitbang*);
        check_expected_ptr(ioctl_arg_tck_bitbang);
    }
    ioctl_arg_index++;

    va_end(args);
    return FAKE_IOCTL_RESULT[index];
}

void expect_any_ioctl_helper()
{
    ioctl_arg_types[0] = IoctlArgType_UInt;
    expect_any_count(__wrap_ioctl, fd, -1);
    expect_any_count(__wrap_ioctl, request, -1);
    expect_any_count(__wrap_ioctl, ioctl_args, -1);
}

JTAG_Handler* create_JTAGHandler_helper()
{
    JTAG_Handler* handler = JTAGHandler();
    assert_true(handler != NULL);
    return handler;
}

static int setup(void** state)
{
    int i = 0;
    for (; i < MAX_IOCTLS; i++)
    {
        ioctl_arg_types[i] = IoctlArgType_UInt;
        FAKE_IOCTL_RESULT[i] = 0;
    }
    ioctl_arg_index = 0;
    test_ioctl_index = 0;
    ioctl_arg_scan_xfr = NULL;
    ioctl_arg_controller_mode_param = NULL;
    ioctl_arg_tap_state_param = NULL;
    ioctl_arg_set_tck_param = NULL;
    ioctl_arg_tck_bitbang = NULL;
    malloc_fail = false;
    *state = (void*)create_JTAGHandler_helper();
    assert_true(*state != NULL);
    return 0;
}

static int teardown(void** state)
{
    free(*state);
    return 0;
}

int MEMCPY_SAFE_RESULT = 0;
int __wrap_memcpy_safe(void* dest, size_t destsize, const void* src,
                       size_t count)
{
    return MEMCPY_SAFE_RESULT;
}

void JTAGHandler_malloc_failure(void** state)
{
    (void)state; /* unused */
    expect_value(__wrap_malloc, size, sizeof(JTAG_Handler));
    malloc_fail = true;
    assert_null(JTAGHandler());
    malloc_fail = false;
}

void JTAGHandler_initializes_padding(void** state)
{
    JTAG_Handler* handler = *state;
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        assert_int_equal(handler->chains[i].shift_padding.drPre, 0);
        assert_int_equal(handler->chains[i].shift_padding.drPost, 0);
        assert_int_equal(handler->chains[i].shift_padding.irPre, 0);
        assert_int_equal(handler->chains[i].shift_padding.irPost, 0);
    }
}

void JTAGHandler_initializes_tap_state_to_TRL(void** state)
{
    JTAG_Handler* handler = *state;
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        assert_int_equal(handler->chains[i].tap_state, jtag_tlr);
    }
}

void JTAGHandler_initializes_padDataOne_to_FFs(void** state)
{
    JTAG_Handler* handler = *state;
    for (int j = 0; j < sizeof(handler->padDataOne); j++)
        assert_int_equal(handler->padDataOne[j], 0xff);
}

void JTAGHandler_initializes_padDataZero_to_0s(void** state)
{
    JTAG_Handler* handler = *state;
    for (int j = 0; j < sizeof(handler->padDataZero); j++)
        assert_int_equal(handler->padDataZero[j], 0);
}

void JTAG_initialize_NULL_state_check(void** state)
{
    (void)state; /* unused */
    STATUS status = JTAG_initialize(NULL, false);
    assert_int_equal(status, ST_ERR);
}

void JTAG_initialize_driver_open_failure(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
    FAKE_DRIVER_HANDLE = -1;
    assert_int_equal(ST_ERR, JTAG_initialize(handler, false));
}

#ifndef JTAG_LEGACY_DRIVER
void JTAG_initialize_sets_JTAG_set_mode_failure(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;

    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);

    ioctl_arg_types[test_ioctl_index] = IoctlArgType_controller_mode_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, JTAG_SIOCMODE);
    expect_any(__wrap_ioctl, ioctl_arg_controller_mode_param);

    assert_int_equal(ST_ERR, JTAG_initialize(handler, false));
}
#endif

void JTAG_initialize_sets_JTAG_set_tap_state_to_TLR(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
#ifndef JTAG_LEGACY_DRIVER
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_controller_mode_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, JTAG_SIOCMODE);
    expect_any(__wrap_ioctl, ioctl_arg_controller_mode_param);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;
#endif
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);

    assert_int_equal(ST_OK, JTAG_initialize(handler, false));
}

void JTAG_initialize_handles_JTAG_set_tap_state_error(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
#ifndef JTAG_LEGACY_DRIVER
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_controller_mode_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, JTAG_SIOCMODE);
    expect_any(__wrap_ioctl, ioctl_arg_controller_mode_param);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;
#endif
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;
    assert_int_equal(ST_ERR, JTAG_initialize(handler, false));
}

void JTAG_initialize_clears_padding(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        handler->chains[i].shift_padding.drPre = 1;
        handler->chains[i].shift_padding.drPost = 2;
        handler->chains[i].shift_padding.irPre = 3;
        handler->chains[i].shift_padding.irPost = 4;
    }
#ifndef JTAG_LEGACY_DRIVER
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_controller_mode_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, JTAG_SIOCMODE);
    expect_any(__wrap_ioctl, ioctl_arg_controller_mode_param);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;
#endif
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    FAKE_IOCTL_RESULT[test_ioctl_index] = 0;
    JTAG_initialize(handler, false);
    for (int i = 0; i < MAX_SCAN_CHAINS; i++)
    {
        assert_int_equal(handler->chains[i].shift_padding.drPre, 0);
        assert_int_equal(handler->chains[i].shift_padding.drPost, 0);
        assert_int_equal(handler->chains[i].shift_padding.irPre, 0);
        assert_int_equal(handler->chains[i].shift_padding.irPost, 0);
    }
}

void JTAG_initialize_returns_ST_OK(void** state)
{
    JTAG_Handler* handler = *state;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, JTAG_DEVICE_PATHNAME);
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
#ifndef JTAG_LEGACY_DRIVER
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_controller_mode_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, JTAG_SIOCMODE);
    expect_any(__wrap_ioctl, ioctl_arg_controller_mode_param);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;
#endif
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, FAKE_DRIVER_HANDLE);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    FAKE_IOCTL_RESULT[test_ioctl_index] = 0;
    assert_int_equal(JTAG_initialize(handler, false), ST_OK);
}

void JTAG_deinitialize_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_deinitialize(NULL), ST_ERR);
}

void JTAG_deinitialize_success(void** state)
{
    JTAG_Handler* handler = *state;
    handler->JTAG_driver_handle = 2;
    assert_int_equal(JTAG_deinitialize(handler), ST_OK);
}

void JTAG_set_padding_param_checks(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_set_padding(NULL, JTAGPaddingTypes_DRPre, 0), ST_ERR);
    JTAG_Handler* handler = *state;
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_DRPre, DRMAXPADSIZE + 1),
        ST_ERR);
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_IRPost, IRMAXPADSIZE + 1),
        ST_ERR);
}

void JTAG_set_padding_handles_JTAGPaddingTypes_DRPre(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected = 78;
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_DRPre, expected), ST_OK);
    assert_int_equal(handler->active_chain->shift_padding.drPre, expected);
}

void JTAG_set_padding_handles_JTAGPaddingTypes_DRPost(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected = 79;
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_DRPost, expected), ST_OK);
    assert_int_equal(handler->active_chain->shift_padding.drPost, expected);
}

void JTAG_set_padding_handles_JTAGPaddingTypes_IRPre(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected = 68;
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_IRPre, expected), ST_OK);
    assert_int_equal(handler->active_chain->shift_padding.irPre, expected);
}

void JTAG_set_padding_handles_JTAGPaddingTypes_IRPost(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected = 69;
    assert_int_equal(
        JTAG_set_padding(handler, JTAGPaddingTypes_IRPost, expected), ST_OK);
    assert_int_equal(handler->active_chain->shift_padding.irPost, expected);
}

void JTAG_set_padding_handles_unknown_padding_type(void** state)
{
    unsigned int expected = 69;
    int some_unknown_type = -22;
    assert_int_equal(JTAG_set_padding(*state, some_unknown_type, expected),
                     ST_ERR);
}

void JTAG_tap_reset_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_tap_reset(NULL), ST_ERR);
}

void JTAG_tap_reset_sets_tap_state_to_TLR(void** state)
{
    JTAG_Handler* handler = *state;
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    assert_int_equal(JTAG_tap_reset(handler), ST_OK);
    assert_int_equal(handler->active_chain->tap_state, jtag_tlr);
}

void JTAG_set_tap_state_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_set_tap_state(NULL, jtag_tlr), ST_ERR);
}

void JTAG_set_tap_state_calls_ioctl_with_correct_parameters(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected_state = jtag_tlr;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = SW_MODE;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];

    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);

    JTAG_set_tap_state(handler, expected_state);
}

void JTAG_set_tap_state_sets_tap_state_correctly(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected = jtag_pau_ir;
    handler->active_chain->tap_state = jtag_tlr;
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    assert_int_equal(JTAG_set_tap_state(handler, expected), ST_OK);
    assert_int_equal(handler->active_chain->tap_state, expected);
}

void JTAG_set_tap_state_jtag_rti_wait_cycles_failed(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected_state = jtag_rti;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    // expectations for set tap state
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
#ifdef JTAG_LEGACY_DRIVER
    // expectations for wait cycles
    ioctl_arg_types[test_ioctl_index += 4] = IoctlArgType_UInt;
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 5);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 5);
    expect_any_count(__wrap_ioctl, ioctl_args, 5);
#else
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_UInt;
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 1);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 1);
    expect_any_count(__wrap_ioctl, ioctl_args, 1);
#endif
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1; // fail on last one
    assert_int_equal(JTAG_set_tap_state(handler, expected_state), ST_ERR);
}

void JTAG_set_tap_state_jtag_rti_execute_five_wait_cycles(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected_state = jtag_rti;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    // expectations for set tap state
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    // expectations for wait cycles
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_UInt;
#ifdef JTAG_LEGACY_DRIVER
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 5);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 5);
    expect_any_count(__wrap_ioctl, ioctl_args, 5);
#else
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 1);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 1);
    expect_any_count(__wrap_ioctl, ioctl_args, 1);
#endif
    assert_int_equal(JTAG_set_tap_state(handler, expected_state), ST_OK);
}

void JTAG_set_tap_state_jtag_pau_dr_execute_five_wait_cycles(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected_state = jtag_pau_dr;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    // expectations for set tap state
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    // expectations for wait cycles
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_UInt;
#ifdef JTAG_LEGACY_DRIVER
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 5);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 5);
    expect_any_count(__wrap_ioctl, ioctl_args, 5);
#else
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 1);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 1);
    expect_any_count(__wrap_ioctl, ioctl_args, 1);
#endif
    assert_int_equal(JTAG_set_tap_state(handler, expected_state), ST_OK);
}

void JTAG_set_tap_state_ioctl_failure(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states expected_state = jtag_pau_dr;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    // expectations for set tap state
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tap_state_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TAPSTATE);
    expect_any(__wrap_ioctl, ioctl_arg_tap_state_param);
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;
    assert_int_equal(JTAG_set_tap_state(handler, expected_state), ST_ERR);
}

void JTAG_get_tap_state_NULL_state_check(void** state)
{
    (void)state; /* unused */
    enum jtag_states jtag_state = jtag_rti;
    assert_int_equal(JTAG_get_tap_state(NULL, &jtag_state), ST_ERR);
}

void JTAG_get_tap_state_NULL_tap_state_check(void** state)
{
    assert_int_equal(JTAG_get_tap_state(*state, NULL), ST_ERR);
}

void JTAG_get_tap_state_returns_correct_state(void** state)
{
    JTAG_Handler* handler = *state;
    enum jtag_states jtag_state = jtag_rti;
    enum jtag_states expected = jtag_upd_dr;
    handler->active_chain->tap_state = expected;
    assert_int_equal(JTAG_get_tap_state(handler, &jtag_state), ST_OK);
    assert_int_equal(jtag_state, expected);
}

void JTAG_shift_NULL_state_check(void** state)
{
    (void)state; /* unused */
    unsigned int input_bytes = 5;
    unsigned char input[input_bytes];
    unsigned int output_bytes = 5;
    unsigned char output[output_bytes];
    assert_int_equal(JTAG_shift(NULL, 38, input_bytes, (unsigned char*)&input,
                                output_bytes, (unsigned char*)&output,
                                jtag_upd_dr),
                     ST_ERR);
}

void JTAG_shift_Shift_IR_sets_correct_scan_state(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = 0;
    handler->active_chain->shift_padding.irPost = 0;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_OK);
    assert_int_equal(handler->active_chain->scan_state, JTAGScanState_Done);
}

void JTAG_shift_Shift_IR_uses_correct_padding(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected_pre = 3;
    unsigned int expected_post = 7;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = expected_pre;
    handler->active_chain->shift_padding.irPost = expected_post;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for prefix shift
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    // expectations for main shift
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    // expectations for postfix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_OK);
}

void JTAG_shift_Shift_DR_uses_correct_padding(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected_pre = 4;
    unsigned int expected_post = 6;
    int expected_pad_data = 24;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_dr;
    handler->active_chain->shift_padding.drPre = expected_pre;
    handler->active_chain->shift_padding.drPost = expected_post;
    memset(handler->padDataOne, expected_pad_data, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for prefix shift
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    // expectations for main shift
    ioctl_arg_types[test_ioctl_index++] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    // expectations for postfix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_OK);
}

void JTAG_shift_handles_incorrect_state(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_tlr;
    assert_int_equal(JTAG_shift(handler, 38, input_bytes, input, output_bytes,
                                output, jtag_upd_dr),
                     ST_ERR);
}

void JTAG_shift_handles_prefix_ioctl_errors(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected_pre = 3;
    unsigned int expected_post = 7;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = expected_pre;
    handler->active_chain->shift_padding.irPost = expected_post;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for prefix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_ERR);
}

void JTAG_shift_handles_shift_ioctl_errors(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected_pre = 3;
    unsigned int expected_post = 7;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = expected_pre;
    handler->active_chain->shift_padding.irPost = expected_post;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for prefix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;

    // expectations for main shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_ERR);
}

void JTAG_shift_handles_postfix_ioctl_errors(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int expected_pre = 3;
    unsigned int expected_post = 7;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = expected_pre;
    handler->active_chain->shift_padding.irPost = expected_post;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for prefix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;

    // expectations for main shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = 0;

    // expectations for postfix shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_ERR);
}

void JTAG_shift_with_no_padding_handles_shift_ioctl_errors(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = 0;
    handler->active_chain->shift_padding.irPost = 0;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for main shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    FAKE_IOCTL_RESULT[test_ioctl_index++] = -1;

    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_ERR);
}

void JTAG_wait_cycles_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_wait_cycles(NULL, 38), ST_ERR);
}

void JTAG_wait_cycles_calls_correct_ioctl_correct_number_of_times(void** state)
{
    JTAG_Handler* handler = *state;
    int i = test_ioctl_index;
    int expected_times = MAX_IOCTLS;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    // expectations for wait cycles
    for (; i < (expected_times + test_ioctl_index); i++)
        ioctl_arg_types[i] = IoctlArgType_tck_bitbang;
#ifdef JTAG_LEGACY_DRIVER
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle,
                       expected_times);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, expected_times);
    expect_any_count(__wrap_ioctl, ioctl_arg_tck_bitbang, expected_times);
#else
    expect_value_count(__wrap_ioctl, fd, handler->JTAG_driver_handle, 1);
    expect_value_count(__wrap_ioctl, request, AST_JTAG_BITBANG, 1);
    expect_any_count(__wrap_ioctl, ioctl_arg_tck_bitbang, 1);
#endif
    assert_int_equal(JTAG_wait_cycles(handler, expected_times), ST_OK);
}

void JTAG_wait_cycles_ioctl_failed(void** state)
{
    JTAG_Handler* handler = *state;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    FAKE_IOCTL_RESULT[test_ioctl_index] = -1;
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_tck_bitbang;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_BITBANG);
    expect_any(__wrap_ioctl, ioctl_arg_tck_bitbang);
    assert_int_equal(JTAG_wait_cycles(handler, 1), ST_ERR);
}

void JTAG_wait_cycles_request_bigger_than_MAX_WAIT_CYCLES_failed(void** state)
{
    JTAG_Handler* handler = *state;
    handler->JTAG_driver_handle = 2;
    handler->sw_mode = true;
    assert_int_equal(JTAG_wait_cycles(handler, 300), ST_ERR);
}

void JTAG_set_jtag_tck_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_set_jtag_tck(NULL, 384), ST_ERR);
}

void JTAG_set_jtag_tck_correctly_calls_ioctl(void** state)
{
    JTAG_Handler* handler = *state;
    handler->JTAG_driver_handle = 2;
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_set_tck_param;
    FAKE_IOCTL_RESULT[test_ioctl_index] = 0;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TCK);
    expect_any(__wrap_ioctl, ioctl_arg_set_tck_param);
    assert_int_equal(JTAG_set_jtag_tck(handler, 1234), ST_OK);
}

void JTAG_set_jtag_tck_handles_ioctl_failure(void** state)
{
    JTAG_Handler* handler = *state;
    handler->JTAG_driver_handle = 2;
    FAKE_IOCTL_RESULT[test_ioctl_index] = -5;
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_set_tck_param;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_SET_TCK);
    expect_any(__wrap_ioctl, ioctl_arg_set_tck_param);
    assert_int_equal(JTAG_set_jtag_tck(handler, 1234), ST_ERR);
}

void JTAG_set_active_chain_NULL_state_check(void** state)
{
    (void)state; /* unused */
    assert_int_equal(JTAG_set_active_chain(NULL, SCAN_CHAIN_0), ST_ERR);
}

void JTAG_set_active_chain_invalid_max_chain_test(void** state)
{
    assert_int_equal(JTAG_set_active_chain(*state, MAX_SCAN_CHAINS), ST_ERR);
}

void JTAG_set_active_chain_invalid_less_than_0_chain_test(void** state)
{
    assert_int_equal(JTAG_set_active_chain(*state, -1), ST_ERR);
}

void JTAG_set_active_chain_successful_change_test(void** state)
{
    JTAG_Handler* handler = *state;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];

    assert_int_equal(JTAG_set_active_chain(handler, SCAN_CHAIN_1), ST_OK);
    assert_ptr_equal(handler->active_chain, &handler->chains[SCAN_CHAIN_1]);
}

void JTAG_set_active_chain_rti_state_test(void** state)
{
    JTAG_Handler* handler = *state;
    handler->chains[SCAN_CHAIN_1].tap_state = jtag_cap_dr;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    assert_int_equal(JTAG_set_active_chain(handler, SCAN_CHAIN_1), ST_OK);

    handler->chains[SCAN_CHAIN_1].tap_state = jtag_rti;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    assert_int_equal(JTAG_set_active_chain(handler, SCAN_CHAIN_1), ST_OK);
    assert_ptr_equal(handler->active_chain, &handler->chains[SCAN_CHAIN_1]);

    handler->chains[SCAN_CHAIN_1].tap_state = jtag_tlr;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    assert_int_equal(JTAG_set_active_chain(handler, SCAN_CHAIN_1), ST_OK);
    assert_ptr_equal(handler->active_chain, &handler->chains[SCAN_CHAIN_1]);
}

void JTAG_shift_Shift_IR_memcpy_error(void** state)
{
    JTAG_Handler* handler = *state;
    unsigned int input_bytes = TEST_BUFFER_SIZE;
    unsigned char input[input_bytes];
    unsigned int output_bytes = TEST_BUFFER_SIZE;
    unsigned char output[output_bytes];
    handler->JTAG_driver_handle = 2;
    handler->active_chain = &handler->chains[SCAN_CHAIN_0];
    handler->active_chain->tap_state = jtag_shf_ir;
    handler->active_chain->shift_padding.irPre = 0;
    handler->active_chain->shift_padding.irPost = 0;
    memset(handler->padDataOne, ~0, sizeof(handler->padDataOne));
    handler->active_chain->scan_state = JTAGScanState_Done;

    // expectations for shift
    ioctl_arg_types[test_ioctl_index] = IoctlArgType_scan_xfer;
    expect_value(__wrap_ioctl, fd, handler->JTAG_driver_handle);
    expect_value(__wrap_ioctl, request, AST_JTAG_READWRITESCAN);
    expect_any(__wrap_ioctl, ioctl_arg_scan_xfr);
    MEMCPY_SAFE_RESULT = 1;
    assert_int_equal(JTAG_shift(handler, 38, input_bytes,
                                (unsigned char*)&input, output_bytes,
                                (unsigned char*)&output, jtag_upd_dr),
                     ST_OK);
    assert_int_equal(handler->active_chain->scan_state, JTAGScanState_Done);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(JTAGHandler_malloc_failure),
        cmocka_unit_test_setup_teardown(JTAGHandler_initializes_padding, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            JTAGHandler_initializes_tap_state_to_TRL, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAGHandler_initializes_padDataOne_to_FFs, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAGHandler_initializes_padDataZero_to_0s, setup, teardown),
        cmocka_unit_test(JTAG_initialize_NULL_state_check),
        cmocka_unit_test_setup_teardown(JTAG_initialize_driver_open_failure,
                                        setup, teardown),
#ifndef JTAG_LEGACY_DRIVER
        cmocka_unit_test_setup_teardown(
            JTAG_initialize_sets_JTAG_set_mode_failure, setup, teardown),
#endif
        cmocka_unit_test_setup_teardown(
            JTAG_initialize_sets_JTAG_set_tap_state_to_TLR, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_initialize_handles_JTAG_set_tap_state_error, setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_initialize_clears_padding, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(JTAG_initialize_returns_ST_OK, setup,
                                        teardown),
        cmocka_unit_test(JTAG_deinitialize_NULL_state_check),
        cmocka_unit_test_setup_teardown(JTAG_deinitialize_success, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(JTAG_set_padding_param_checks, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(JTAG_shift_Shift_IR_memcpy_error, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_padding_handles_JTAGPaddingTypes_DRPre, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_padding_handles_JTAGPaddingTypes_DRPost, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_padding_handles_JTAGPaddingTypes_IRPre, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_padding_handles_JTAGPaddingTypes_IRPost, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_padding_handles_unknown_padding_type, setup, teardown),
        cmocka_unit_test(JTAG_tap_reset_NULL_state_check),
        cmocka_unit_test_setup_teardown(JTAG_tap_reset_sets_tap_state_to_TLR,
                                        setup, teardown),
        cmocka_unit_test(JTAG_set_tap_state_NULL_state_check),
        cmocka_unit_test_setup_teardown(
            JTAG_set_tap_state_calls_ioctl_with_correct_parameters, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_tap_state_sets_tap_state_correctly, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_tap_state_jtag_rti_wait_cycles_failed, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_tap_state_jtag_rti_execute_five_wait_cycles, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_tap_state_jtag_pau_dr_execute_five_wait_cycles, setup,
            teardown),
        cmocka_unit_test_setup_teardown(JTAG_set_tap_state_ioctl_failure, setup,
                                        teardown),
        cmocka_unit_test(JTAG_get_tap_state_NULL_state_check),
        cmocka_unit_test_setup_teardown(JTAG_get_tap_state_NULL_tap_state_check,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_get_tap_state_returns_correct_state, setup, teardown),
        cmocka_unit_test(JTAG_shift_NULL_state_check),
        cmocka_unit_test_setup_teardown(
            JTAG_shift_Shift_IR_sets_correct_scan_state, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_shift_Shift_IR_uses_correct_padding, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_shift_Shift_DR_uses_correct_padding, setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_shift_handles_incorrect_state,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_shift_handles_prefix_ioctl_errors,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_shift_handles_shift_ioctl_errors,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_shift_handles_postfix_ioctl_errors,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_shift_with_no_padding_handles_shift_ioctl_errors, setup,
            teardown),
        cmocka_unit_test(JTAG_wait_cycles_NULL_state_check),
        cmocka_unit_test_setup_teardown(
            JTAG_wait_cycles_calls_correct_ioctl_correct_number_of_times, setup,
            teardown),
        cmocka_unit_test_setup_teardown(JTAG_wait_cycles_ioctl_failed, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_wait_cycles_request_bigger_than_MAX_WAIT_CYCLES_failed, setup,
            teardown),
        cmocka_unit_test(JTAG_set_jtag_tck_NULL_state_check),
        cmocka_unit_test_setup_teardown(JTAG_set_jtag_tck_correctly_calls_ioctl,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_set_jtag_tck_handles_ioctl_failure,
                                        setup, teardown),
        cmocka_unit_test(JTAG_set_active_chain_NULL_state_check),
        cmocka_unit_test_setup_teardown(
            JTAG_set_active_chain_invalid_max_chain_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_active_chain_invalid_less_than_0_chain_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            JTAG_set_active_chain_successful_change_test, setup, teardown),
        cmocka_unit_test_setup_teardown(JTAG_set_active_chain_rti_state_test,
                                        setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
