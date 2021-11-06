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
#include <linux/i2c-dev.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../i2c_handler.h"
#include "../logging.h"
#include "cmocka.h"

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

int __real_open(const char* pathname, int flags, int mode);
int __wrap_open(const char* pathname, int flags, int mode)
{
    if (strlen(pathname) > 5 &&
        !strcmp(pathname + strlen(pathname) - 5, ".gcda"))
        return __real_open(pathname, flags, mode);

    check_expected_ptr(pathname);
    check_expected(flags);
    return FAKE_DRIVER_HANDLE;
}

int __wrap_close(int fd)
{
    check_expected(fd);
    return 0;
}

struct i2c_rdwr_ioctl_data* ioctl_data_ptr;
static int FAKE_IOCTL_RESULT = 0;
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    check_expected(fd);
    check_expected(request);

    va_list args;
    va_start(args, request);

    ioctl_data_ptr = va_arg(args, struct i2c_rdwr_ioctl_data*);
    check_expected_ptr(ioctl_data_ptr);

    va_end(args);

    return FAKE_IOCTL_RESULT;
}

void I2CHandler_null_parameter_test()
{
    I2C_Handler* handler = I2CHandler(NULL);
    assert_true(handler == NULL);
}

static i2c_config global_i2c_config;

void I2CHandler_create_test()
{
    I2C_Handler* handler = I2CHandler(&global_i2c_config);
    assert_true(handler != NULL);
    assert_true(sizeof(handler) != sizeof(I2C_Handler));
    assert_true(handler->i2c_driver_handle == UNINITIALIZED_I2C_DRIVER_HANDLE);
    free(handler);
}

void I2CHandler_malloc_fail_test()
{
    expect_value(__wrap_malloc, size, sizeof(I2C_Handler));
    malloc_fail = true;
    assert_null(I2CHandler(&global_i2c_config));
    malloc_fail = false;
}

void i2c_bus_select_null_state_test()
{
    assert_int_equal(i2c_bus_select(NULL, 1), ST_ERR);
}

void i2c_bus_select_i2c_disabled_test()
{
    global_i2c_config.enable_i2c = false;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_bus_select(&handler, 1), ST_ERR);
}

#define FAKE_FILE_NAME "/dev/i2c-19"
#define FAKE_BUS 19

void i2c_bus_select_open_test(void** state)
{
    global_i2c_config.default_bus = FAKE_BUS;
    for (int i = 0; i < MAX_I2C_BUSES; i++)
    {
        global_i2c_config.allowed_buses[i] = true;
    }

    I2C_Handler handler;
    handler.i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;
    global_i2c_config.enable_i2c = true;
    handler.config = &global_i2c_config;
    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, FAKE_FILE_NAME);
    expect_value(__wrap_open, flags, O_RDWR);

    STATUS status = i2c_bus_select(&handler, FAKE_BUS);
    assert_int_equal(status, ST_OK);
    assert_int_equal(handler.i2c_driver_handle, FAKE_DRIVER_HANDLE);
    assert_int_equal(handler.i2c_bus, FAKE_BUS);
}

void i2c_bus_select_i2c_open_driver_test(void** state)
{
    global_i2c_config.default_bus = FAKE_BUS;
    for (int i = 0; i < MAX_I2C_BUSES; i++)
    {
        global_i2c_config.allowed_buses[i] = true;
    }

    I2C_Handler handler;
    handler.i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;
    global_i2c_config.enable_i2c = true;
    handler.config = &global_i2c_config;
    FAKE_DRIVER_HANDLE = -1;
    expect_string(__wrap_open, pathname, FAKE_FILE_NAME);
    expect_value(__wrap_open, flags, O_RDWR);

    STATUS status = i2c_bus_select(&handler, FAKE_BUS);
    assert_int_equal(status, ST_ERR);
}

void i2c_bus_select_no_op_test()
{
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.i2c_bus = FAKE_BUS;
    handler.config = &global_i2c_config;
    STATUS status = i2c_bus_select(&handler, FAKE_BUS);
    assert_int_equal(status, ST_OK);
    assert_int_equal(handler.i2c_bus, FAKE_BUS);
}

void i2c_bus_select_close_test()
{
    global_i2c_config.default_bus = FAKE_BUS;
    global_i2c_config.enable_i2c = true;
    for (int i = 0; i < MAX_I2C_BUSES; i++)
    {
        global_i2c_config.allowed_buses[i] = true;
    }
    I2C_Handler handler;
    int expected = 22;
    handler.i2c_driver_handle = expected;
    handler.i2c_bus = 88;
    handler.config = &global_i2c_config;

    expect_value(__wrap_close, fd, expected);

    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, FAKE_FILE_NAME);
    expect_value(__wrap_open, flags, O_RDWR);

    STATUS status = i2c_bus_select(&handler, FAKE_BUS);
    assert_int_equal(status, ST_OK);
    assert_int_equal(handler.i2c_driver_handle, FAKE_DRIVER_HANDLE);
    assert_int_equal(handler.i2c_bus, FAKE_BUS);
}

void i2c_bus_select_disallowed_bus_test()
{
    for (int i = 0; i < MAX_I2C_BUSES; i++)
    {
        global_i2c_config.allowed_buses[i] = false;
    }
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_bus_select(&handler, 1), ST_ERR);
}

void i2c_bus_select_max_bus_test()
{
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_bus_select(&handler, 200), ST_ERR);
}

void i2c_initialize_null_state_test()
{
    assert_int_equal(i2c_initialize(NULL), ST_ERR);
}

void i2c_initialize_i2c_disabled_test()
{
    global_i2c_config.enable_i2c = false;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_initialize(&handler), ST_ERR);
}

void i2c_initialize_initializes_default_bus_test()
{
    for (int i = 0; i < MAX_I2C_BUSES; i++)
    {
        global_i2c_config.allowed_buses[i] = true;
    }
    global_i2c_config.default_bus = FAKE_BUS;
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    handler.i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;

    FAKE_DRIVER_HANDLE = 5;
    expect_string(__wrap_open, pathname, FAKE_FILE_NAME);
    expect_value(__wrap_open, flags, O_RDWR);

    STATUS status = i2c_initialize(&handler);
    assert_int_equal(status, ST_OK);
    assert_int_equal(handler.i2c_driver_handle, FAKE_DRIVER_HANDLE);
    assert_int_equal(handler.i2c_bus, FAKE_BUS);
}

void i2c_deinitialize_null_state_test()
{
    assert_int_equal(i2c_deinitialize(NULL), ST_ERR);
}

void i2c_deinitialize_closes_driver_test()
{
    I2C_Handler handler;
    int expected = 22;
    handler.i2c_driver_handle = expected;
    handler.i2c_bus = 88;

    expect_value(__wrap_close, fd, expected);

    assert_int_equal(i2c_deinitialize(&handler), ST_OK);
}

void i2c_set_sclk_null_state_test()
{
    assert_int_equal(i2c_set_sclk(NULL, 0), ST_ERR);
}

void i2c_set_sclk_i2c_enabled_test()
{
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_set_sclk(&handler, 0), ST_OK);
}

void i2c_set_sclk_i2c_not_enabled_test()
{
    global_i2c_config.enable_i2c = false;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_set_sclk(&handler, 0), ST_ERR);
}

void i2c_read_write_null_state_test()
{
    assert_int_equal(i2c_read_write(NULL, NULL), ST_ERR);
}

void i2c_read_write_i2c_not_enabled_test()
{
    global_i2c_config.enable_i2c = false;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_read_write(&handler, NULL), ST_ERR);
}

void i2c_read_write_null_msg_set_test()
{
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    assert_int_equal(i2c_read_write(&handler, NULL), ST_ERR);
}

void i2c_read_write_ioclt_fail_test()
{
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    struct i2c_rdwr_ioctl_data ioctl_data;
    FAKE_IOCTL_RESULT = 6;
    ioctl_data.nmsgs = (__u32)(FAKE_IOCTL_RESULT - 2);
    expect_value(__wrap_ioctl, fd, handler.i2c_driver_handle);
    expect_value(__wrap_ioctl, request, I2C_RDWR);
    expect_any(__wrap_ioctl, ioctl_data_ptr);
    assert_int_equal(i2c_read_write(&handler, &ioctl_data), ST_ERR);
}

void i2c_read_write_success_test()
{
    global_i2c_config.enable_i2c = true;
    I2C_Handler handler;
    handler.config = &global_i2c_config;
    struct i2c_rdwr_ioctl_data ioctl_data;
    FAKE_IOCTL_RESULT = 6;
    ioctl_data.nmsgs = (__u32)FAKE_IOCTL_RESULT;
    expect_value(__wrap_ioctl, fd, handler.i2c_driver_handle);
    expect_value(__wrap_ioctl, request, I2C_RDWR);
    expect_any(__wrap_ioctl, ioctl_data_ptr);
    assert_int_equal(i2c_read_write(&handler, &ioctl_data), ST_OK);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(I2CHandler_null_parameter_test),
        cmocka_unit_test(I2CHandler_create_test),
        cmocka_unit_test(I2CHandler_malloc_fail_test),
        cmocka_unit_test(i2c_bus_select_null_state_test),
        cmocka_unit_test(i2c_bus_select_i2c_disabled_test),
        cmocka_unit_test(i2c_bus_select_open_test),
        cmocka_unit_test(i2c_bus_select_i2c_open_driver_test),
        cmocka_unit_test(i2c_bus_select_no_op_test),
        cmocka_unit_test(i2c_bus_select_close_test),
        cmocka_unit_test(i2c_bus_select_disallowed_bus_test),
        cmocka_unit_test(i2c_bus_select_max_bus_test),
        cmocka_unit_test(i2c_initialize_null_state_test),
        cmocka_unit_test(i2c_initialize_i2c_disabled_test),
        cmocka_unit_test(i2c_initialize_initializes_default_bus_test),
        cmocka_unit_test(i2c_deinitialize_null_state_test),
        cmocka_unit_test(i2c_deinitialize_closes_driver_test),
        cmocka_unit_test(i2c_set_sclk_i2c_enabled_test),
        cmocka_unit_test(i2c_set_sclk_null_state_test),
        cmocka_unit_test(i2c_set_sclk_i2c_not_enabled_test),
        cmocka_unit_test(i2c_read_write_null_state_test),
        cmocka_unit_test(i2c_read_write_i2c_not_enabled_test),
        cmocka_unit_test(i2c_read_write_null_msg_set_test),
        cmocka_unit_test(i2c_read_write_ioclt_fail_test),
        cmocka_unit_test(i2c_read_write_success_test)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
