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

#include "../asd_common.h"
#include "../asd_main.h"
#include "../asd_msg.h"
#include "../authenticate.h"
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

void __wrap_ASD_initialize_log_settings(ASD_LogLevel level,
                                        ASD_LogStream stream,
                                        bool write_to_syslog,
                                        ShouldLogFunctionPtr should_log_ptr,
                                        LogFunctionPtr log_ptr)
{
    check_expected(level);
    check_expected(stream);
    check_expected(write_to_syslog);
    check_expected_ptr(should_log_ptr);
    check_expected_ptr(log_ptr);
}

void expect_any_ASD_initialize_log_settings()
{
    expect_any(__wrap_ASD_initialize_log_settings, level);
    expect_any(__wrap_ASD_initialize_log_settings, stream);
    expect_any(__wrap_ASD_initialize_log_settings, write_to_syslog);
    expect_any(__wrap_ASD_initialize_log_settings, should_log_ptr);
    expect_any(__wrap_ASD_initialize_log_settings, log_ptr);
}

void expect_default_ASD_initialize_log_settings()
{
    expect_value(__wrap_ASD_initialize_log_settings, level, DEFAULT_LOG_LEVEL);
    expect_value(__wrap_ASD_initialize_log_settings, stream,
                 DEFAULT_LOG_STREAMS);
    expect_value(__wrap_ASD_initialize_log_settings, write_to_syslog, false);
    expect_any(__wrap_ASD_initialize_log_settings, should_log_ptr);
    expect_any(__wrap_ASD_initialize_log_settings, log_ptr);
}

bool strtolevel_result = true;
ASD_LogLevel strtolevel_output = ASD_LogLevel_Off;
bool __wrap_strtolevel(char* input, ASD_LogLevel* output)
{
    check_expected_ptr(input);
    check_expected_ptr(output);
    *output = strtolevel_output;
    return strtolevel_result;
}

bool strtostreams_result = true;
ASD_LogStream strtostreams_output = ASD_LogStream_None;
bool __wrap_strtostreams(char* input, ASD_LogStream* output)
{
    check_expected_ptr(input);
    check_expected_ptr(output);
    *output = strtostreams_output;
    return strtostreams_result;
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

static bool free_fail;
void __real_free(void* ptr);
void __wrap_free(void* ptr)
{
    if (free_fail)
    {

        check_expected(ptr);
        return;
    }
    return __real_free(ptr);
}

int GETPEERNAME_RESULT = 0;
int __wrap_getpeername(int fd, __SOCKADDR_ARG addr, socklen_t* addrlen)
{
    check_expected(fd);
    check_expected(addr);
    check_expected_ptr(addrlen);
    return GETPEERNAME_RESULT;
}

void expect_getpeername(int result)
{
    expect_any(__wrap_getpeername, fd);
    expect_any(__wrap_getpeername, addr);
    expect_any(__wrap_getpeername, addrlen);
    GETPEERNAME_RESULT = result;
}

void expect_getpeername_check_fd(int result, int fd)
{
    expect_value(__wrap_getpeername, fd, fd);
    expect_any(__wrap_getpeername, addr);
    expect_any(__wrap_getpeername, addrlen);
    GETPEERNAME_RESULT = result;
}

Session SESSION;
Session* SESSION_INIT_RESULT = &SESSION;
bool SESSION_INIT_MALLOC = false;
Session* __wrap_session_init(ExtNet* extnet)
{
    check_expected_ptr(extnet);
    if (SESSION_INIT_MALLOC)
        return (Session*)__real_malloc(sizeof(Session));
    return SESSION_INIT_RESULT;
}

void expect_any_session_init()
{
    expect_any(__wrap_session_init, extnet);
    SESSION_INIT_RESULT = &SESSION;
}

extnet_conn_t* SESSION_LOOKUP_CONN_RESULT = NULL;
extnet_conn_t* __wrap_session_lookup_conn(Session* state, int fd)
{
    check_expected_ptr(state);
    check_expected(fd);
    return SESSION_LOOKUP_CONN_RESULT;
}

void expect_session_lookup_conn(extnet_conn_t* conn, int expected_fd)
{
    expect_any(__wrap_session_lookup_conn, state);
    expect_value(__wrap_session_lookup_conn, fd, expected_fd);
    SESSION_LOOKUP_CONN_RESULT = conn;
}

STATUS SESSION_OPEN_RESLUT = ST_OK;
STATUS __wrap_session_open(Session* state, extnet_conn_t* p_extconn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    return SESSION_OPEN_RESLUT;
}

void expect_session_open()
{
    expect_any(__wrap_session_open, state);
    expect_any(__wrap_session_open, p_extconn);
    SESSION_OPEN_RESLUT = ST_OK;
}

STATUS SESSION_CLOSE_RESULT = ST_OK;
STATUS __wrap_session_close(Session* state, extnet_conn_t* p_extconn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    return SESSION_CLOSE_RESULT;
}

void expect_session_close(STATUS result)
{
    expect_any(__wrap_session_close, state);
    expect_any(__wrap_session_close, p_extconn);
    SESSION_CLOSE_RESULT = result;
}

void __wrap_session_close_all(Session* state)
{
    check_expected_ptr(state);
}

void __wrap_session_close_expired_unauth(Session* state)
{
    check_expected_ptr(state);
}

int MEMCPY_SAFE_RESULT = 0;
int __wrap_memcpy_safe(void* dest, size_t destsize, const void* src,
                       size_t count)
{
    return MEMCPY_SAFE_RESULT;
}

STATUS SESSION_ALREADY_AUTHENTICATED_RESULT = ST_OK;
STATUS __wrap_session_already_authenticated(Session* state,
                                            extnet_conn_t* p_extconn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    return SESSION_ALREADY_AUTHENTICATED_RESULT;
}

void expect_session_already_authenticated(STATUS result)
{
    expect_any(__wrap_session_already_authenticated, state);
    expect_any(__wrap_session_already_authenticated, p_extconn);
    SESSION_ALREADY_AUTHENTICATED_RESULT = result;
}

STATUS SESSION_AUTH_COMPLETE_RESULT = ST_OK;
STATUS __wrap_session_auth_complete(Session* state, extnet_conn_t* p_extconn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    return SESSION_AUTH_COMPLETE_RESULT;
}

void expect_session_auth_complete(STATUS result)
{
    expect_any(__wrap_session_auth_complete, state);
    expect_any(__wrap_session_auth_complete, p_extconn);
    SESSION_AUTH_COMPLETE_RESULT = result;
}

STATUS SESSION_GET_AUTHENTICATED_CONN_RESULT = ST_OK;
STATUS __wrap_session_get_authenticated_conn(Session* state,
                                             extnet_conn_t* p_authd_conn)
{
    check_expected_ptr(state);
    check_expected_ptr(p_authd_conn);
    return SESSION_GET_AUTHENTICATED_CONN_RESULT;
}

void expect_session_get_authenticated_conn(STATUS result)
{
    expect_any(__wrap_session_get_authenticated_conn, state);
    expect_any(__wrap_session_get_authenticated_conn, p_authd_conn);
    SESSION_GET_AUTHENTICATED_CONN_RESULT = result;
}

int SESSION_FDS_RESULT_INDEX = 0;
STATUS SESSION_FDS_RESULT[2];
int SESSION_FDS_COUNT = 0;
session_fdarr_t SESSION_FDS;
int SESSION_TIMEOUT = 0;
STATUS __wrap_session_getfds(Session* state, session_fdarr_t* na_fds,
                             int* pn_fds, int* pn_timeout)
{
    check_expected_ptr(state);
    check_expected_ptr(na_fds);
    check_expected_ptr(pn_fds);
    check_expected_ptr(pn_timeout);
    *pn_fds = SESSION_FDS_COUNT;
    for (int i = 0; i < SESSION_FDS_COUNT; i++)
        *na_fds[i] = SESSION_FDS[i];
    *pn_timeout = SESSION_TIMEOUT;
    return SESSION_FDS_RESULT[SESSION_FDS_RESULT_INDEX++];
}

void expect_session_getfds(STATUS result, int index)
{
    expect_any(__wrap_session_getfds, state);
    expect_any(__wrap_session_getfds, na_fds);
    expect_any(__wrap_session_getfds, pn_fds);
    expect_any(__wrap_session_getfds, pn_timeout);
    SESSION_FDS_RESULT[index] = result;
    SESSION_FDS_RESULT_INDEX = 0;
}

STATUS SESSION_SET_DATA_PENDING_RESULT = ST_OK;
bool SESSION_DATA_PENDING_STORED_VALUE = false;
STATUS __wrap_session_set_data_pending(Session* state, extnet_conn_t* p_extconn,
                                       bool b_data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    check_expected(b_data_pending);
    SESSION_DATA_PENDING_STORED_VALUE = b_data_pending;
    return SESSION_SET_DATA_PENDING_RESULT;
}

void expect_session_data_pending(STATUS result)
{
    expect_any(__wrap_session_set_data_pending, state);
    expect_any(__wrap_session_set_data_pending, p_extconn);
    expect_any(__wrap_session_set_data_pending, b_data_pending);
    SESSION_SET_DATA_PENDING_RESULT = result;
}

STATUS SESSION_GET_DATA_PENDING = ST_OK;
bool SESSION_DATA_PENDING = false;
STATUS __wrap_session_get_data_pending(Session* state, extnet_conn_t* p_extconn,
                                       bool* b_data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(p_extconn);
    check_expected_ptr(b_data_pending);
    *b_data_pending = SESSION_DATA_PENDING;
    return SESSION_GET_DATA_PENDING;
}

void expect_session_get_data_pending(STATUS result, bool pending)
{
    expect_any(__wrap_session_get_data_pending, state);
    expect_any(__wrap_session_get_data_pending, p_extconn);
    expect_any(__wrap_session_get_data_pending, b_data_pending);
    SESSION_GET_DATA_PENDING = result;
    SESSION_DATA_PENDING = pending;
}

void __wrap_close(int fd)
{
    check_expected(fd);
}

ASD_MSG* FAKE_ASD_MSG_INIT_RESULT = NULL;
ASD_MSG* __wrap_asd_msg_init(SendFunctionPtr send_function,
                             ReadFunctionPtr read_function, void* cb_state,
                             config* config)
{
    check_expected_ptr(cb_state);
    check_expected_ptr(send_function);
    check_expected_ptr(read_function);
    check_expected_ptr(config);
    return FAKE_ASD_MSG_INIT_RESULT;
}

void expect_any_asd_msg_init(ASD_MSG* sdk)
{
    FAKE_ASD_MSG_INIT_RESULT = sdk;
    expect_any(__wrap_asd_msg_init, cb_state);
    expect_any(__wrap_asd_msg_init, send_function);
    expect_any(__wrap_asd_msg_init, read_function);
    expect_any(__wrap_asd_msg_init, config);
}

STATUS ASD_MSG_FREE_RESULT = ST_OK;
STATUS __wrap_asd_msg_free(ASD_MSG* state)
{
    check_expected_ptr(state);
    return ASD_MSG_FREE_RESULT;
}

void expect_asd_msg_free(STATUS result)
{
    expect_any(__wrap_asd_msg_free, state);
    ASD_MSG_FREE_RESULT = result;
}

STATUS ASD_MSG_READ_RESULT = ST_OK;
STATUS __wrap_asd_msg_read(ASD_MSG* state, void* conn, bool* data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(conn);
    check_expected_ptr(data_pending);
    return ASD_MSG_READ_RESULT;
}

void expect_asd_msg_read(STATUS result)
{
    expect_any(__wrap_asd_msg_read, state);
    expect_any(__wrap_asd_msg_read, conn);
    expect_any(__wrap_asd_msg_read, data_pending);
    ASD_MSG_READ_RESULT = result;
}

target_fdarr_t GPIO_FDS;
int NUM_GPIO_FDS = 0;
int ASD_MSG_GET_FDS_INDEX = 0;
STATUS ASD_MSG_GET_FDS_RESULT[2];
STATUS __wrap_asd_msg_get_fds(ASD_MSG* state, target_fdarr_t* fds, int* num_fds)
{
    check_expected_ptr(state);
    check_expected_ptr(fds);
    check_expected_ptr(num_fds);
    *num_fds = NUM_GPIO_FDS;
    for (int i = 0; i < NUM_GPIO_FDS; i++)
    {
        (*fds)[i] = GPIO_FDS[i];
    }
    return ASD_MSG_GET_FDS_RESULT[ASD_MSG_GET_FDS_INDEX++];
}

void expect_asd_msg_get_fds(STATUS result, int index)
{
    expect_any(__wrap_asd_msg_get_fds, state);
    expect_any(__wrap_asd_msg_get_fds, fds);
    expect_any(__wrap_asd_msg_get_fds, num_fds);
    ASD_MSG_GET_FDS_RESULT[index] = result;
    ASD_MSG_GET_FDS_INDEX = 0;
}

STATUS ASD_MSG_EVENT_RESULT = ST_OK;
STATUS __wrap_asd_msg_event(ASD_MSG* state, int fd)
{
    check_expected_ptr(state);
    check_expected(fd);
    return ASD_MSG_EVENT_RESULT;
}

void expect_asd_msg_event(STATUS result)
{
    expect_any(__wrap_asd_msg_event, state);
    expect_any(__wrap_asd_msg_event, fd);
    ASD_MSG_EVENT_RESULT = result;
}

ExtNet EXTNET;
ExtNet* FAKE_EXTNET_INIT_RESULT = &EXTNET;
bool EXTNET_INIT_MALLOC = false;
ExtNet* __wrap_extnet_init(extnet_hdlr_type_t eType, void* p_hdlr_data,
                           int n_max_sessions)
{
    check_expected(eType);
    check_expected_ptr(p_hdlr_data);
    check_expected(n_max_sessions);
    if (EXTNET_INIT_MALLOC)
        return (ExtNet*)__real_malloc(sizeof(ExtNet));
    return FAKE_EXTNET_INIT_RESULT;
}

void expect_any_extnet_init()
{
    FAKE_EXTNET_INIT_RESULT = &EXTNET;
    expect_any(__wrap_extnet_init, eType);
    expect_any(__wrap_extnet_init, p_hdlr_data);
    expect_any(__wrap_extnet_init, n_max_sessions);
}

STATUS FAKE_EXTNET_OPEN_EXTERNAL_RESULT = ST_OK;
STATUS __wrap_extnet_open_external_socket(ExtNet* state, const char* cp_bind_if,
                                          uint16_t u16_port, int* pfd_sock)
{
    check_expected_ptr(state);
    check_expected_ptr(cp_bind_if);
    check_expected(u16_port);
    check_expected_ptr(pfd_sock);
    return FAKE_EXTNET_OPEN_EXTERNAL_RESULT;
}

void expect_any_extnet_open_external_socket()
{
    FAKE_EXTNET_OPEN_EXTERNAL_RESULT = ST_OK;
    expect_any(__wrap_extnet_open_external_socket, state);
    expect_any(__wrap_extnet_open_external_socket, cp_bind_if);
    expect_any(__wrap_extnet_open_external_socket, u16_port);
    expect_any(__wrap_extnet_open_external_socket, pfd_sock);
}

STATUS EXTNET_ACCEPT_CONNECTION = ST_OK;
STATUS __wrap_extnet_accept_connection(ExtNet* state, int ext_listen_sockfd,
                                       extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected(ext_listen_sockfd);
    check_expected_ptr(pconn);
    return EXTNET_ACCEPT_CONNECTION;
}

void expect_extnet_accept_connection()
{
    EXTNET_ACCEPT_CONNECTION = ST_OK;
    expect_any(__wrap_extnet_accept_connection, state);
    expect_any(__wrap_extnet_accept_connection, ext_listen_sockfd);
    expect_any(__wrap_extnet_accept_connection, pconn);
}

STATUS __wrap_extnet_close_client(extnet_conn_t* pconn)
{
    return ST_OK;
}

STATUS __wrap_extnet_init_client(extnet_conn_t* pconn)
{
    return ST_OK;
}

bool __wrap_extnet_is_client_closed(extnet_conn_t* pconn)
{
    return false;
}

int EXTNET_RECV_RESULT = 0;
bool EXTNET_RECV_PENDING = false;
int __wrap_extnet_recv(ExtNet* state, extnet_conn_t* pconn, void* pv_buf,
                       size_t sz_len, bool* b_data_pending)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    check_expected_ptr(pv_buf);
    check_expected(sz_len);
    check_expected_ptr(b_data_pending);
    *b_data_pending = EXTNET_RECV_PENDING;
    return EXTNET_RECV_RESULT;
}

void expect_extnet_recv(int result, size_t num, bool pending)
{
    expect_any(__wrap_extnet_recv, state);
    expect_any(__wrap_extnet_recv, pconn);
    expect_any(__wrap_extnet_recv, pv_buf);
    expect_value(__wrap_extnet_recv, sz_len, num);
    expect_any(__wrap_extnet_recv, b_data_pending);
    EXTNET_RECV_RESULT = result;
    EXTNET_RECV_PENDING = pending;
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

STATUS FAKE_AUTH_INIT_RESULT = ST_OK;
STATUS __wrap_auth_init(auth_hdlr_type_t e_type, void* p_hdlr_data)
{
    check_expected(e_type);
    check_expected_ptr(p_hdlr_data);
    return FAKE_AUTH_INIT_RESULT;
}

void expect_any_auth_init()
{
    expect_any(__wrap_auth_init, e_type);
    expect_any(__wrap_auth_init, p_hdlr_data);
    FAKE_AUTH_INIT_RESULT = ST_OK;
}

STATUS AUTH_CLIENT_HANDSHAKE_RESULT = ST_OK;
STATUS __wrap_auth_client_handshake(extnet_conn_t* p_extconn)
{
    check_expected_ptr(p_extconn);
    return AUTH_CLIENT_HANDSHAKE_RESULT;
}

void expect_auth_client_handshake(STATUS result)
{
    expect_any(__wrap_auth_client_handshake, p_extconn);
    AUTH_CLIENT_HANDSHAKE_RESULT = result;
}

STATUS SET_CONFIG_DEFAULTS_RESULT = ST_OK;
STATUS __wrap_set_config_defaults(config* config, const i2c_options* i2c)
{
    check_expected_ptr(config);
    check_expected_ptr(i2c);
    return SET_CONFIG_DEFAULTS_RESULT;
}

void expect_set_config_defaults(STATUS result)
{
    expect_any(__wrap_set_config_defaults, config);
    expect_any(__wrap_set_config_defaults, i2c);
    SET_CONFIG_DEFAULTS_RESULT = result;
}

int EVENT_FD_RESULT = 0;
int __wrap_eventfd(unsigned int initval, int flags)
{
    check_expected(initval);
    check_expected(flags);
    return EVENT_FD_RESULT;
}

void expect_any_eventfd()
{
    expect_any(__wrap_eventfd, initval);
    expect_any(__wrap_eventfd, flags);
    EVENT_FD_RESULT = 0;
}

void expect_asd_init()
{
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();
    expect_any_auth_init();
    expect_any_session_init();
    expect_any_eventfd();
    expect_any_extnet_open_external_socket();
}

uint64_t FAKE_READ_VALUE = 0;
ssize_t FAKE_READ_RESULT = 0;
ssize_t __wrap_read(int fd, void* buf, size_t count)
{
    check_expected(fd);
    check_expected_ptr(buf);
    check_expected(count);
    memcpy(buf, &FAKE_READ_VALUE, sizeof(FAKE_READ_VALUE));
    return FAKE_READ_RESULT;
}

void expect_read(int fd, int result, uint64_t value)
{
    expect_value(__wrap_read, fd, fd);
    expect_any(__wrap_read, buf);
    expect_value(__wrap_read, count, sizeof(uint64_t));
    FAKE_READ_RESULT = result;
    FAKE_READ_VALUE = value;
}

void expect_on_client_connect(ASD_MSG* asd_msg, STATUS result)
{
    expect_getpeername(0);

    expect_set_config_defaults(result);

    if (result == ST_OK)
    {
        expect_any_asd_msg_init(asd_msg);
        expect_any_ASD_initialize_log_settings();
    }
}

void expect_on_client_disconnect(STATUS result)
{
    expect_set_config_defaults(result);
    if (result == ST_OK)
    {
        expect_any_ASD_initialize_log_settings();
        expect_asd_msg_free(ST_OK);
    }
}

void expect_process_client_message(extnet_conn_t* conn, int fd, STATUS result)
{
    expect_session_lookup_conn(conn, fd);
    expect_session_get_data_pending(ST_OK, true);
    expect_session_already_authenticated(ST_OK);
    expect_asd_msg_read(ST_OK);
    expect_session_data_pending(result);
}

int POLL_RESULT = 0;
short POLL_REVENTS[MAX_FDS];
int __wrap_poll(struct pollfd* fds, nfds_t nfds, int timeout)
{
    check_expected_ptr(fds);
    check_expected(nfds);
    check_expected(timeout);

    for (int i = 0; i < MAX_FDS; i++)
    {
        fds[i].revents = POLL_REVENTS[i];
    }

    return POLL_RESULT;
}

void expect_poll(int timeout, int result)
{
    expect_any(__wrap_poll, fds);
    expect_any(__wrap_poll, nfds);
    expect_value(__wrap_poll, timeout, timeout);
    POLL_RESULT = result;
}

void expect_process_new_client(asd_state* asd, ExtNet* extnet)
{
    asd->extnet = extnet;
    asd->args.session.e_auth_type = AUTH_HDLR_NONE;

    expect_extnet_accept_connection();
    expect_session_open();
}

void expect_process_all_client_messages(int* fds, int num_fds,
                                        extnet_conn_t* fake_conn, STATUS result)
{
    expect_any(__wrap_session_close_expired_unauth, state);

    for (int i = 0; i < num_fds; i++)
        if ((i + 1) < num_fds)
            expect_process_client_message(fake_conn, fds[i], ST_OK);
        else
            expect_process_client_message(fake_conn, fds[i], result);
}

static int setup(void** state)
{
    (void)state;
    malloc_fail_mode = false;
    free_fail = false;
    for (int i = 0; i < MAX_COMMANDS; i++)
        malloc_fail[i] = false;
    return 0;
}

static int teardown(void** state)
{
    (void)state;
    malloc_fail_mode = false;
    return 0;
}

void asd_main_process_command_line_failure_test(void** state)
{
    (void)state;
    optind = 1;
    char* argv[] = {"blah", "--unkwown_arg"};

    expect_default_ASD_initialize_log_settings();

    assert_int_equal(asd_main(2, (char**)&argv), 1);
}

void asd_main_process_command_line_request_processing_failure_test(void** state)
{
    (void)state;
    optind = 1;
    char* argv[] = {"blah"};

    SESSION_INIT_MALLOC = true;
    EXTNET_INIT_MALLOC = true;
    expect_default_ASD_initialize_log_settings();
    expect_asd_init();
    expect_session_getfds(ST_ERR, 0);
    expect_any(__wrap_session_close_all, state);

    assert_int_equal(asd_main(1, (char**)&argv), 1);
    SESSION_INIT_MALLOC = false;
    EXTNET_INIT_MALLOC = false;
}

void process_command_line_sets_defaults_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah"};
    assert_true(process_command_line(1, (char**)&argv, &args));
    assert_int_equal(args.i2c.enable, DEFAULT_I2C_ENABLE);
    assert_int_equal(args.i2c.bus, DEFAULT_I2C_BUS);
    assert_false(args.use_syslog);
    assert_int_equal(args.log_streams, DEFAULT_LOG_STREAMS);
    assert_int_equal(args.log_level, DEFAULT_LOG_LEVEL);
    assert_int_equal(args.session.n_port_number, DEFAULT_PORT);
    assert_string_equal(args.session.cp_certkeyfile, DEFAULT_CERT_FILE);
    assert_int_equal(args.session.cp_net_bind_device, NULL);
    assert_int_equal(args.session.e_extnet_type, EXTNET_HDLR_TLS);
    assert_int_equal(args.session.e_auth_type, AUTH_HDLR_PAM);
}

void process_command_line_port_number_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-p 5555"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_int_equal(args.session.n_port_number, 5555);
}

void process_command_line_log_to_syslog_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-s"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_true(args.use_syslog);
}

void process_command_line_set_unsecure_mode_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-u"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_int_equal(args.session.e_extnet_type, EXTNET_HDLR_NON_ENCRYPT);
    assert_int_equal(args.session.e_auth_type, AUTH_HDLR_NONE);
}

void process_command_line_set_key_file_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-kblah-halb"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_string_equal(args.session.cp_certkeyfile, "blah-halb");
}

void process_command_line_set_net_bind_device_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-neth2"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_string_equal(args.session.cp_net_bind_device, "eth2");
}

void process_command_line_set_i2c_test(void** state)
{
    (void)state; /* unused */
    asd_args args;
    optind = 1;
    char* argv[] = {"blah", "-i 4"};
    assert_true(process_command_line(2, (char**)&argv, &args));
    assert_int_equal(args.i2c.enable, true);
    assert_int_equal(args.i2c.bus, 4);
}

static void process_command_line_log_level_test(void** state)
{
    (void)state;
    optind = 1;
    asd_args args;
    char* argvOff[] = {"blah", "--log-level=Trace"};
    expect_any(__wrap_strtolevel, input);
    expect_any(__wrap_strtolevel, output);
    strtolevel_output = ASD_LogLevel_Trace;
    strtolevel_result = true;
    assert_true(process_command_line(2, (char**)&argvOff, &args));
    assert_int_equal(args.log_level, ASD_LogLevel_Trace);

    optind = 1;
    char* argvNo[] = {"blah", "--log-level=NonExistentLevel"};
    expect_any(__wrap_strtolevel, input);
    expect_any(__wrap_strtolevel, output);
    strtolevel_result = false;
    assert_false(process_command_line(2, (char**)&argvNo, &args));
}

static void process_command_line_log_streams_test(void** state)
{
    (void)state;
    optind = 1;
    asd_args args;
    char* argvOff[] = {"blah", "--log-streams=All"};
    expect_any(__wrap_strtostreams, input);
    expect_any(__wrap_strtostreams, output);
    strtostreams_output = ASD_LogStream_All;
    strtostreams_result = true;
    assert_true(process_command_line(2, (char**)&argvOff, &args));
    assert_int_equal(args.log_streams, ASD_LogStream_All);

    optind = 1;
    char* argvNo[] = {"blah", "--log-streams=NonExistentStream"};
    expect_any(__wrap_strtostreams, input);
    expect_any(__wrap_strtostreams, output);
    strtostreams_result = false;
    assert_false(process_command_line(2, (char**)&argvNo, &args));
}

static void process_command_line_unknown_arg_test(void** state)
{
    (void)state;
    optind = 1;
    asd_args args;
    char* argvOff[] = {"blah", "--unkwown_arg"};
    assert_false(process_command_line(2, (char**)&argvOff, &args));
}

static void
    init_asd_state_handles_set_config_defaults_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_set_config_defaults(ST_ERR);
    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

static void init_asd_state_handles_extnet_init_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);

    FAKE_EXTNET_INIT_RESULT = NULL;
    expect_value(__wrap_extnet_init, eType,
                 asd_state.args.session.e_extnet_type);
    expect_any(__wrap_extnet_init, p_hdlr_data);
    expect_value(__wrap_extnet_init, n_max_sessions, MAX_SESSIONS);

    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

static void init_asd_state_handles_auth_init_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();

    FAKE_AUTH_INIT_RESULT = ST_ERR;
    expect_value(__wrap_auth_init, e_type, asd_state.args.session.e_auth_type);
    expect_any(__wrap_auth_init, p_hdlr_data);

    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

static void init_asd_state_handles_session_init_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();
    expect_any_auth_init();

    expect_any(__wrap_session_init, extnet);
    SESSION_INIT_RESULT = NULL;

    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

void init_asd_state_handles_eventfd_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();
    expect_any_auth_init();
    expect_any_session_init();

    expect_value(__wrap_eventfd, initval, 0);
    expect_value(__wrap_eventfd, flags, O_NONBLOCK);
    EVENT_FD_RESULT = -1;

    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

static void init_asd_state_handles_extnet_open_external_socket_failure_test(
    void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();
    expect_any_auth_init();
    expect_any_session_init();
    expect_any_eventfd();

    FAKE_EXTNET_OPEN_EXTERNAL_RESULT = ST_ERR;
    expect_any(__wrap_extnet_open_external_socket, state);
    expect_any(__wrap_extnet_open_external_socket, cp_bind_if);
    expect_value(__wrap_extnet_open_external_socket, u16_port,
                 asd_state.args.session.n_port_number);
    expect_any(__wrap_extnet_open_external_socket, pfd_sock);

    assert_int_equal(ST_ERR, init_asd_state(&asd_state));
}

static void init_asd_state_returns_true_test(void** state)
{
    (void)state;
    asd_state asd_state;
    expect_any_ASD_initialize_log_settings();
    expect_set_config_defaults(ST_OK);
    expect_any_extnet_init();
    expect_any_auth_init();
    expect_any_session_init();
    expect_any_eventfd();
    expect_any_extnet_open_external_socket();

    assert_int_equal(ST_OK, init_asd_state(&asd_state));
}

void asd_main_init_asd_state_failure_test(void** state)
{
    (void)state;
    optind = 1;
    char* argv[] = {"blah"};

    expect_default_ASD_initialize_log_settings();

    // cause init_asd_state to fail
    expect_set_config_defaults(ST_ERR);

    assert_int_equal(asd_main(1, (char**)&argv), 1);
}

void deinit_asd_state_test(void** state)
{
    (void)state;
    int expected_fd = 68;
    asd_state asd_state;
    ASD_MSG sdk;
    Session session;
    asd_state.session = &session;
    asd_state.host_fd = expected_fd;
    asd_state.asd_msg = &sdk;

    expect_any(__wrap_session_close_all, state);
    expect_value(__wrap_close, fd, expected_fd);
    expect_asd_msg_free(ST_OK);

    deinit_asd_state(&asd_state);
}

void send_out_msg_on_socket_params_test(void** state)
{
    (void)state;
    unsigned char buffer[9];
    asd_state asd_state;

    assert_int_equal(ST_ERR,
                     send_out_msg_on_socket(NULL, (unsigned char*)&buffer, 1));
    assert_int_equal(ST_ERR, send_out_msg_on_socket(&asd_state, NULL, 1));
}

void send_out_msg_on_socket_no_authenticated_socket_test(void** state)
{
    (void)state;
    unsigned char buffer[9];
    asd_state asd_state;

    expect_session_get_authenticated_conn(ST_ERR);

    assert_int_equal(
        ST_ERR, send_out_msg_on_socket(&asd_state, (unsigned char*)&buffer, 1));
}

void send_out_msg_on_socket_send_failure_test(void** state)
{
    (void)state;
    unsigned char buffer[9];
    asd_state asd_state;
    int given_length = 9;
    int actual_length = 8;

    expect_session_get_authenticated_conn(ST_OK);
    expect_any(__wrap_extnet_send, state);
    expect_any(__wrap_extnet_send, pconn);
    expect_any(__wrap_extnet_send, pv_buf);
    expect_value(__wrap_extnet_send, sz_len, given_length);
    EXTNET_SEND_RESULT = actual_length;

    assert_int_equal(ST_ERR,
                     send_out_msg_on_socket(&asd_state, (unsigned char*)&buffer,
                                            given_length));
}

void send_out_msg_on_socket_success_test(void** state)
{
    (void)state;
    unsigned char buffer[9];
    asd_state asd_state;
    int length = 9;

    expect_session_get_authenticated_conn(ST_OK);
    expect_any(__wrap_extnet_send, state);
    expect_any(__wrap_extnet_send, pconn);
    expect_any(__wrap_extnet_send, pv_buf);
    expect_value(__wrap_extnet_send, sz_len, length);
    EXTNET_SEND_RESULT = length;

    assert_int_equal(ST_OK, send_out_msg_on_socket(
                                &asd_state, (unsigned char*)&buffer, length));
}

void request_processing_loop_poll_failure_test(void** state)
{
    (void)state;
    asd_state asd_state;

    SESSION_FDS_COUNT = 1;
    SESSION_FDS[0] = 777;
    SESSION_TIMEOUT = 0;
    expect_session_getfds(ST_OK, 0);
    expect_asd_msg_get_fds(ST_OK, 0);
    expect_poll(SESSION_TIMEOUT, -1);

    assert_int_equal(ST_ERR, request_processing_loop(&asd_state));
}

void request_processing_loop_process_get_session_fds_failure_test(void** state)
{
    (void)state;
    ASD_MSG sdk;
    asd_state asd_state;
    asd_state.event_fd = 99;
    asd_state.asd_msg = &sdk;

    SESSION_FDS_COUNT = 1;
    SESSION_FDS[0] = 777;
    SESSION_TIMEOUT = 0;
    expect_asd_msg_get_fds(ST_OK, 0);
    expect_session_getfds(ST_ERR, 0);

    assert_int_equal(ST_ERR, request_processing_loop(&asd_state));
}

void request_processing_loop_expect_process_new_client_test(void** state)
{
    (void)state;
    ASD_MSG sdk;
    asd_state asd_state;
    asd_state.event_fd = 99;
    asd_state.asd_msg = &sdk;

    NUM_GPIO_FDS = 0;
    expect_asd_msg_get_fds(ST_OK, 0);

    SESSION_FDS_COUNT = 0;
    SESSION_TIMEOUT = 0;
    expect_session_getfds(ST_OK, 0);

    POLL_REVENTS[HOST_FD_INDEX] = POLLIN;
    POLL_REVENTS[GPIO_FD_INDEX] = 0;
    expect_poll(SESSION_TIMEOUT, 0);

    expect_extnet_accept_connection();
    expect_session_open();

    expect_any(__wrap_session_close_expired_unauth, state);
    expect_any(__wrap_session_lookup_conn, state);
    expect_any(__wrap_session_lookup_conn, fd);

    // loop will continue forever, so create an error to end the test
    expect_asd_msg_get_fds(ST_ERR, 1);
    expect_session_getfds(ST_ERR, 1);

    assert_int_equal(ST_ERR, request_processing_loop(&asd_state));
}

void request_processing_loop_gpio_failure_test(void** state)
{
    (void)state;
    ASD_MSG sdk;
    asd_state asd_state;
    asd_state.event_fd = 99;
    asd_state.asd_msg = &sdk;
    GPIO_FDS[0].fd = 1;
    GPIO_FDS[0].events = POLLIN;
    NUM_GPIO_FDS = 1;
    expect_asd_msg_get_fds(ST_OK, 0);

    SESSION_FDS_COUNT = 1;
    SESSION_FDS[0] = 777;
    SESSION_TIMEOUT = 0;
    expect_session_getfds(ST_OK, 0);

    POLL_REVENTS[0] = 0;
    POLL_REVENTS[1] = POLL_GPIO;
    expect_poll(SESSION_TIMEOUT, 0);

    expect_asd_msg_event(ST_ERR);

    // session will be closed
    expect_session_get_authenticated_conn(ST_OK);
    expect_on_client_disconnect(ST_OK);
    expect_session_close(ST_OK);

    // loop will continue forever, so create an error to end the test
    expect_asd_msg_get_fds(ST_ERR, 1);
    expect_session_getfds(ST_ERR, 1);

    assert_int_equal(ST_ERR, request_processing_loop(&asd_state));
}

void process_new_client_invalid_params_test(void** state)
{
    (void)state;
    asd_state asd;
    struct pollfd fds;
    int clients = 0;

    assert_int_equal(ST_ERR, process_new_client(NULL, &fds, 1, &clients, 1));
    assert_int_equal(ST_ERR, process_new_client(&asd, NULL, 1, &clients, 1));
    assert_int_equal(ST_ERR, process_new_client(&asd, &fds, 1, NULL, 1));
}

void process_new_client_accept_connection_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    struct pollfd fds;
    ExtNet extnet;
    int clients = 0;
    int fd = 97;
    asd.extnet = &extnet;
    asd.host_fd = fd;

    EXTNET_ACCEPT_CONNECTION = ST_ERR;
    expect_any(__wrap_extnet_accept_connection, state);
    expect_value(__wrap_extnet_accept_connection, ext_listen_sockfd, fd);
    expect_any(__wrap_extnet_accept_connection, pconn);

    assert_int_equal(ST_ERR, process_new_client(&asd, &fds, 1, &clients, 3));
}

void process_new_client_session_open_test(void** state)
{
    (void)state;
    asd_state asd;
    struct pollfd fds;
    ExtNet extnet;
    int clients = 0;
    asd.extnet = &extnet;

    expect_extnet_accept_connection();

    expect_any(__wrap_session_open, state);
    expect_any(__wrap_session_open, p_extconn);
    SESSION_OPEN_RESLUT = ST_ERR;

    assert_int_equal(ST_ERR, process_new_client(&asd, &fds, 1, &clients, 3));
}

void process_new_client_auth_none_success_test(void** state)
{
    (void)state;
    asd_state asd;
    struct pollfd fds[MAX_FDS];
    ExtNet extnet;
    int clients = 0;

    expect_process_new_client(&asd, &extnet);

    assert_int_equal(ST_OK,
                     process_new_client(&asd, fds, MAX_FDS, &clients, 3));
    assert_int_equal(1, clients);
    assert_int_equal(POLLIN, fds[3].revents);
}

void process_all_client_messages_invalid_params_test(void** state)
{
    (void)state;
    asd_state asd;
    struct pollfd fds[5];

    assert_int_equal(ST_ERR, process_all_client_messages(
                                 NULL, (const struct pollfd*)&fds, 1));
    assert_int_equal(ST_ERR, process_all_client_messages(&asd, NULL, 1));
}

void process_all_client_messages_success_test(void** state)
{
    (void)state;
    asd_state asd;
    int fd[2];
    fd[0] = 99;
    fd[1] = 678;
    struct pollfd fds[5];
    fds[0].fd = fd[0];
    fds[0].revents = POLLIN;
    fds[1].fd = fd[1];
    fds[1].revents = POLLIN;
    extnet_conn_t fake_conn;

    expect_process_all_client_messages(fd, 2, &fake_conn, ST_OK);

    assert_int_equal(ST_OK, process_all_client_messages(
                                &asd, (const struct pollfd*)&fds, 2));
}

void process_all_client_messages_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    int fd0 = 99;
    int fd1 = 678;
    struct pollfd fds[5];
    fds[0].fd = fd0;
    fds[0].revents = POLLIN;
    fds[1].fd = fd1;
    fds[1].revents = POLLIN;
    extnet_conn_t fake_conn;

    expect_any(__wrap_session_close_expired_unauth, state);

    expect_process_client_message(&fake_conn, fd0, ST_OK);
    expect_process_client_message(&fake_conn, fd1, ST_ERR);

    assert_int_equal(ST_ERR, process_all_client_messages(
                                 &asd, (const struct pollfd*)&fds, 2));
}

void process_client_message_invalid_params_test(void** state)
{
    (void)state;
    struct pollfd fd = {0};
    assert_int_equal(ST_ERR, process_client_message(NULL, fd));
}

void process_client_message_session_lookup_conn_failure_test(void** state)
{
    (void)state;
    int expected_fd = 45;
    asd_state sdk;
    struct pollfd fd = {0};
    fd.fd = expected_fd;

    expect_session_lookup_conn(NULL, expected_fd);

    assert_int_equal(ST_ERR, process_client_message(&sdk, fd));
}

void process_client_message_session_get_data_pending_failure_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    int expected_fd = 45;
    asd_state sdk;
    struct pollfd fd = {0};
    fd.fd = expected_fd;

    expect_session_lookup_conn(&conn, expected_fd);
    expect_session_get_data_pending(ST_ERR, false);

    assert_int_equal(ST_ERR, process_client_message(&sdk, fd));
}

void process_client_message_asd_msg_read_failure_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    int expected_fd = 45;
    asd_state sdk;
    struct pollfd fd = {0};
    fd.fd = expected_fd;

    expect_session_lookup_conn(&conn, expected_fd);
    expect_session_get_data_pending(ST_OK, true);
    expect_session_already_authenticated(ST_OK);
    expect_asd_msg_read(ST_ERR);
    // After the read failure, these items are called to close the
    // connection
    expect_set_config_defaults(ST_OK);
    expect_any_ASD_initialize_log_settings();
    expect_asd_msg_free(ST_OK);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR, process_client_message(&sdk, fd));
}

void process_client_message_session_set_data_pending_failure_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    int expected_fd = 45;
    asd_state sdk;
    struct pollfd fd = {0};
    fd.fd = expected_fd;

    expect_session_lookup_conn(&conn, expected_fd);
    expect_session_get_data_pending(ST_OK, true);
    expect_session_already_authenticated(ST_OK);
    expect_asd_msg_read(ST_OK);
    expect_session_data_pending(ST_ERR);

    assert_int_equal(ST_ERR, process_client_message(&sdk, fd));
}

void process_client_message_success_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    int expected_fd = 45;
    asd_state sdk;
    struct pollfd fd = {0};
    fd.fd = expected_fd;

    expect_process_client_message(&conn, expected_fd, ST_OK);

    assert_int_equal(ST_OK, process_client_message(&sdk, fd));

    assert_true(SESSION_DATA_PENDING_STORED_VALUE);
}

void read_data_invalid_params_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    char* buffer = "blah";
    size_t num = 0;
    bool pending = false;
    assert_int_equal(ST_ERR,
                     read_data(NULL, &connection, buffer, &num, &pending));
    assert_int_equal(ST_ERR, read_data(&asd, NULL, buffer, &num, &pending));
    assert_int_equal(ST_ERR,
                     read_data(&asd, &connection, NULL, &num, &pending));
    assert_int_equal(ST_ERR,
                     read_data(&asd, &connection, buffer, NULL, &pending));
    assert_int_equal(ST_ERR, read_data(&asd, &connection, buffer, &num, NULL));
}

void read_data_extnet_recv_failure_0_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    char* buffer = "blah";
    size_t num = 50;
    int expected_recv = 0;
    int expected_remaining = (int)(num);
    bool pending = false;

    expect_extnet_recv(expected_recv, num, false);
    expect_on_client_disconnect(ST_OK);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR,
                     read_data(&asd, &connection, buffer, &num, &pending));
    assert_int_equal(num, expected_remaining);
}

void read_data_extnet_recv_failure_neg1_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    char* buffer = "blah";
    size_t num = 50;
    int expected_recv = -1;
    int expected_remaining = (int)(num);
    bool pending = false;

    expect_extnet_recv(expected_recv, num, false);
    expect_on_client_disconnect(ST_OK);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR,
                     read_data(&asd, &connection, buffer, &num, &pending));
    assert_int_equal(num, expected_remaining);
}

void read_data_success_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    char* buffer = "blah";
    size_t num = 50;
    int expected_recv = 10;
    int expected_remaining = (int)(num - expected_recv);
    bool pending = false;

    expect_extnet_recv(expected_recv, num, true);

    assert_int_equal(ST_OK,
                     read_data(&asd, &connection, buffer, &num, &pending));
    assert_int_equal(num, expected_remaining);
    assert_true(pending);
}

void ensure_client_authenticated_invalid_params_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, ensure_client_authenticated(NULL, &connection));
    assert_int_equal(ST_ERR, ensure_client_authenticated(&asd, NULL));
}

void ensure_client_authenticated_already_authenticated_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    expect_session_already_authenticated(ST_OK);

    assert_int_equal(ST_OK, ensure_client_authenticated(&asd, &connection));
}

void ensure_client_authenticated_auth_client_handshake_failure_test(
    void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;

    expect_session_already_authenticated(ST_ERR);
    expect_auth_client_handshake(ST_ERR);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR, ensure_client_authenticated(&asd, &connection));
}

void ensure_client_authenticated_session_auth_complete_failure_test(
    void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;

    expect_session_already_authenticated(ST_ERR);
    expect_auth_client_handshake(ST_OK);
    expect_session_auth_complete(ST_ERR);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR, ensure_client_authenticated(&asd, &connection));
}

void ensure_client_authenticated_connect_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    ASD_MSG sdk;
    extnet_conn_t connection;
    asd.asd_msg = &sdk;

    expect_session_already_authenticated(ST_ERR);
    expect_auth_client_handshake(ST_OK);
    expect_session_auth_complete(ST_OK);
    expect_on_client_connect(asd.asd_msg, ST_ERR);
    expect_on_client_disconnect(ST_ERR);
    expect_session_close(ST_OK);

    assert_int_equal(ST_ERR, ensure_client_authenticated(&asd, &connection));
}

void ensure_client_authenticated_connect_failure_memcpy_fail_test(void** state)
{
    (void)state;
    asd_state asd;
    ASD_MSG sdk;
    extnet_conn_t connection;
    asd.asd_msg = &sdk;

    expect_session_already_authenticated(ST_ERR);
    expect_auth_client_handshake(ST_OK);
    expect_session_auth_complete(ST_OK);
    expect_on_client_connect(asd.asd_msg, ST_ERR);
    expect_on_client_disconnect(ST_ERR);
    expect_session_close(ST_OK);
    MEMCPY_SAFE_RESULT = 1;
    assert_int_equal(ST_ERR, ensure_client_authenticated(&asd, &connection));
}

void ensure_client_authenticated_success_test(void** state)
{
    (void)state;
    asd_state asd;
    ASD_MSG sdk;
    extnet_conn_t connection;
    asd.asd_msg = &sdk;

    expect_session_already_authenticated(ST_ERR);
    expect_auth_client_handshake(ST_OK);
    expect_session_auth_complete(ST_OK);
    expect_on_client_connect(asd.asd_msg, ST_OK);

    assert_int_equal(ST_OK, ensure_client_authenticated(&asd, &connection));
}

void on_client_connect_invalid_params_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, on_client_connect(NULL, &connection));
    assert_int_equal(ST_ERR, on_client_connect(&asd, NULL));
}

void on_client_connect_set_config_defaults_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    connection.sockfd = 8;

    expect_getpeername_check_fd(0, connection.sockfd);
    expect_set_config_defaults(ST_ERR);

    assert_int_equal(ST_ERR, on_client_connect(&asd, &connection));
}

void on_client_connect_asd_msg_init_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    extnet_conn_t connection;
    connection.sockfd = 8;
    expect_getpeername_check_fd(0, connection.sockfd);
    expect_set_config_defaults(ST_OK);
    expect_any_asd_msg_init(NULL);

    assert_int_equal(ST_ERR, on_client_connect(&asd, &connection));
}

void on_client_connect_success_test(void** state)
{
    (void)state;
    ASD_MSG sdk;
    asd_state asd;
    extnet_conn_t connection;
    connection.sockfd = 8;
    expect_getpeername_check_fd(0, connection.sockfd);
    expect_set_config_defaults(ST_OK);
    expect_any_asd_msg_init(&sdk);
    expect_any_ASD_initialize_log_settings();

    assert_int_equal(ST_OK, on_client_connect(&asd, &connection));
}

void on_client_disconnect_invalid_params_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, on_client_disconnect(NULL));
}

void on_client_disconnect_set_config_defaults_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    expect_set_config_defaults(ST_ERR);

    assert_int_equal(ST_ERR, on_client_disconnect(&asd));
}

void on_client_disconnect_asd_msg_free_failure_test(void** state)
{
    (void)state;
    asd_state asd;
    expect_set_config_defaults(ST_OK);

    expect_any_ASD_initialize_log_settings();
    expect_asd_msg_free(ST_ERR);

    assert_int_equal(ST_ERR, on_client_disconnect(&asd));
}

void on_client_disconnect_success_test(void** state)
{
    (void)state;
    asd_state asd;
    expect_set_config_defaults(ST_OK);

    expect_any_ASD_initialize_log_settings();
    expect_asd_msg_free(ST_OK);

    assert_int_equal(ST_OK, on_client_disconnect(&asd));
}

void close_connection_already_closed_test(void** state)
{
    (void)state;
    asd_state asd;
    expect_session_get_authenticated_conn(ST_ERR);

    assert_int_equal(ST_OK, close_connection(&asd));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(asd_main_process_command_line_failure_test),
        cmocka_unit_test(
            asd_main_process_command_line_request_processing_failure_test),
        cmocka_unit_test(process_command_line_sets_defaults_test),
        cmocka_unit_test(process_command_line_port_number_test),
        cmocka_unit_test(process_command_line_log_to_syslog_test),
        cmocka_unit_test(process_command_line_set_unsecure_mode_test),
        cmocka_unit_test(process_command_line_set_key_file_test),
        cmocka_unit_test(process_command_line_set_net_bind_device_test),
        cmocka_unit_test(process_command_line_set_i2c_test),
        cmocka_unit_test(process_command_line_log_level_test),
        cmocka_unit_test(process_command_line_log_streams_test),
        cmocka_unit_test(process_command_line_unknown_arg_test),
        cmocka_unit_test(
            init_asd_state_handles_set_config_defaults_failure_test),
        cmocka_unit_test(init_asd_state_handles_extnet_init_failure_test),
        cmocka_unit_test(init_asd_state_handles_auth_init_failure_test),
        cmocka_unit_test(init_asd_state_handles_session_init_failure_test),
        cmocka_unit_test(init_asd_state_handles_eventfd_failure_test),
        cmocka_unit_test(
            init_asd_state_handles_extnet_open_external_socket_failure_test),
        cmocka_unit_test(init_asd_state_returns_true_test),

        cmocka_unit_test(asd_main_init_asd_state_failure_test),

        cmocka_unit_test(deinit_asd_state_test),

        cmocka_unit_test(send_out_msg_on_socket_params_test),
        cmocka_unit_test(send_out_msg_on_socket_no_authenticated_socket_test),
        cmocka_unit_test(send_out_msg_on_socket_send_failure_test),
        cmocka_unit_test(send_out_msg_on_socket_success_test),

        cmocka_unit_test(request_processing_loop_poll_failure_test),
        cmocka_unit_test(
            request_processing_loop_process_get_session_fds_failure_test),
        cmocka_unit_test(
            request_processing_loop_expect_process_new_client_test),
        cmocka_unit_test(request_processing_loop_gpio_failure_test),
        cmocka_unit_test(process_new_client_invalid_params_test),
        cmocka_unit_test(process_new_client_accept_connection_failure_test),
        cmocka_unit_test(process_new_client_session_open_test),
        cmocka_unit_test(process_new_client_auth_none_success_test),

        cmocka_unit_test(process_all_client_messages_invalid_params_test),
        cmocka_unit_test(process_all_client_messages_success_test),
        cmocka_unit_test(process_all_client_messages_failure_test),

        cmocka_unit_test(process_client_message_invalid_params_test),
        cmocka_unit_test(
            process_client_message_session_lookup_conn_failure_test),
        cmocka_unit_test(
            process_client_message_session_get_data_pending_failure_test),
        cmocka_unit_test(process_client_message_asd_msg_read_failure_test),
        cmocka_unit_test(
            process_client_message_session_set_data_pending_failure_test),
        cmocka_unit_test(process_client_message_success_test),

        cmocka_unit_test(read_data_invalid_params_test),
        cmocka_unit_test(read_data_extnet_recv_failure_0_test),
        cmocka_unit_test(read_data_extnet_recv_failure_neg1_test),
        cmocka_unit_test(read_data_success_test),

        cmocka_unit_test(ensure_client_authenticated_invalid_params_test),
        cmocka_unit_test(
            ensure_client_authenticated_already_authenticated_test),
        cmocka_unit_test(
            ensure_client_authenticated_auth_client_handshake_failure_test),
        cmocka_unit_test(
            ensure_client_authenticated_session_auth_complete_failure_test),
        cmocka_unit_test(ensure_client_authenticated_connect_failure_test),
        cmocka_unit_test(
            ensure_client_authenticated_connect_failure_memcpy_fail_test),
        cmocka_unit_test(ensure_client_authenticated_success_test),
        cmocka_unit_test(on_client_connect_invalid_params_test),
        cmocka_unit_test(on_client_connect_set_config_defaults_failure_test),
        cmocka_unit_test(on_client_connect_asd_msg_init_failure_test),
        cmocka_unit_test(on_client_connect_success_test),

        cmocka_unit_test(on_client_disconnect_invalid_params_test),
        cmocka_unit_test(on_client_disconnect_set_config_defaults_failure_test),
        cmocka_unit_test(on_client_disconnect_asd_msg_free_failure_test),
        cmocka_unit_test(on_client_disconnect_success_test),
        cmocka_unit_test(close_connection_already_closed_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
