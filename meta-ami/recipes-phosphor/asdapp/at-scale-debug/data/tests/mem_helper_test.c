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

#include <getopt.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <time.h>

#include "../logging.h"
#include "../mem_helper.h"
#include "cmocka.h"

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

void memcpy_success_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char src[] = "test";
    char dest[8];

    assert_int_equal(0, memcpy_safe(dest, Destsize, src, Sourcesize));
}

void memcpy_success_src_null_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char* src = NULL;
    char dest[8];

    assert_int_equal(1, memcpy_safe(dest, Destsize, src, Sourcesize));
}

void memcpy_success_dest_null_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char src[] = "test";
    char* dest = NULL;

    assert_int_equal(1, memcpy_safe(dest, Destsize, src, Sourcesize));
}

void memcpy_success_size_fail_test(void** state)
{
    (void)state;
    size_t Sourcesize = 1;
    size_t Destsize = 8;
    char src[] = "test";
    char dest[8];

    assert_int_equal(0, memcpy_safe(dest, Destsize, src, Sourcesize));
}

void strcpy_success_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char src[] = "test";
    char dest[8];

    assert_int_equal(0, strcpy_safe(dest, Destsize, src, Sourcesize));
}

void strcpy_success_src_null_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char* src = NULL;
    char dest[8];

    assert_int_equal(1, strcpy_safe(dest, Destsize, src, Sourcesize));
}

void strcpy_success_dest_null_test(void** state)
{
    (void)state;
    size_t Sourcesize = 8;
    size_t Destsize = 8;
    char src[] = "test";
    char* dest = NULL;

    assert_int_equal(1, strcpy_safe(dest, Destsize, src, Sourcesize));
}

void strcpy_success_size_fail_test(void** state)
{
    (void)state;
    size_t Sourcesize = 1;
    size_t Destsize = 8;
    char src[] = "test";
    char dest[8];

    assert_int_equal(0, strcpy_safe(dest, Destsize, src, Sourcesize));
}

void snprintf_safe_success_test(void** state)
{
    (void)state;
    char buffer[10];
    int array[] = {1};

    assert_int_equal(0, snprintf_safe(buffer, 10, "abcdefgh%d", array, 1));
}

void snprintf_safe_success_test2(void** state)
{
    (void)state;
    char buffer[35] = {};
    char text[] = "/sys/class/gpio/gpio22/edge";
    int array[] = {22};
    assert_int_equal(
        0, snprintf_safe(buffer, 35, "/sys/class/gpio/gpio%d/edge", array, 1));
    assert_memory_equal(buffer, text, sizeof(text));
}

void snprintf_safe_success_mul_test2(void** state)
{
    (void)state;
    char buffer[35] = {};
    char text[] = "/sys/class/gpio/gpio22/edge35/";
    int array[] = {22, 35};
    assert_int_equal(
        0,
        snprintf_safe(buffer, 35, "/sys/class/gpio/gpio%d/edge%d/", array, 2));
    assert_memory_equal(buffer, text, sizeof(text));
}

void snprintf_safe_failure(void** state)
{
    (void)state;
    char buffer[35] = {};
    char text[] = "/sys/class/gpio/gpio22/edge";
    int array[] = {32};
    assert_int_equal(
        0, snprintf_safe(buffer, 2, "/sys/class/gpio/gpio%d/edge", array, 1));
    assert_memory_not_equal(buffer, text, sizeof(text));
}

void snprintf_safe_failure_integer_zero(void** state)
{
    (void)state;
    char buffer[35] = {};
    char text[] = "/sys/class/gpio/gpio22/edge";
    int array[] = {32};
    assert_int_equal(
        1, snprintf_safe(buffer, 35, "/sys/class/gpio/gpio%d/edge", array, 0));
    assert_memory_not_equal(buffer, text, sizeof(text));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(memcpy_success_test),
        cmocka_unit_test(memcpy_success_src_null_test),
        cmocka_unit_test(memcpy_success_dest_null_test),
        cmocka_unit_test(memcpy_success_size_fail_test),
        cmocka_unit_test(strcpy_success_test),
        cmocka_unit_test(strcpy_success_src_null_test),
        cmocka_unit_test(strcpy_success_dest_null_test),
        cmocka_unit_test(strcpy_success_size_fail_test),
        cmocka_unit_test(snprintf_safe_success_test),
        cmocka_unit_test(snprintf_safe_success_test2),
        cmocka_unit_test(snprintf_safe_success_mul_test2),
        cmocka_unit_test(snprintf_safe_failure),
        cmocka_unit_test(snprintf_safe_failure_integer_zero),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}