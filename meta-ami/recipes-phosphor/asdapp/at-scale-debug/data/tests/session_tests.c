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

#include "../ext_network.h"
#include "../logging.h"
#include "../session.h"
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

int MEMCPY_SAFE_RESULT = 0;
int __wrap_memcpy_safe(void* dest, size_t destsize, const void* src,
                       size_t count)
{
    if (MEMCPY_SAFE_RESULT == 1)
        return MEMCPY_SAFE_RESULT;
    else
    {
        while (count--)
            *(char*)dest++ = *(const char*)src++;
        return MEMCPY_SAFE_RESULT;
    }
}
STATUS __wrap_extnet_open_external_socket(ExtNet* state, const char* cp_bind_if,
                                          uint16_t u16_port, int* pfd_sock)
{
    check_expected_ptr(state);
    check_expected_ptr(cp_bind_if);
    check_expected(u16_port);
    check_expected_ptr(pfd_sock);
    return ST_OK;
}

STATUS __wrap_extnet_accept_connection(ExtNet* state, int ext_listen_sockfd,
                                       extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected(ext_listen_sockfd);
    check_expected_ptr(pconn);
    return ST_OK;
}

STATUS __wrap_extnet_close_client(ExtNet* state, extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    return ST_OK;
}

STATUS EXTNET_INIT_CLIENT_RESULT = ST_OK;
STATUS __wrap_extnet_init_client(ExtNet* state, extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    pconn->sockfd = UNUSED_SOCKET_FD;
    return EXTNET_INIT_CLIENT_RESULT;
}

bool EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;
bool __wrap_extnet_is_client_closed(ExtNet* state, extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    return EXTNET_IS_CLIENT_CLOSED_RESPONSE;
}

static ExtNet extnet;

Session* init_session_and_check_success()
{
    EXTNET_INIT_CLIENT_RESULT = ST_OK;
    expect_any_count(__wrap_extnet_init_client, state, MAX_SESSIONS);
    expect_any_count(__wrap_extnet_init_client, pconn, MAX_SESSIONS);
    Session* session = session_init(&extnet);
    assert_non_null(session);
    return session;
}

void session_init_invalid_param_test(void** state)
{
    (void)state;
    assert_null(session_init(NULL));
}

void session_init_malloc_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    expect_value(__wrap_malloc, size, sizeof(Session));
    malloc_fail = true;
    assert_null(session_init(&extnet));
    malloc_fail = false;
}

void session_init_success_test(void** state)
{
    (void)state;
    Session* session = init_session_and_check_success();
    free(session);
}

void session_init_call_to_extnet_init_client_fails_test(void** state)
{
    (void)state;
    ExtNet extnet;
    EXTNET_INIT_CLIENT_RESULT = ST_ERR;
    expect_any_count(__wrap_extnet_init_client, state, 1);
    expect_any_count(__wrap_extnet_init_client, pconn, 1);
    assert_null(session_init(&extnet));
}

void session_lookup_conn_not_initialized_returns_null_test(void** state)
{
    (void)state;
    assert_null(session_lookup_conn(NULL, 0));
}

void session_lookup_conn_success_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    int fd = 4;
    connection.sockfd = fd;

    Session* session = init_session_and_check_success();
    session_open(session, &connection);
    MEMCPY_SAFE_RESULT = 0;
    assert_non_null(session_lookup_conn(session, fd));
    free(session);
}

void session_open_invalid_param_returns_error_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    Session* session = init_session_and_check_success();
    assert_int_equal(ST_ERR, session_open(session, NULL));
    assert_int_equal(ST_ERR, session_open(NULL, &connection));
    free(session);
}

void session_open_not_initialized_returns_error_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, session_open(NULL, &connection));
}

void session_open_no_available_sessions_test(void** state)
{
    (void)state;
    Session session;
    session.b_initialized = true;
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        session.sessions[i].extconn.sockfd = 2; // a in-use session...
    }
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, session_open(&session, &connection));
}

void session_open_success_test(void** state)
{
    (void)state;
    Session* session = init_session_and_check_success();
    extnet_conn_t connection;
    assert_int_equal(ST_OK, session_open(session, &connection));
    free(session);
}

void session_close_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, session_close(&session, NULL));
    assert_int_equal(ST_ERR, session_close(NULL, &connection));
}

void session_close_unused_socket_fd_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = UNUSED_SOCKET_FD;
    assert_int_equal(ST_ERR, session_close(&session, &connection));
}

void session_close_invalid_session_test(void** state)
{
    (void)state;
    Session session;
    session.b_initialized = true;
    extnet_conn_t connection;
    connection.sockfd = 45;
    assert_int_equal(ST_ERR, session_close(&session, &connection));
}

void session_close_already_closed_test(void** state)
{
    (void)state;
    int fd = 45;
    int id = 36;
    Session session;
    session.b_initialized = true;
    session.n_authenticated_id = id; // it will be the authenticated session
    session.sessions[0].extconn.sockfd = fd;
    session.sessions[0].id = id;
    session.sessions[0].b_authenticated = true;
    session.sessions[0].t_auth_tout = 2;
    extnet_conn_t connection;
    connection.sockfd = fd;
    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = true;

    assert_int_equal(ST_OK, session_close(&session, &connection));
    assert_int_equal(NO_SESSION_AUTHENTICATED, session.n_authenticated_id);
    assert_false(session.sessions[0].b_authenticated);
    assert_int_equal(0, session.sessions[0].t_auth_tout);
}

void session_close_success_test(void** state)
{
    (void)state;
    int fd = 45;
    int id = 36;
    Session session;
    session.b_initialized = true;
    session.n_authenticated_id = id; // it will be the authenticated session
    session.sessions[0].extconn.sockfd = fd;
    session.sessions[0].id = id;
    session.sessions[0].b_authenticated = true;
    session.sessions[0].t_auth_tout = 2;
    extnet_conn_t connection;
    connection.sockfd = fd;
    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = true;

    assert_int_equal(ST_OK, session_close(&session, &connection));
    assert_int_equal(NO_SESSION_AUTHENTICATED, session.n_authenticated_id);
    assert_false(session.sessions[0].b_authenticated);
    assert_int_equal(0, session.sessions[0].t_auth_tout);
}

void session_close_all_invalid_params_test(void** state)
{
    (void)state;
    session_close_all(NULL); // simply does not crash
}

void session_close_all_success_test(void** state)
{
    (void)state;
    extnet_conn_t connections[MAX_SESSIONS];
    Session* session = init_session_and_check_success();
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        connections[i].sockfd = i;
        assert_int_equal(ST_OK, session_open(session, &connections[i]));
    }
    // there are two functions that check this, so times two...
    expect_any_count(__wrap_extnet_is_client_closed, state, MAX_SESSIONS * 2);
    expect_any_count(__wrap_extnet_is_client_closed, pconn, MAX_SESSIONS * 2);
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;

    expect_any_count(__wrap_extnet_close_client, state, MAX_SESSIONS);
    expect_any_count(__wrap_extnet_close_client, pconn, MAX_SESSIONS);

    session_close_all(session);
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        assert_false(session->sessions[i].b_authenticated);
        assert_int_equal(0, session->sessions[i].t_auth_tout);
        assert_false(session->sessions[i].b_data_pending);
    }
    free(session);
}

void session_close_expired_unauth_invalid_params_test(void** state)
{
    (void)state;
    session_close_expired_unauth(NULL); // simply does not crash
}

void session_close_expired_unauth_closes_timed_out_connections_test(
    void** state)
{
    (void)state;
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;
    extnet_conn_t connections[MAX_SESSIONS];
    Session* session = init_session_and_check_success();
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        connections[i].sockfd = i;
        MEMCPY_SAFE_RESULT = 0;
        assert_int_equal(ST_OK, session_open(session, &connections[i]));
        if (i < MAX_SESSIONS / 2)
        {
            // increment timeout to time in distant future, so it
            // will not timeout during this test.
            session->sessions[i].t_auth_tout += 10000;
            expect_any(__wrap_extnet_is_client_closed, state);
            expect_any(__wrap_extnet_is_client_closed, pconn);
        }
        else
        {
            // decrement timeout to time in past so it will timeout
            // during this test.
            session->sessions[i].t_auth_tout -= 10000;
            expect_any_count(__wrap_extnet_is_client_closed, state, 2);
            expect_any_count(__wrap_extnet_is_client_closed, pconn, 2);

            expect_any(__wrap_extnet_close_client, state);
            expect_any(__wrap_extnet_close_client, pconn);
        }
    }
    session_close_expired_unauth(session); // simply does not crash
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (i < MAX_SESSIONS / 2)
        {
            assert_true(session->sessions[i].t_auth_tout > 0);
        }
        else
        {
            assert_int_equal(0, session->sessions[i].t_auth_tout);
        }
    }
    free(session);
}

void session_already_authenticated_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    assert_int_equal(ST_ERR, session_already_authenticated(NULL, &connection));
    assert_int_equal(ST_ERR, session_already_authenticated(&session, NULL));
    session.b_initialized = false;
    assert_int_equal(ST_ERR,
                     session_already_authenticated(&session, &connection));
    session.b_initialized = true;
    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = true;
    assert_int_equal(ST_ERR,
                     session_already_authenticated(&session, &connection));
}

void session_already_authenticated_invalid_session_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    connection.sockfd = 99;

    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;

    assert_int_equal(ST_ERR,
                     session_already_authenticated(&session, &connection));
}

void session_already_authenticated_success_test(void** state)
{
    (void)state;
    int fd = 98;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    session.sessions[0].extconn.sockfd = fd;
    connection.sockfd = fd;

    // not authenticated
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;
    session.sessions[0].b_authenticated = false;
    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    assert_int_equal(ST_ERR,
                     session_already_authenticated(&session, &connection));
    // authenticated
    session.sessions[0].b_authenticated = true;
    expect_any(__wrap_extnet_is_client_closed, state);
    expect_any(__wrap_extnet_is_client_closed, pconn);
    assert_int_equal(ST_OK,
                     session_already_authenticated(&session, &connection));
}

void session_auth_complete_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    assert_int_equal(ST_ERR, session_auth_complete(NULL, &connection));
    assert_int_equal(ST_ERR, session_auth_complete(&session, NULL));
    session.b_initialized = false;
    assert_int_equal(ST_ERR, session_auth_complete(&session, &connection));
}

void session_auth_complete_another_session_already_authenticated_test(
    void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = 6;
    session.b_initialized = true;
    session.n_authenticated_id = 66; // not 6
    assert_int_equal(ST_ERR, session_auth_complete(&session, &connection));
}

void session_auth_complete_session_not_found_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = 6;
    session.b_initialized = true;
    session.n_authenticated_id = UNUSED_SOCKET_FD;
    assert_int_equal(ST_ERR, session_auth_complete(&session, &connection));
}

void session_auth_complete_success_test(void** state)
{
    (void)state;
    int fd = 54;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = fd;
    session.b_initialized = true;
    session.n_authenticated_id = UNUSED_SOCKET_FD;
    session.sessions[0].extconn.sockfd = fd;
    assert_int_equal(ST_OK, session_auth_complete(&session, &connection));

    assert_int_equal(session.n_authenticated_id, session.sessions[0].id);
    assert_true(session.sessions[0].b_authenticated);
}

void session_get_authenticated_conn_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    assert_int_equal(ST_ERR, session_get_authenticated_conn(NULL, &connection));
    session.n_authenticated_id = UNUSED_SOCKET_FD;
    assert_int_equal(ST_ERR,
                     session_get_authenticated_conn(&session, &connection));
    session.n_authenticated_id = 0;
    session.b_initialized = false;
    assert_int_equal(ST_ERR,
                     session_get_authenticated_conn(&session, &connection));
}

void session_get_authenticated_conn_success_test(void** state)
{
    (void)state;
    int auth_conn_id = 1;
    int expected_fd = 98745;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    session.n_authenticated_id = auth_conn_id;
    session.sessions[auth_conn_id].extconn.sockfd = expected_fd;
    MEMCPY_SAFE_RESULT = 0;
    assert_int_equal(ST_OK,
                     session_get_authenticated_conn(&session, &connection));
    assert_int_equal(expected_fd, connection.sockfd);
}

void session_getfds_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    session_fdarr_t session_fds;
    int count;
    int timeout;

    session.b_initialized = true;
    assert_int_equal(ST_ERR,
                     session_getfds(NULL, &session_fds, &count, &timeout));
    assert_int_equal(ST_ERR, session_getfds(&session, NULL, &count, &timeout));
    assert_int_equal(ST_ERR,
                     session_getfds(&session, &session_fds, NULL, &timeout));
    assert_int_equal(ST_ERR,
                     session_getfds(&session, &session_fds, &count, NULL));

    session.b_initialized = false;
    assert_int_equal(ST_ERR,
                     session_getfds(&session, &session_fds, &count, &timeout));
}

void session_getfds_success_test(void** state)
{
    (void)state;
    Session session;
    session_fdarr_t session_fds;
    int count;
    int timeout = -1;
    EXTNET_IS_CLIENT_CLOSED_RESPONSE = false;

    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        session.sessions[i].id = i;
        session.sessions[i].extconn.sockfd = i;
        session.sessions[0].t_auth_tout = 0;
        session.sessions[1].t_auth_tout = time(0L) - 10000;

        expect_any(__wrap_extnet_is_client_closed, state);
        expect_any(__wrap_extnet_is_client_closed, pconn);
    }

    session.b_initialized = true;
    assert_int_equal(ST_OK,
                     session_getfds(&session, &session_fds, &count, &timeout));
    assert_int_equal(MAX_SESSIONS, count);
    assert_int_equal(MAX_SESSIONS, count);
    assert_int_equal(0, timeout);
}

void session_set_data_pending_invalid_params_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;

    session.b_initialized = true;
    assert_int_equal(ST_ERR, session_set_data_pending(NULL, &connection, true));
    assert_int_equal(ST_ERR, session_set_data_pending(&session, NULL, true));

    session.b_initialized = false;
    assert_int_equal(ST_ERR,
                     session_set_data_pending(&session, &connection, true));
}

void session_set_data_pending_invalid_connection_test(void** state)
{
    (void)state;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = UNUSED_SOCKET_FD;

    session.b_initialized = true;
    assert_int_equal(ST_ERR,
                     session_set_data_pending(&session, &connection, true));
}

void session_set_data_pending_success_test(void** state)
{
    (void)state;
    int fd = 45;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = fd;
    session.sessions[1].extconn.sockfd = fd;
    session.sessions[1].b_data_pending = false;

    session.b_initialized = true;
    assert_int_equal(ST_OK,
                     session_set_data_pending(&session, &connection, true));

    assert_true(session.sessions[1].b_data_pending);
}

void session_get_data_pending_invalid_params_test(void** state)
{
    (void)state;
    bool data;
    Session session;
    extnet_conn_t connection;

    session.b_initialized = true;
    assert_int_equal(ST_ERR,
                     session_get_data_pending(NULL, &connection, &data));
    assert_int_equal(ST_ERR, session_get_data_pending(&session, NULL, &data));
    assert_int_equal(ST_ERR,
                     session_get_data_pending(&session, &connection, NULL));

    session.b_initialized = false;
    assert_int_equal(ST_ERR,
                     session_get_data_pending(&session, &connection, &data));
}

void session_get_data_pending_invalid_connection_test(void** state)
{
    (void)state;
    bool data;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = UNUSED_SOCKET_FD;

    session.b_initialized = true;
    assert_int_equal(ST_ERR,
                     session_get_data_pending(&session, &connection, &data));
}

void session_get_data_pending_success_test(void** state)
{
    (void)state;
    int fd = 45;
    bool data;
    bool expected = true;
    Session session;
    extnet_conn_t connection;
    connection.sockfd = fd;
    session.sessions[1].extconn.sockfd = fd;
    session.sessions[1].b_data_pending = expected;

    session.b_initialized = true;
    assert_int_equal(ST_OK,
                     session_get_data_pending(&session, &connection, &data));

    assert_true(session.sessions[1].b_data_pending == expected);
}

void session_lookup_conn_memcpy_fail_test(void** state)
{
    (void)state;
    extnet_conn_t connection;
    MEMCPY_SAFE_RESULT = 1;
    int fd = 4;
    connection.sockfd = fd;
    Session* session = init_session_and_check_success();
    session_open(session, &connection);
    free(session);
}

void session_get_authenticated_conn_memcpy_fail_test(void** state)
{
    (void)state;
    int auth_conn_id = 1;
    int expected_fd = 98745;
    Session session;
    extnet_conn_t connection;
    session.b_initialized = true;
    session.n_authenticated_id = auth_conn_id;
    session.sessions[auth_conn_id].extconn.sockfd = expected_fd;
    MEMCPY_SAFE_RESULT = 1;
    assert_int_equal(ST_ERR,
                     session_get_authenticated_conn(&session, &connection));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(session_init_invalid_param_test),
        cmocka_unit_test(session_init_malloc_fail_test),
        cmocka_unit_test(session_init_success_test),
        cmocka_unit_test(session_init_call_to_extnet_init_client_fails_test),
        cmocka_unit_test(session_lookup_conn_not_initialized_returns_null_test),
        cmocka_unit_test(session_lookup_conn_success_test),
        cmocka_unit_test(session_lookup_conn_memcpy_fail_test),
        cmocka_unit_test(session_open_invalid_param_returns_error_test),
        cmocka_unit_test(session_open_not_initialized_returns_error_test),
        cmocka_unit_test(session_open_no_available_sessions_test),
        cmocka_unit_test(session_open_success_test),
        cmocka_unit_test(session_close_invalid_params_test),
        cmocka_unit_test(session_close_unused_socket_fd_test),
        cmocka_unit_test(session_close_invalid_session_test),
        cmocka_unit_test(session_close_already_closed_test),
        cmocka_unit_test(session_close_success_test),
        cmocka_unit_test(session_close_all_invalid_params_test),
        cmocka_unit_test(session_close_all_success_test),
        cmocka_unit_test(session_close_expired_unauth_invalid_params_test),
        cmocka_unit_test(
            session_close_expired_unauth_closes_timed_out_connections_test),
        cmocka_unit_test(session_already_authenticated_invalid_params_test),
        cmocka_unit_test(session_already_authenticated_invalid_session_test),
        cmocka_unit_test(session_already_authenticated_success_test),
        cmocka_unit_test(session_auth_complete_invalid_params_test),
        cmocka_unit_test(
            session_auth_complete_another_session_already_authenticated_test),
        cmocka_unit_test(session_auth_complete_session_not_found_test),
        cmocka_unit_test(session_auth_complete_success_test),
        cmocka_unit_test(session_get_authenticated_conn_invalid_params_test),
        cmocka_unit_test(session_get_authenticated_conn_success_test),
        cmocka_unit_test(session_get_authenticated_conn_memcpy_fail_test),
        cmocka_unit_test(session_getfds_invalid_params_test),
        cmocka_unit_test(session_getfds_success_test),
        cmocka_unit_test(session_set_data_pending_invalid_params_test),
        cmocka_unit_test(session_set_data_pending_invalid_connection_test),
        cmocka_unit_test(session_set_data_pending_success_test),
        cmocka_unit_test(session_get_data_pending_invalid_params_test),
        cmocka_unit_test(session_get_data_pending_invalid_connection_test),
        cmocka_unit_test(session_get_data_pending_success_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
