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
#include <security/pam_appl.h>
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

#include "../auth_pam.h"
#include "../authenticate.h"
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

#define MAX_COMMANDS 20
int malloc_index = 0;
static bool malloc_fail[MAX_COMMANDS];
static bool malloc_fail_mode = false;
void* __real_malloc(size_t size);
void* __wrap_malloc(size_t size)
{
    if (malloc_fail_mode)
    {
        if (malloc_fail[malloc_index])
        {
            // put the flag back. This behavior can be changed later
            // if needed.
            check_expected(size);
            malloc_index++;
            return NULL;
        }
        malloc_index++;
    }
    return __real_malloc(size);
}

int calloc_index = 0;
static bool calloc_fail[MAX_COMMANDS];
static bool calloc_fail_mode = false;
void* __real_calloc(size_t num, size_t size);
void* __wrap_calloc(size_t num, size_t size)
{
    if (calloc_fail_mode)
    {
        if (calloc_fail[malloc_index])
        {
            check_expected(num);
            check_expected(size);
            calloc_index++;
            return NULL;
        }
        calloc_index++;
    }
    return __real_calloc(num, size);
}

int PAM_START_RESULT = PAM_SUCCESS;
pam_handle_t* PAM_START_PAMH = NULL;
int __wrap_pam_start(const char* service_name, const char* user,
                     const struct pam_conv* pam_conversation,
                     pam_handle_t** pamh)
{
    check_expected_ptr(service_name);
    check_expected_ptr(user);
    check_expected_ptr(pam_conversation);
    check_expected_ptr(pamh);
    *pamh = PAM_START_PAMH;
    return PAM_START_RESULT;
}

void expect_pam_start(bool success, pam_handle_t* pamh)
{
    expect_string(__wrap_pam_start, service_name, ASD_PAM_SERVICE);
    expect_string(__wrap_pam_start, user, ASD_PAM_USER);
    expect_any(__wrap_pam_start, pam_conversation);
    expect_any(__wrap_pam_start, pamh);
    PAM_START_PAMH = pamh;
    if (success)
        PAM_START_RESULT = PAM_SUCCESS;
    else
        PAM_START_RESULT = PAM_OPEN_ERR;
}

int PAM_END_RESULT = PAM_SUCCESS;
int __wrap_pam_end(pam_handle_t* pamh, int pam_status)
{
    check_expected_ptr(pamh);
    check_expected(pam_status);
    return PAM_END_RESULT;
}

void expect_pam_end(bool success)
{
    expect_any(__wrap_pam_end, pamh);
    expect_any(__wrap_pam_end, pam_status);
    PAM_END_RESULT = success ? PAM_SUCCESS : PAM_OPEN_ERR;
}

int PAM_AUTHENTICATE_RESULT = PAM_SUCCESS;
int __wrap_pam_authenticate(pam_handle_t* pamh, int flags)
{
    check_expected_ptr(pamh);
    check_expected(flags);
    return PAM_AUTHENTICATE_RESULT;
}

void expect_pam_authenticate(bool success)
{
    expect_any(__wrap_pam_authenticate, pamh);
    expect_value(__wrap_pam_authenticate, flags,
                 PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);
    PAM_AUTHENTICATE_RESULT = success ? PAM_SUCCESS : PAM_OPEN_ERR;
}

int RAND_BYTES_RESULT = 1;
unsigned char RAND_BYTES_BUF[MAX_PW_LEN];
int __wrap_RAND_bytes(unsigned char* buf, int num)
{
    check_expected_ptr(buf);
    check_expected(num);
    for (int i = 0; i < num; i++)
    {
        buf[i] = RAND_BYTES_BUF[i];
    }
    return RAND_BYTES_RESULT;
}

void expect_RAND_bytes_success()
{
    RAND_BYTES_RESULT = 1;
    expect_any(__wrap_RAND_bytes, buf);
    expect_value(__wrap_RAND_bytes, num, MAX_PW_LEN);
    for (int i = 0; i < MAX_PW_LEN; i++)
    {
        RAND_BYTES_BUF[i] = (unsigned char)i;
    }
}

int EXTNET_SEND_RESULT = 0;
int __wrap_extnet_send(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                       size_t sz_len)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    check_expected_ptr(pv_buf);
    check_expected(sz_len);
    return EXTNET_SEND_RESULT;
}

void expect_extnet_send(bool success, auth_handshake_ret_t c_resp)
{
    auth_handshake_resp_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.svr_hdr_version = AUTH_HDR_VERSION;
    resp.result_code = c_resp;

    expect_any(__wrap_extnet_send, state);
    expect_any(__wrap_extnet_send, pconn);
    expect_memory(__wrap_extnet_send, pv_buf, &resp, sizeof(resp));
    expect_value(__wrap_extnet_send, sz_len, sizeof(resp));
    EXTNET_SEND_RESULT = success ? sizeof(resp) : -1;
}

int EXTNET_RECV_RESULT = 0;
bool EXTNET_RECV_DATA_PENDING = false;
char EXTNET_RECV_DATA[10240];
int __wrap_extnet_recv(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                       size_t sz_len, bool* b_data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    check_expected_ptr(pv_buf);
    check_expected(sz_len);
    check_expected_ptr(b_data_pending);
    if (EXTNET_RECV_RESULT > 0)
        memcpy(pv_buf, &EXTNET_RECV_DATA, EXTNET_RECV_RESULT);
    *b_data_pending = EXTNET_RECV_DATA_PENDING;
    return EXTNET_RECV_RESULT;
}

void expect_extnet_recv_success(bool data_pending, char* data_read,
                                size_t data_len)
{
    EXTNET_RECV_RESULT = (int)data_len;
    memset(&EXTNET_RECV_DATA, 0, sizeof(EXTNET_RECV_DATA));
    memcpy(&EXTNET_RECV_DATA, data_read, data_len);
    EXTNET_RECV_DATA_PENDING = data_pending;
    expect_any(__wrap_extnet_recv, state);
    expect_any(__wrap_extnet_recv, pconn);
    expect_any(__wrap_extnet_recv, pv_buf);
    expect_any(__wrap_extnet_recv, sz_len);
    expect_any(__wrap_extnet_recv, b_data_pending);
}

void expect_extnet_recv_failure()
{
    EXTNET_RECV_RESULT = -1;
    expect_any(__wrap_extnet_recv, state);
    expect_any(__wrap_extnet_recv, pconn);
    expect_any(__wrap_extnet_recv, pv_buf);
    expect_any(__wrap_extnet_recv, sz_len);
    expect_any(__wrap_extnet_recv, b_data_pending);
}

STATUS SESSION_GET_AUTHENTICATED_CONN_RESULT = ST_OK;
STATUS __wrap_session_get_authenticated_conn(Session* state,
                                             extnet_conn_t* p_authd_conn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_authd_conn);
    return SESSION_GET_AUTHENTICATED_CONN_RESULT;
}

void expect_session_get_authenticated_conn(STATUS status)
{
    expect_any(__wrap_session_get_authenticated_conn, state);
    expect_any(__wrap_session_get_authenticated_conn, p_authd_conn);
    SESSION_GET_AUTHENTICATED_CONN_RESULT = status;
}

int STRCPY_SAFE_RESULT = 0;
int __wrap_strcpy_safe(char* dest, size_t destsize, const char* src,
                       size_t count)
{
    if (!STRCPY_SAFE_RESULT)
        while (count--)
            *(char*)dest++ = *(const char*)src++;
    return STRCPY_SAFE_RESULT;
}

void expect_authenticate(bool success, char* passphrase)
{
    int dummy_pamh;
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(success);
    expect_pam_end(true);
}

static int setup(void** state)
{
    (void)state;
    malloc_fail_mode = false;
    calloc_fail_mode = false;
    malloc_index = 0;
    calloc_index = 0;
    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        malloc_fail[i] = false;
        calloc_fail[i] = false;
    }
    return 0;
}

static int teardown(void** state)
{
    (void)state;
    malloc_fail_mode = false;
    calloc_fail_mode = false;
    return 0;
}

void authpam_init_success_test(void** state)
{
    (void)state;
    assert_int_equal(ST_OK, authpam_hdlrs.init(NULL));
}

void authpam_client_handshake_get_random_bytes_failure_test(void** state)
{
    (void)state;
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    RAND_BYTES_RESULT = 0;
    expect_any(__wrap_RAND_bytes, buf);
    expect_value(__wrap_RAND_bytes, num, MAX_PW_LEN);
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_params_test(void** state)
{
    (void)state;
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(NULL, &net_state, &extconn));
    assert_int_equal(ST_ERR,
                     authpam_hdlrs.client_handshake(&session, NULL, &extconn));
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, NULL));
}

void authpam_client_handshake_recv_failure_test(void** state)
{
    (void)state;
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_failure();
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_data_pending_error_test(void** state)
{
    (void)state;
    const char* passphrase = "123abc";
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(true, (char*)passphrase, strlen(passphrase));
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_pam_start_failure_test(void** state)
{
    (void)state;
    char version_passphrase[265];
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], "123abc", 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(false, NULL);
    expect_extnet_send(true, AUTH_HANDSHAKE_SYSERR);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_passphrase_too_big_test(void** state)
{
    (void)state;
    char version_passphrase[1000];
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    for (int i = 0; i < (MAX_PW_LEN + 1); i++)
    { // 1 bigger than allowed
        version_passphrase[i + 1] = 'a';
    }
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)&version_passphrase,
                               strlen(version_passphrase));
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_pam_authenticate_failure_test(void** state)
{
    (void)state;
    char* passphrase = "123abc";
    char version_passphrase[265];
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, NULL);
    expect_pam_authenticate(false);
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_pam_end_failure_test(void** state)
{
    (void)state;
    char* passphrase = "123abc";
    char version_passphrase[265];
    int dummy_pamh;
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(true);
    // pam end error can be ignored.
    expect_pam_end(false);
    // ST_ERR means there is not already a current session
    expect_session_get_authenticated_conn(ST_ERR);
    expect_extnet_send(true, AUTH_HANDSHAKE_SUCCESS);
    assert_int_equal(
        ST_OK, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_session_already_authenticated_test(void** state)
{
    (void)state;
    char* passphrase = "123abc";
    char version_passphrase[265];
    int dummy_pamh;
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(true);
    expect_pam_end(true);
    // ST_OK means there already is a current session
    expect_session_get_authenticated_conn(ST_OK);
    expect_extnet_send(true, AUTH_HANDSHAKE_BUSY);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_fail_to_send_result_test(void** state)
{
    (void)state;
    char* passphrase = "123abc";
    char version_passphrase[265];
    int dummy_pamh;
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(true);
    expect_pam_end(true);
    // ST_ERR means there is not already a current session
    expect_session_get_authenticated_conn(ST_ERR);
    expect_extnet_send(false, AUTH_HANDSHAKE_SUCCESS);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_success_test(void** state)
{
    (void)state;
    char* passphrase = "123abc";
    char version_passphrase[265];
    int dummy_pamh;
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, 6);
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(true);
    expect_pam_end(true);
    // ST_ERR means there is not already a current session
    expect_session_get_authenticated_conn(ST_ERR);
    expect_extnet_send(true, AUTH_HANDSHAKE_SUCCESS);
    assert_int_equal(
        ST_OK, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_strip_null_test(void** state)
{
    (void)state;
    char* passphrase = "123abc\n";
    char version_passphrase[265];
    int dummy_pamh;
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, strlen(passphrase));
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_pam_start(true, (pam_handle_t*)&dummy_pamh);
    expect_pam_authenticate(true);
    expect_pam_end(true);
    // ST_ERR means there is not already a current session
    expect_session_get_authenticated_conn(ST_ERR);
    expect_extnet_send(true, AUTH_HANDSHAKE_SUCCESS);
    assert_int_equal(
        ST_OK, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void do_failed_logon_attempt()
{
    char* passphrase = "123abc\n";
    char version_passphrase[265];
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, strlen(passphrase));
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_authenticate(NULL, passphrase);
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void authpam_client_handshake_locked_out_test(void** state)
{
    (void)state;
    do_failed_logon_attempt(); // attempt 1
    do_failed_logon_attempt(); // attempt 2
    do_failed_logon_attempt(); // attempt 3
    do_failed_logon_attempt(); // attempt 4 - locked out
    char* passphrase = "123abc\n";
    char version_passphrase[265];
    memset(&version_passphrase, 0, sizeof(version_passphrase));
    version_passphrase[0] = AUTH_HDR_VERSION;
    memcpy(&version_passphrase[1], passphrase, strlen(passphrase));
    Session session;
    ExtNet net_state;
    extnet_conn_t extconn;
    expect_RAND_bytes_success();
    expect_extnet_recv_success(false, (char*)version_passphrase,
                               strlen(version_passphrase));
    expect_authenticate(false, "abcdefghijklmnopqrs");
    expect_extnet_send(true, AUTH_HANDSHAKE_FAILURE);
    assert_int_equal(
        ST_ERR, authpam_hdlrs.client_handshake(&session, &net_state, &extconn));
}

void pam_conversation_function_invalid_params_test(void** state)
{
    (void)state;
    const struct pam_message* msg;
    struct pam_response* resp;
    const char* app_data = "password123";

    assert_int_equal(PAM_AUTH_ERR, pam_conversation_function(0, NULL, &resp,
                                                             (void*)app_data));
    assert_int_equal(PAM_AUTH_ERR,
                     pam_conversation_function(0, &msg, NULL, (void*)app_data));
    assert_int_equal(PAM_AUTH_ERR,
                     pam_conversation_function(0, &msg, &resp, NULL));
}

void pam_conversation_function_malloc_fail_test(void** state)
{
    (void)state;
    struct pam_message* msg[2];
    struct pam_message temp_msg1;
    struct pam_message temp_msg2;
    msg[0] = &temp_msg1;
    msg[0]->msg_style = PAM_PROMPT_ECHO_OFF;
    msg[1] = &temp_msg2;
    msg[1]->msg_style = PAM_PROMPT_ECHO_ON;
    struct pam_response* resp;
    const char* app_data = "password123";

    malloc_fail[0] = true;
    expect_value(__wrap_malloc, size, strlen(app_data) + 1);
    malloc_fail_mode = true;

    assert_int_equal(PAM_AUTH_ERR, pam_conversation_function(
                                       2, (const struct pam_message**)msg,
                                       &resp, (void*)app_data));
    malloc_fail_mode = false;

    free(resp);
}

void pam_conversation_function_calloc_fail_test(void** state)
{
    (void)state;
    struct pam_message* msg[2];
    struct pam_message temp_msg1;
    struct pam_message temp_msg2;
    msg[0] = &temp_msg1;
    msg[0]->msg_style = PAM_PROMPT_ECHO_OFF;
    msg[1] = &temp_msg2;
    msg[1]->msg_style = PAM_PROMPT_ECHO_ON;
    struct pam_response* resp;
    const char* app_data = "password123";
    STRCPY_SAFE_RESULT = 0;
    calloc_fail[0] = true;
    expect_value(__wrap_calloc, num, 2);
    expect_value(__wrap_calloc, size, sizeof(struct pam_response));
    calloc_fail_mode = true;

    assert_int_equal(PAM_AUTH_ERR, pam_conversation_function(
                                       2, (const struct pam_message**)msg,
                                       &resp, (void*)app_data));
    malloc_fail_mode = false;

    free(resp);
}

void pam_conversation_function_missing_message_echo_off_msg_test(void** state)
{
    (void)state;
    struct pam_message* msg[2];
    struct pam_message temp_msg1;
    struct pam_message temp_msg2;
    msg[0] = &temp_msg1;
    msg[0]->msg_style = PAM_PROMPT_ECHO_ON;
    msg[1] = &temp_msg2;
    msg[1]->msg_style = PAM_PROMPT_ECHO_ON;
    struct pam_response* resp;
    const char* app_data = "password123";

    assert_int_equal(PAM_AUTH_ERR, pam_conversation_function(
                                       2, (const struct pam_message**)msg,
                                       &resp, (void*)app_data));
}

void pam_conversation_function_success_test(void** state)
{
    (void)state;
    struct pam_message* msg[2];
    struct pam_message temp_msg1;
    struct pam_message temp_msg2;
    msg[0] = &temp_msg1;
    msg[0]->msg_style = PAM_PROMPT_ECHO_OFF;
    msg[1] = &temp_msg2;
    msg[1]->msg_style = PAM_PROMPT_ECHO_ON;
    struct pam_response* resp;
    const char* app_data = "password123";
    STRCPY_SAFE_RESULT = 0;
    assert_int_equal(PAM_SUCCESS, pam_conversation_function(
                                      2, (const struct pam_message**)msg, &resp,
                                      (void*)app_data));

    assert_string_equal(app_data, resp[0].resp);
    free(resp[0].resp);
    free(resp);
}

void pam_conversation_function_strcpy_fail_test(void** state)
{
    (void)state;
    struct pam_message* msg[2];
    struct pam_message temp_msg1;
    struct pam_message temp_msg2;
    msg[0] = &temp_msg1;
    msg[0]->msg_style = PAM_PROMPT_ECHO_OFF;
    msg[1] = &temp_msg2;
    msg[1]->msg_style = PAM_PROMPT_ECHO_ON;
    struct pam_response* resp;
    const char* app_data = "password123";
    STRCPY_SAFE_RESULT = 1;
    calloc_fail[0] = true;
    expect_value(__wrap_calloc, num, 2);
    expect_value(__wrap_calloc, size, sizeof(struct pam_response));
    calloc_fail_mode = true;

    assert_int_equal(
        1, pam_conversation_function(2, (const struct pam_message**)msg, &resp,
                                     (void*)app_data));
    malloc_fail_mode = false;

    free(resp);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(authpam_init_success_test),
        cmocka_unit_test(
            authpam_client_handshake_get_random_bytes_failure_test),
        cmocka_unit_test(pam_conversation_function_strcpy_fail_test),
        cmocka_unit_test(authpam_client_handshake_params_test),
        cmocka_unit_test(authpam_client_handshake_recv_failure_test),
        cmocka_unit_test(authpam_client_handshake_data_pending_error_test),
        cmocka_unit_test(authpam_client_handshake_pam_start_failure_test),
        cmocka_unit_test(authpam_client_handshake_passphrase_too_big_test),
        cmocka_unit_test(
            authpam_client_handshake_pam_authenticate_failure_test),
        cmocka_unit_test(authpam_client_handshake_pam_end_failure_test),
        cmocka_unit_test(
            authpam_client_handshake_session_already_authenticated_test),
        cmocka_unit_test(authpam_client_handshake_fail_to_send_result_test),
        cmocka_unit_test(authpam_client_handshake_success_test),
        cmocka_unit_test(authpam_client_handshake_strip_null_test),
        cmocka_unit_test(authpam_client_handshake_locked_out_test),
        cmocka_unit_test(pam_conversation_function_invalid_params_test),
        cmocka_unit_test_setup_teardown(
            pam_conversation_function_malloc_fail_test, setup, teardown),
        cmocka_unit_test_setup_teardown(
            pam_conversation_function_calloc_fail_test, setup, teardown),
        cmocka_unit_test(
            pam_conversation_function_missing_message_echo_off_msg_test),
        cmocka_unit_test(pam_conversation_function_success_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
