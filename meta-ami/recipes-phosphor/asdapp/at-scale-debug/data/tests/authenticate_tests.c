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

#include "../authenticate.h"
#include "../logging.h"
#include "cmocka.h"

bool fake_authnone_init_was_called = false;
STATUS fake_authnone_init(void* p_hdlr_data)
{
    (void)p_hdlr_data;
    fake_authnone_init_was_called = true;
    return ST_OK;
}

bool fake_auth_none_was_called = false;
STATUS fake_auth_none(Session* session, ExtNet* state, extnet_conn_t* p_extconn)
{
    (void)session;
    (void)state;
    (void)p_extconn;
    fake_auth_none_was_called = true;
    return ST_OK;
}

bool fake_authpam_init_was_called = false;
STATUS fake_authpam_init(void* p_hdlr_data)
{
    (void)p_hdlr_data;
    fake_authpam_init_was_called = true;
    return ST_OK;
}

bool fake_auth_pam_was_called = false;
STATUS fake_auth_pam(Session* session, ExtNet* state, extnet_conn_t* p_extconn)
{
    (void)session;
    (void)state;
    (void)p_extconn;
    fake_auth_pam_was_called = true;
    return ST_OK;
}

auth_hdlrs_t authnone_hdlrs = {fake_authnone_init, fake_auth_none};
auth_hdlrs_t authpam_hdlrs = {fake_authpam_init, fake_auth_pam};

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

static int setup(void** state)
{
    (void)state;
    authnone_hdlrs.init = fake_authnone_init;
    authnone_hdlrs.client_handshake = fake_auth_none;
    authpam_hdlrs.init = fake_authnone_init;
    authpam_hdlrs.client_handshake = fake_auth_none;
    fake_authnone_init_was_called = false;
    fake_auth_none_was_called = false;
    return 0;
}

static int teardown(void** state)
{
    (void)state;
    return 0;
}

void auth_init_invalid_handler_type_returns_error_test(void** state)
{
    (void)state;
    int invalid_handler = 948;
    assert_int_equal(ST_ERR, auth_init(invalid_handler, NULL));
}

void auth_init_invalid_handler_init_returns_error_test(void** state)
{
    (void)state;
    authnone_hdlrs.init = NULL;
    assert_int_equal(ST_ERR, auth_init(AUTH_HDLR_NONE, NULL));
}

void auth_init_calls_init_successfully_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, auth_init(AUTH_HDLR_NONE, NULL));
    assert_true(fake_authnone_init_was_called);

    fake_authnone_init_was_called = false;
    assert_int_equal(ST_OK, auth_init(AUTH_HDLR_PAM, NULL));
    assert_true(fake_authnone_init_was_called);
}

void auth_client_handshake_invalid_handler_client_handshake_returns_error_test(
    void** state)
{
    (void)state;
    authnone_hdlrs.client_handshake = NULL;
    authpam_hdlrs.client_handshake = NULL;
    assert_int_equal(ST_ERR, auth_client_handshake(NULL, NULL, NULL));
}

void auth_client_handshake_calls_client_handshake_successfully_test(
    void** state)
{
    (void)state;
    assert_int_equal(ST_OK, auth_client_handshake(NULL, NULL, AUTH_HDLR_NONE));
    assert_true(fake_auth_none_was_called);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            auth_init_invalid_handler_type_returns_error_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            auth_init_invalid_handler_init_returns_error_test, setup, teardown),
        cmocka_unit_test_setup_teardown(auth_init_calls_init_successfully_test,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(
            auth_client_handshake_invalid_handler_client_handshake_returns_error_test,
            setup, teardown),
        cmocka_unit_test_setup_teardown(
            auth_client_handshake_calls_client_handshake_successfully_test,
            setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
