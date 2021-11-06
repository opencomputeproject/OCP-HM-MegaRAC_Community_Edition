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

#include "../ext_network.h"
#include "../ext_tcp.h"
#include "../logging.h"
#include "cmocka.h"

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

ssize_t __wrap_recv(int socket, void* buffer, size_t length, int flags)
{
    check_expected(socket);
    check_expected_ptr(buffer);
    check_expected(length);
    check_expected(flags);
    return length;
}

ssize_t __wrap_send(int socket, void* buffer, size_t length, int flags)
{
    check_expected(socket);
    check_expected_ptr(buffer);
    check_expected(length);
    check_expected(flags);
    return length;
}

void exttcp_init_returns_success_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, exttcp_init(NULL));
}

void exttcp_cleanup_returns_without_issue_test(void** state)
{
    (void)state;
    exttcp_cleanup();
}

void exttcp_on_accept_returns_success_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, exttcp_on_accept(NULL, NULL));
}

void exttcp_init_client_returns_success_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, exttcp_init_client(NULL));
}

void exttcp_on_close_client_returns_success_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, exttcp_on_close_client(NULL));
}

void exttcp_recv_returns_error_on_invalid_connection_pointer_test(void** state)
{
    (void)state;
    char buffer[15];
    bool data_pending = false;
    assert_int_equal(-1, exttcp_recv(NULL, &buffer, 15, &data_pending));
}

void exttcp_recv_returns_error_on_invalid_buffer_pointer_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    connection.sockfd = 99;
    bool data_pending = false;
    assert_int_equal(-1, exttcp_recv(&connection, NULL, 1, &data_pending));
}

void exttcp_recv_returns_error_on_invalid_socket_fd_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    connection.sockfd = -1;
    char buffer[15];
    bool data_pending = false;
    assert_int_equal(-1, exttcp_recv(&connection, &buffer, 1, &data_pending));
}

void exttcp_recv_returns_calls_recv_test(void** state)
{
    (void)state;
    int expected_socket = 12;
    size_t expected_length = 123;
    extnet_conn_t connection;
    connection.sockfd = expected_socket;
    char buffer[15];
    bool data_pending = false;
    expect_value(__wrap_recv, socket, expected_socket);
    expect_any(__wrap_recv, buffer);
    expect_value(__wrap_recv, length, expected_length);
    expect_value(__wrap_recv, flags, 0);

    assert_int_equal(
        expected_length,
        exttcp_recv(&connection, &buffer, expected_length, &data_pending));

    assert_false(data_pending);
}

void exttcp_send_returns_error_on_invalid_connection_pointer_test(void** state)
{
    (void)state;
    char buffer[15];
    assert_int_equal(-1, exttcp_send(NULL, &buffer, 15));
}

void exttcp_send_returns_error_on_invalid_buffer_pointer_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    connection.sockfd = 99;
    assert_int_equal(-1, exttcp_send(&connection, NULL, 1));
}

void exttcp_send_returns_error_on_invalid_socket_fd_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    connection.sockfd = -1;
    char buffer[15];
    assert_int_equal(-1, exttcp_send(&connection, &buffer, 1));
}

void exttcp_send_returns_calls_recv_test(void** state)
{
    (void)state;
    int expected_socket = 12;
    size_t expected_length = 1234;
    extnet_conn_t connection;
    connection.sockfd = expected_socket;
    char buffer[15];
    expect_value(__wrap_send, socket, expected_socket);
    expect_any(__wrap_send, buffer);
    expect_value(__wrap_send, length, expected_length);
    expect_value(__wrap_send, flags, 0);

    assert_int_equal(expected_length,
                     exttcp_send(&connection, &buffer, expected_length));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(exttcp_init_returns_success_test),
        cmocka_unit_test(exttcp_cleanup_returns_without_issue_test),
        cmocka_unit_test(exttcp_on_accept_returns_success_test),
        cmocka_unit_test(exttcp_init_client_returns_success_test),
        cmocka_unit_test(exttcp_on_close_client_returns_success_test),
        cmocka_unit_test(
            exttcp_recv_returns_error_on_invalid_connection_pointer_test),
        cmocka_unit_test(
            exttcp_recv_returns_error_on_invalid_buffer_pointer_test),
        cmocka_unit_test(exttcp_recv_returns_error_on_invalid_socket_fd_test),
        cmocka_unit_test(exttcp_recv_returns_calls_recv_test),
        cmocka_unit_test(
            exttcp_send_returns_error_on_invalid_connection_pointer_test),
        cmocka_unit_test(
            exttcp_send_returns_error_on_invalid_buffer_pointer_test),
        cmocka_unit_test(exttcp_send_returns_error_on_invalid_socket_fd_test),
        cmocka_unit_test(exttcp_send_returns_calls_recv_test)};

    return cmocka_run_group_tests(tests, NULL, NULL);
}
