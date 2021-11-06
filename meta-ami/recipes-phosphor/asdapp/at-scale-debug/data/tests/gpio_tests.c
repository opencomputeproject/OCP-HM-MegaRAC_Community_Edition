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
#include <sys/stat.h>

#include "../gpio.h"
#include "../logging.h"
#include "cmocka.h"

int FAKE_FD[10];
int FAKE_FD_INDEX = 0;

int __real_open(const char* pathname, int flags, int mode);
int __wrap_open(const char* pathname, int flags, int mode)
{
    if (strlen(pathname) > 5 &&
        !strcmp(pathname + strlen(pathname) - 5, ".gcda"))
        return __real_open(pathname, flags, mode);

    check_expected_ptr(pathname);
    check_expected(flags);
    check_expected(mode);
    return FAKE_FD[FAKE_FD_INDEX++];
}

int CLOSE_RESULT = 0;
int __wrap_close(int fd)
{
    check_expected(fd);
    return CLOSE_RESULT;
}

ssize_t WRITE_RESULT = 0;
ssize_t __wrap_write(int fd, const void* buf, size_t size)
{
    check_expected(fd);
    check_expected_ptr(buf);
    check_expected(size);
    return WRITE_RESULT;
}

ssize_t READ_RESULT = 0;
char FAKE_READ_VALUE[256];
ssize_t __wrap_read(int fd, void* buf, size_t size)
{
    check_expected(fd);
    check_expected_ptr(buf);
    check_expected(size);
    memcpy(buf, &FAKE_READ_VALUE, size);
    return READ_RESULT;
}

off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    check_expected(fd);
    check_expected(offset);
    check_expected(whence);
    return 0;
}

void gpio_export_invalid_param_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, gpio_export(99, NULL));
}

void gpio_export_already_exported_test(void** state)
{
    (void)state;
    int expected_gpio = 87;
    int actual_fd;
    int expected_fd = 88;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = expected_fd;
    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_OK, gpio_export(expected_gpio, &actual_fd));
    assert_int_equal(expected_fd, actual_fd);
}

void gpio_export_open_fail_test(void** state)
{
    (void)state;
    int expected_gpio = 87;
    int actual_fd = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/export");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = -1;

    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR, gpio_export(expected_gpio, &actual_fd));
    assert_int_equal(-1, actual_fd);
}

void gpio_export_fail_test(void** state)
{
    (void)state;
    int expected_gpio = 66;
    int expected_fd = 77;
    int actual_fd = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/export");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = expected_fd;

    expect_value(__wrap_write, fd, expected_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = -1; // fail the write

    expect_value(__wrap_close, fd, FAKE_FD[1]);

    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR, gpio_export(expected_gpio, &actual_fd));
    assert_int_equal(-1, actual_fd);
}

void gpio_export_failed_to_get_fd_after_export_test(void** state)
{
    (void)state;
    int expected_gpio = 66;
    int expected_export_fd = 77;
    int actual_fd;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/export");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = expected_export_fd;

    expect_value(__wrap_write, fd, expected_export_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = 2; // fail the write

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
    FAKE_FD[2] = -1;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_close, fd, FAKE_FD[1]);

    assert_int_equal(ST_ERR, gpio_export(expected_gpio, &actual_fd));
    assert_int_equal(-1, actual_fd);
}

void gpio_export_success_test(void** state)
{
    (void)state;
    int expected_gpio = 66;
    int expected_export_fd = 77;
    int expected_fd = 77;
    int actual_fd;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/export");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = expected_export_fd;

    expect_value(__wrap_write, fd, expected_export_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = 2; // fail the write

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_RDWR);
    expect_any(__wrap_open, mode);
    FAKE_FD[2] = expected_fd;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_close, fd, FAKE_FD[1]);

    assert_int_equal(ST_OK, gpio_export(expected_gpio, &actual_fd));
    assert_int_equal(expected_fd, actual_fd);
}

void gpio_unexport_not_exported_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;
    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_OK, gpio_unexport(expected_gpio));
}

void gpio_unexport_open_fail_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 99;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    expect_string(__wrap_open, pathname, "/sys/class/gpio/unexport");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = -1;

    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR, gpio_unexport(expected_gpio));
}

void gpio_unexport_fail_test(void** state)
{
    (void)state;
    int expected_gpio = 66;
    int expected_fd = 77;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 99;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    expect_string(__wrap_open, pathname, "/sys/class/gpio/unexport");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = expected_fd;

    expect_value(__wrap_write, fd, expected_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = -1; // fail the write

    expect_value(__wrap_close, fd, FAKE_FD[1]);

    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR, gpio_unexport(expected_gpio));
}

void gpio_unexport_success_test(void** state)
{
    (void)state;
    int expected_gpio = 66;
    int expected_fd = 77;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio66/value");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 44;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    expect_string(__wrap_open, pathname, "/sys/class/gpio/unexport");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[1] = expected_fd;

    expect_value(__wrap_write, fd, expected_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = 2; // fail the write

    FAKE_FD_INDEX = 0;

    expect_value(__wrap_close, fd, FAKE_FD[1]);

    assert_int_equal(ST_OK, gpio_unexport(expected_gpio));
}

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

void gpio_get_value_invalid_parameter_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, gpio_get_value(78, NULL));
}

void gpio_get_value_success_asserted_test(void** state)
{
    (void)state;
    int actual_value = 0;
    int expected_fd = 3;
    int expected_value = 1;

    expect_value(__wrap_lseek, fd, expected_fd);
    expect_value(__wrap_lseek, offset, 0);
    expect_value(__wrap_lseek, whence, SEEK_SET);

    expect_value(__wrap_read, fd, expected_fd);
    expect_any(__wrap_read, buf);
    expect_value(__wrap_read, size, 1);
    memset(&FAKE_READ_VALUE, 0, sizeof(FAKE_READ_VALUE));
    memcpy(&FAKE_READ_VALUE, "1", 1);

    assert_int_equal(ST_OK, gpio_get_value(expected_fd, &actual_value));
    assert_int_equal(expected_value, actual_value);
}

void gpio_get_value_success_deasserted_test(void** state)
{
    (void)state;
    int actual_value = 0;
    int expected_fd = 3;
    int expected_value = 0;

    expect_value(__wrap_lseek, fd, expected_fd);
    expect_value(__wrap_lseek, offset, 0);
    expect_value(__wrap_lseek, whence, SEEK_SET);

    expect_value(__wrap_read, fd, expected_fd);
    expect_any(__wrap_read, buf);
    expect_value(__wrap_read, size, 1);
    memset(&FAKE_READ_VALUE, 0, sizeof(FAKE_READ_VALUE));
    memcpy(&FAKE_READ_VALUE, "0", 1);

    assert_int_equal(ST_OK, gpio_get_value(expected_fd, &actual_value));
    assert_int_equal(expected_value, actual_value);
}

void gpio_set_value_write_fail_test(void** state)
{
    (void)state;
    int expected_value = 0;
    int expected_fd = 87;

    expect_value(__wrap_lseek, fd, expected_fd);
    expect_value(__wrap_lseek, offset, 0);
    expect_value(__wrap_lseek, whence, SEEK_SET);

    expect_value(__wrap_write, fd, expected_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, sizeof(char));
    WRITE_RESULT = -1; // fail the write

    assert_int_equal(ST_ERR, gpio_set_value(expected_fd, expected_value));
}

void gpio_set_value_success_test(void** state)
{
    (void)state;
    int expected_value = 0;
    int expected_fd = 87;

    expect_value(__wrap_lseek, fd, expected_fd);
    expect_value(__wrap_lseek, offset, 0);
    expect_value(__wrap_lseek, whence, SEEK_SET);

    expect_value(__wrap_write, fd, expected_fd);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, sizeof(char));
    WRITE_RESULT = sizeof(char); // fail the write

    assert_int_equal(ST_OK, gpio_set_value(expected_fd, expected_value));
}

void gpio_set_edge_doesnt_exist_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/edge");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;
    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR, gpio_set_edge(expected_gpio, GPIO_EDGE_NONE));
}

void gpio_set_edge_none_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/edge");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "none");
    expect_value(__wrap_write, size, 4);
    WRITE_RESULT = 4;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK, gpio_set_edge(expected_gpio, GPIO_EDGE_NONE));
}

void gpio_set_edge_rising_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/edge");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "rising");
    expect_value(__wrap_write, size, 6);
    WRITE_RESULT = 4;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK, gpio_set_edge(expected_gpio, GPIO_EDGE_RISING));
}

void gpio_set_edge_falling_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/edge");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "falling");
    expect_value(__wrap_write, size, 7);
    WRITE_RESULT = 4;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK, gpio_set_edge(expected_gpio, GPIO_EDGE_FALLING));
}

void gpio_set_edge_both_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/edge");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "both");
    expect_value(__wrap_write, size, 4);
    WRITE_RESULT = 4;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK, gpio_set_edge(expected_gpio, GPIO_EDGE_BOTH));
}

void gpio_set_direction_doesnt_exist_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/direction");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;
    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR,
                     gpio_set_direction(expected_gpio, GPIO_DIRECTION_IN));
}

void gpio_set_direction_in_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/direction");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "in");
    expect_value(__wrap_write, size, 2);
    WRITE_RESULT = 2;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK,
                     gpio_set_direction(expected_gpio, GPIO_DIRECTION_IN));
}

void gpio_set_direction_out_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/direction");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "out");
    expect_value(__wrap_write, size, 3);
    WRITE_RESULT = 3;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK,
                     gpio_set_direction(expected_gpio, GPIO_DIRECTION_OUT));
}

void gpio_set_direction_low_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/direction");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "low");
    expect_value(__wrap_write, size, 3);
    WRITE_RESULT = 3;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK,
                     gpio_set_direction(expected_gpio, GPIO_DIRECTION_LOW));
}

void gpio_set_direction_high_test(void** state)
{
    (void)state;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/direction");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 2;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_string(__wrap_write, buf, "high");
    expect_value(__wrap_write, size, 4);
    WRITE_RESULT = 4;

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK,
                     gpio_set_direction(expected_gpio, GPIO_DIRECTION_HIGH));
}

void gpio_set_active_low_doesnt_exist_test(void** state)
{
    (void)state;
    bool expected_active_low = false;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/active_low");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = -1;
    FAKE_FD_INDEX = 0;

    assert_int_equal(ST_ERR,
                     gpio_set_active_low(expected_gpio, expected_active_low));
}

void gpio_set_active_low_write_fail_test(void** state)
{
    (void)state;
    bool expected_active_low = false;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/active_low");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 33;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, sizeof(expected_active_low));
    WRITE_RESULT = -1; // fail the write

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_ERR,
                     gpio_set_active_low(expected_gpio, expected_active_low));
}

void gpio_set_active_low_success_test(void** state)
{
    (void)state;
    bool expected_active_low = false;
    uint8_t expected_active_low_raw = 0;
    int expected_gpio = 87;

    expect_string(__wrap_open, pathname, "/sys/class/gpio/gpio87/active_low");
    expect_value(__wrap_open, flags, O_WRONLY);
    expect_any(__wrap_open, mode);
    FAKE_FD[0] = 33;
    FAKE_FD_INDEX = 0;

    expect_value(__wrap_write, fd, FAKE_FD[0]);
    expect_any(__wrap_write, buf);
    expect_value(__wrap_write, size, sizeof(expected_active_low_raw));
    WRITE_RESULT = sizeof(expected_active_low_raw); // fail the write

    expect_value(__wrap_close, fd, FAKE_FD[0]);

    assert_int_equal(ST_OK,
                     gpio_set_active_low(expected_gpio, expected_active_low));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(gpio_export_invalid_param_test),
        cmocka_unit_test(gpio_export_already_exported_test),
        cmocka_unit_test(gpio_export_open_fail_test),
        cmocka_unit_test(gpio_export_fail_test),
        cmocka_unit_test(gpio_export_failed_to_get_fd_after_export_test),
        cmocka_unit_test(gpio_export_success_test),
        cmocka_unit_test(gpio_unexport_not_exported_test),
        cmocka_unit_test(gpio_unexport_open_fail_test),
        cmocka_unit_test(gpio_unexport_fail_test),
        cmocka_unit_test(gpio_unexport_success_test),
        cmocka_unit_test(gpio_get_value_invalid_parameter_test),
        cmocka_unit_test(gpio_get_value_success_asserted_test),
        cmocka_unit_test(gpio_get_value_success_deasserted_test),
        cmocka_unit_test(gpio_set_value_write_fail_test),
        cmocka_unit_test(gpio_set_value_success_test),
        cmocka_unit_test(gpio_set_edge_doesnt_exist_test),
        cmocka_unit_test(gpio_set_edge_none_test),
        cmocka_unit_test(gpio_set_edge_rising_test),
        cmocka_unit_test(gpio_set_edge_falling_test),
        cmocka_unit_test(gpio_set_edge_both_test),
        cmocka_unit_test(gpio_set_direction_doesnt_exist_test),
        cmocka_unit_test(gpio_set_direction_in_test),
        cmocka_unit_test(gpio_set_direction_out_test),
        cmocka_unit_test(gpio_set_direction_low_test),
        cmocka_unit_test(gpio_set_direction_high_test),
        cmocka_unit_test(gpio_set_active_low_doesnt_exist_test),
        cmocka_unit_test(gpio_set_active_low_write_fail_test),
        cmocka_unit_test(gpio_set_active_low_success_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
