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

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "../logging.h"
#include "cmocka.h"

bool shouldRemoteLogResponse = false;
bool shouldRemoteLogCalled = false;
static bool shouldRemoteLog(ASD_LogLevel level, ASD_LogStream stream)
{
    (void)level;
    (void)stream;
    shouldRemoteLogCalled = true;
    return shouldRemoteLogResponse;
}

ASD_LogLevel remoteMessageCalledWithLevel = ASD_LogLevel_Off;
ASD_LogStream remoteMessageCalledWithStream = ASD_LogStream_None;
char remoteMessageCalledWithMessage[256] = "";
static void sendRemoteLoggingMessage(ASD_LogLevel level, ASD_LogStream stream,
                                     const char* message)
{
    remoteMessageCalledWithLevel = level;
    remoteMessageCalledWithStream = stream;
    memcpy(&remoteMessageCalledWithMessage, message, strlen(message) + 1);
}

void __wrap_vsyslog(int priority, const char* format, ...)
{
    check_expected(priority);
    check_expected_ptr(format);
}

static char temporary_syslog_buffer[256];
void __wrap_syslog(int priority, const char* format, ...)
{
    check_expected(priority);
    check_expected_ptr(format);

    va_list args;
    va_start(args, format);
    vsnprintf(temporary_syslog_buffer, sizeof(temporary_syslog_buffer), format,
              args);
    check_expected(temporary_syslog_buffer);
    va_end(args);
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

static char temporary_vfprintf_buffer[1024];
int __wrap_vfprintf(FILE* const file, const char* format, va_list ap)
{
    check_expected_ptr(file);
    check_expected_ptr(format);

    vsnprintf(temporary_vfprintf_buffer, sizeof(temporary_vfprintf_buffer),
              format, ap);
    check_expected(temporary_vfprintf_buffer);
    return 0;
}

static int setup(void** state)
{
    (void)state;
    malloc_fail = false;
    asd_log_level = ASD_LogLevel_Off;
    asd_log_streams = ASD_LogStream_None;
    remoteMessageCalledWithLevel = ASD_LogLevel_Off;
    remoteMessageCalledWithStream = ASD_LogStream_None;
    shouldRemoteLogResponse = false;
    shouldRemoteLogCalled = false;
    return 0;
}

static int teardown(void** state)
{
    (void)state;
    return 0;
}

void ASD_log_writes_to_syslog_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Error, ASD_LogStream_All, true,
                                NULL, NULL);
    const char* expected = "some log message text";
    expect_value(__wrap_vsyslog, priority, LOG_USER);
    expect_string(__wrap_vsyslog, format, expected);
    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
            expected);
}

void ASD_log_writes_to_stderr_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Error, ASD_LogStream_All, false,
                                NULL, NULL);
    const char* expected = "some log message text";
    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);
    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
            expected);
}

void ASD_log_writes_to_logging_callback_test(void** state)
{
    (void)state;
    shouldRemoteLogResponse = true;
    memset(&remoteMessageCalledWithMessage[0], 0,
           sizeof(remoteMessageCalledWithMessage));
    const char* expected = "some log message text";
    ASD_initialize_log_settings(ASD_LogLevel_Error, ASD_LogStream_JTAG, false,
                                shouldRemoteLog, sendRemoteLoggingMessage);
    ASD_log(ASD_LogLevel_Trace, ASD_LogStream_JTAG, ASD_LogOption_None,
            expected);
    assert_true(shouldRemoteLogCalled);
    assert_true(remoteMessageCalledWithLevel == ASD_LogLevel_Trace);
    assert_true(remoteMessageCalledWithStream == ASD_LogStream_JTAG);
    assert_string_equal(expected, &remoteMessageCalledWithMessage);
}

void ASD_log_writes_to_logging_callback_and_stderr_test(void** state)
{
    (void)state;
    asd_log_level = ASD_LogLevel_Error;
    asd_log_streams = ASD_LogStream_Network;
    shouldRemoteLogResponse = true;
    memset(&remoteMessageCalledWithMessage[0], 0,
           sizeof(remoteMessageCalledWithMessage));
    const char* expected = "some log message text";
    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);
    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
            expected);
    assert_true(shouldRemoteLogCalled);
    assert_true(remoteMessageCalledWithLevel == ASD_LogLevel_Error);
    assert_true(remoteMessageCalledWithStream == ASD_LogStream_Network);
    assert_string_equal(expected, &remoteMessageCalledWithMessage);
}

void ASD_log_buffer_writes_to_syslog_with_undersize_prefix_test(void** state)
{
    (void)state;
    const char* prefix = "TEST";
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    expect_value(__wrap_syslog, priority, LOG_USER);
    expect_string(__wrap_syslog, format, "%s");
    expect_string(__wrap_syslog, temporary_syslog_buffer,
                  "TEST  : 0000000: 1020 30\n");
    ASD_initialize_log_settings(ASD_LogLevel_Warning, ASD_LogStream_Pins, true,
                                NULL, NULL);
    ASD_log_buffer(ASD_LogLevel_Warning, ASD_LogStream_Pins, ASD_LogOption_None,
                   &buffer[0], length, prefix);
}

void ASD_log_buffer_writes_to_syslog_with_max_prefix_test(void** state)
{
    (void)state;
    const char* prefix = "TESTTE";
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    expect_value(__wrap_syslog, priority, LOG_USER);
    expect_string(__wrap_syslog, format, "%s");
    expect_string(__wrap_syslog, temporary_syslog_buffer,
                  "TESTTE: 0000000: 1020 30\n");
    ASD_initialize_log_settings(ASD_LogLevel_Info, ASD_LogStream_I2C, true,
                                NULL, NULL);
    ASD_log_buffer(ASD_LogLevel_Info, ASD_LogStream_I2C, ASD_LogOption_None,
                   &buffer[0], length, prefix);
}

void ASD_log_buffer_writes_to_syslog_with_oversize_prefix_test(void** state)
{
    (void)state;
    const char* prefix = "TESTTEST";
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    expect_value(__wrap_syslog, priority, LOG_USER);
    expect_string(__wrap_syslog, format, "%s");
    expect_string(__wrap_syslog, temporary_syslog_buffer,
                  "TESTTE: 0000000: 1020 30\n");
    ASD_initialize_log_settings(ASD_LogLevel_Debug, ASD_LogStream_Test, true,
                                NULL, NULL);
    ASD_log_buffer(ASD_LogLevel_Debug, ASD_LogStream_Test, ASD_LogOption_None,
                   &buffer[0], length, prefix);
}

void ASD_log_buffer_writes_to_logging_callback_test(void** state)
{
    (void)state;
    shouldRemoteLogResponse = true;
    memset(&remoteMessageCalledWithMessage[0], 0,
           sizeof(remoteMessageCalledWithMessage));
    const char* expected = "TEST  : 0000000: 1020 30";
    const char* prefix = "TEST";
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    ASD_initialize_log_settings(ASD_LogLevel_Trace, ASD_LogStream_All, false,
                                shouldRemoteLog, sendRemoteLoggingMessage);
    ASD_log_buffer(ASD_LogLevel_Trace, ASD_LogStream_JTAG, ASD_LogOption_None,
                   &buffer[0], length, prefix);
    assert_true(shouldRemoteLogCalled);
    assert_true(remoteMessageCalledWithLevel == ASD_LogLevel_Trace);
    assert_true(remoteMessageCalledWithStream == ASD_LogStream_JTAG);
    assert_string_equal(expected, &remoteMessageCalledWithMessage);
}

void ASD_log_buffer_doesnt_log_test(void** state)
{
    (void)state;
    shouldRemoteLogResponse = false;
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    ASD_initialize_log_settings(ASD_LogLevel_Off, ASD_LogStream_None, false,
                                shouldRemoteLog, sendRemoteLoggingMessage);
    // test will have unmet expectations if test fails.
    ASD_log_buffer(ASD_LogLevel_Trace, ASD_LogStream_JTAG, ASD_LogOption_None,
                   &buffer[0], length, "TEST");
}

void ASD_log_buffer_writes_to_stderr_test(void** state)
{
    (void)state;
    memset(&remoteMessageCalledWithMessage[0], 0,
           sizeof(remoteMessageCalledWithMessage));
    const char* prefix = "TEST";
    size_t length = 3;
    const unsigned char buffer[] = {16, 32, 48};
    ASD_initialize_log_settings(ASD_LogLevel_Trace, ASD_LogStream_JTAG, false,
                                NULL, NULL);

    // It seems that we cannot mock fprintf with cmocka.
    // Perhaps cmocka is using fprintf and so it fails to link the mock
    // correctly.
    ASD_log_buffer(ASD_LogLevel_Trace, ASD_LogStream_JTAG, ASD_LogOption_None,
                   &buffer[0], length, prefix);
}

void ASD_initialize_log_settings_IRDR_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Trace, ASD_LogStream_JTAG, false,
                                NULL, NULL);
    assert_int_equal(asd_log_level, ASD_LogLevel_Trace);
    assert_int_equal(asd_log_streams, ASD_LogStream_JTAG);
}

void ASD_initialize_log_settings_all_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Debug, ASD_LogStream_All, false,
                                NULL, NULL);
    assert_int_equal(asd_log_level, ASD_LogLevel_Debug);
    assert_int_equal(asd_log_streams, ASD_LogStream_All);
}

void ASD_log_shift_invalid_params_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Debug, ASD_LogStream_All, false,
                                NULL, NULL);
    unsigned char data[1024];
    memset(&data, ~0, sizeof(data));
    // these function calls should return without doing anything.
    // if they do try to actually log, they will be met with failed
    // mock expectations and will fail.
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None, 0,
                  sizeof(data), (unsigned char*)&data, "blah");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None, 1,
                  0, (unsigned char*)&data, "blah");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None, 1,
                  sizeof(data), NULL, "blah");
}

void ASD_log_shift_malloc_failure_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Debug, ASD_LogStream_All, false,
                                NULL, NULL);
    unsigned char data[1024];
    memset(&data, ~0, sizeof(data));
    expect_value(__wrap_malloc, size, (sizeof(data) * 2) + 1);
    malloc_fail = true;
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None, 1,
                  sizeof(data), (unsigned char*)&data, "blah");
}

void ASD_log_shift_writes_correct_log_messages_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Trace, ASD_LogStream_All, false,
                                NULL, NULL);
    unsigned char data[1024];
    memset(&data, ~0, sizeof(data));
    const char* expected_format = "%s: [%db] 0x%s";

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift DR TDI: [10b] 0x3ff");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                  10, sizeof(data), (unsigned char*)&data, "Shift DR TDI");

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift IR TDI: [100b] 0xfffffffffffffffffffffffff");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                  100, sizeof(data), (unsigned char*)&data, "Shift IR TDI");

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift IR TDO: [1b] 0x1");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_Pins, ASD_LogOption_None, 1,
                  sizeof(data), (unsigned char*)&data, "Shift IR TDO");

    // In this next case, 10 bits is requested, but only 1 byte is passed
    // in. It should log the 8bits.
    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift DR TDO: [8b] 0xff");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_Test, ASD_LogOption_None,
                  10, 1, (unsigned char*)&data, "Shift DR TDO");

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift IR TDO: [1024b] 0xffffffffffffffffffffffffffffffff"
                  "ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
                  "ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
                  "ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
                  "ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                  1024, sizeof(data), (unsigned char*)&data, "Shift IR TDO");

    memset(&data, 0, sizeof(data));

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift IR TDI: [100b] 0x0000000000000000000000000");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_Test, ASD_LogOption_None,
                  100, sizeof(data), (unsigned char*)&data, "Shift IR TDI");

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected_format);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer,
                  "Shift DR TDO: [1b] 0x0");
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_Test, ASD_LogOption_None, 1,
                  sizeof(data), (unsigned char*)&data, "Shift DR TDO");
}

void ASD_log_shift_doesnt_log_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Off, ASD_LogStream_None, false,
                                shouldRemoteLog, sendRemoteLoggingMessage);
    unsigned char data[1024];
    memset(&data, ~0, sizeof(data));
    ASD_log_shift(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                  10, sizeof(data), (unsigned char*)&data, "Shift DR TDI");
}

void ASD_should_log_correctly_filters_messages_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Trace, ASD_LogStream_All, false,
                                NULL, NULL);

    const char* expected = "some log message text";
    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);

    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Test, ASD_LogOption_None,
            expected);

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);

    ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Test, ASD_LogOption_None,
            expected);

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);

    ASD_log(ASD_LogLevel_Info, ASD_LogStream_Test, ASD_LogOption_None,
            expected);

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);

    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Test, ASD_LogOption_None,
            expected);

    expect_value(__wrap_vfprintf, file, stderr);
    expect_string(__wrap_vfprintf, format, expected);
    expect_string(__wrap_vfprintf, temporary_vfprintf_buffer, expected);

    ASD_log(ASD_LogLevel_Trace, ASD_LogStream_Test, ASD_LogOption_None,
            expected);
}

void ASD_should_log_honors_no_remote_option_test(void** state)
{
    (void)state;
    ASD_initialize_log_settings(ASD_LogLevel_Error, ASD_LogStream_All, false,
                                NULL, NULL);
    shouldRemoteLogResponse = true;
    const char* expected = "some log message text";
    ASD_log(ASD_LogLevel_Trace, ASD_LogStream_Test, ASD_LogOption_No_Remote,
            expected);
    assert_false(shouldRemoteLogCalled);
}

void strtolevel_test(void** state)
{
    (void)state;
    ASD_LogLevel level;
    assert_false(strtolevel(NULL, NULL));
    assert_false(strtolevel("", NULL));
    assert_false(strtolevel(NULL, &level));
    assert_false(strtolevel("test", &level));
    assert_true(strtolevel("OfF", &level));
    assert_int_equal(level, ASD_LogLevel_Off);
    assert_true(strtolevel("TrAcE", &level));
    assert_int_equal(level, ASD_LogLevel_Trace);
    assert_true(strtolevel("DeBuG", &level));
    assert_int_equal(level, ASD_LogLevel_Debug);
    assert_true(strtolevel("InFo", &level));
    assert_int_equal(level, ASD_LogLevel_Info);
    assert_true(strtolevel("WaRnInG", &level));
    assert_int_equal(level, ASD_LogLevel_Warning);
    assert_true(strtolevel("ErRor", &level));
    assert_int_equal(level, ASD_LogLevel_Error);
}

void strtostreams_test(void** state)
{
    (void)state;
    ASD_LogStream streams;
    assert_false(strtostreams(NULL, NULL));
    assert_false(strtostreams("", NULL));
    assert_false(strtostreams(NULL, &streams));
    assert_false(strtostreams("unknown", &streams));
    assert_true(strtostreams("nOne", &streams));
    assert_int_equal(streams, ASD_LogStream_None);
    assert_true(strtostreams("NetWorK", &streams));
    assert_int_equal(streams, ASD_LogStream_Network);
    assert_true(strtostreams("JTAG", &streams));
    assert_int_equal(streams, ASD_LogStream_JTAG);
    assert_true(strtostreams("pins", &streams));
    assert_int_equal(streams, ASD_LogStream_Pins);
    assert_true(strtostreams("i2C", &streams));
    assert_int_equal(streams, ASD_LogStream_I2C);
    assert_true(strtostreams("TEst", &streams));
    assert_int_equal(streams, ASD_LogStream_Test);
    assert_true(strtostreams("dAemon", &streams));
    assert_int_equal(streams, ASD_LogStream_Daemon);
    assert_true(strtostreams("SDK", &streams));
    assert_int_equal(streams, ASD_LogStream_SDK);
    assert_true(strtostreams("All", &streams));
    assert_int_equal(streams, ASD_LogStream_All);
    assert_true(strtostreams("Network,JTAG", &streams));
    assert_int_equal(streams, ASD_LogStream_Network | ASD_LogStream_JTAG);
    assert_true(strtostreams("Network,Network", &streams));
    assert_int_equal(streams, ASD_LogStream_Network);
    assert_false(strtostreams("Network,JTAG,unknown", &streams));
    assert_false(strtostreams("Network,JTAG,unknownthatistoolong", &streams));
    assert_false(strtostreams("Network,JTAG,,Pins", &streams));
}

void streamtostring_test(void** state)
{
    (void)state;
    assert_string_equal(streamtostring(ASD_LogStream_None), "None");
    assert_string_equal(streamtostring(ASD_LogStream_Network), "Network");
    assert_string_equal(streamtostring(ASD_LogStream_JTAG), "JTAG");
    assert_string_equal(streamtostring(ASD_LogStream_Pins), "Pins");
    assert_string_equal(streamtostring(ASD_LogStream_I2C), "I2C");
    assert_string_equal(streamtostring(ASD_LogStream_Test), "Test");
    assert_string_equal(streamtostring(ASD_LogStream_Daemon), "Daemon");
    assert_string_equal(streamtostring(ASD_LogStream_SDK), "SDK");
    assert_string_equal(streamtostring(ASD_LogStream_All), "All");
    assert_string_equal(streamtostring((ASD_LogStream)77), "Unknown-Stream");
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(ASD_log_writes_to_syslog_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(ASD_log_writes_to_stderr_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(ASD_log_writes_to_logging_callback_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_writes_to_logging_callback_and_stderr_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_buffer_writes_to_syslog_with_undersize_prefix_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_buffer_writes_to_syslog_with_max_prefix_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_buffer_writes_to_syslog_with_oversize_prefix_test, setup,
            teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_buffer_writes_to_logging_callback_test, setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_log_buffer_doesnt_log_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(ASD_log_buffer_writes_to_stderr_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_initialize_log_settings_IRDR_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_initialize_log_settings_all_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_log_shift_invalid_params_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_log_shift_malloc_failure_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            ASD_log_shift_writes_correct_log_messages_test, setup, teardown),
        cmocka_unit_test_setup_teardown(ASD_log_shift_doesnt_log_test, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(
            ASD_should_log_correctly_filters_messages_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            ASD_should_log_honors_no_remote_option_test, setup, teardown),
        cmocka_unit_test_setup_teardown(strtolevel_test, setup, teardown),
        cmocka_unit_test_setup_teardown(strtostreams_test, setup, teardown),
        cmocka_unit_test_setup_teardown(streamtostring_test, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
