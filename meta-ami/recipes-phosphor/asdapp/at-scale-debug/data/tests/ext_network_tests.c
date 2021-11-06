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

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/tcp.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>

#include "../ext_network.h"
#include "../logging.h"
#include "cmocka.h"

STATUS FAKE_EXTTCP_INIT = ST_OK;
STATUS fake_exttcp_init(void* p_hdlr_data)
{
    return FAKE_EXTTCP_INIT;
}

void fake_exttcp_cleanup(void)
{
    return;
}

STATUS FAKE_ON_ACCEPT_RESULT = ST_OK;
STATUS fake_exttcp_on_accept(void* net_state, extnet_conn_t* pconn)
{
    return FAKE_ON_ACCEPT_RESULT;
}

STATUS FAKE_INIT_CLIENT_RESULT = ST_OK;
STATUS fake_exttcp_init_client(extnet_conn_t* pconn)
{
    return FAKE_INIT_CLIENT_RESULT;
}

STATUS ON_CLOSE_CLIENT_RESULT = ST_OK;
STATUS fake_exttcp_on_close_client(extnet_conn_t* pconn)
{
    return ON_CLOSE_CLIENT_RESULT;
}

int FAKE_RECV_RESULT = 0;
int fake_exttcp_recv(extnet_conn_t* pconn, void* pv_buf, size_t sz_len,
                     bool* b_data_pending)
{
    return FAKE_RECV_RESULT;
}

int FAKE_SEND_RESULT = 0;
int fake_exttcp_send(extnet_conn_t* pconn, void* pv_buf, size_t sz_len)
{
    return FAKE_SEND_RESULT;
}

extnet_hdlrs_t tcp_hdlrs = {
    fake_exttcp_init,        fake_exttcp_on_accept, fake_exttcp_on_close_client,
    fake_exttcp_init_client, fake_exttcp_recv,      fake_exttcp_send,
    fake_exttcp_cleanup,
};

extnet_hdlrs_t tls_hdlrs = {
    fake_exttcp_init,        fake_exttcp_on_accept, fake_exttcp_on_close_client,
    fake_exttcp_init_client, fake_exttcp_recv,      fake_exttcp_send,
    fake_exttcp_cleanup,
};

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

typedef void (*sighandler_t)(int);
sighandler_t __real_signal(int signum, sighandler_t handler);
sighandler_t __wrap_signal(int signum, sighandler_t handler)
{
    if (signum == SIGPIPE)
    {
        check_expected(signum);
        check_expected(handler);
    }
    else
    {
        return __real_signal(signum, handler);
    }
    return 0;
}

int SOCKET_RESULT = 0;
int __wrap_socket(int domain, int type, int protocol)
{
    check_expected(domain);
    check_expected(type);
    check_expected(protocol);
    return SOCKET_RESULT;
}

void expect_socket_create(int socket_result)
{
    SOCKET_RESULT = socket_result;
    expect_value(__wrap_socket, domain, AF_INET6);
    expect_value(__wrap_socket, type, SOCK_STREAM);
    expect_value(__wrap_socket, protocol, 0);
}

int SETSOCKOPT_TCP_NODELAY_RESULT = 0;
int SETSOCKOPT_SO_REUSEADDR_RESULT = 0;
int SETSOCKOPT_SO_BINDTODEVICE_RESULT = 0;
int __wrap_setsockopt(int sockfd, int level, int optname, const void* optval,
                      socklen_t optlen)
{
    check_expected(sockfd);
    check_expected(level);
    check_expected(optname);
    check_expected_ptr(optval);
    check_expected(optlen);
    if (optname == TCP_NODELAY)
        return SETSOCKOPT_TCP_NODELAY_RESULT;
    else if (optname == SO_REUSEADDR)
        return SETSOCKOPT_SO_REUSEADDR_RESULT;
    else if (optname == SO_BINDTODEVICE)
        return SETSOCKOPT_SO_BINDTODEVICE_RESULT;
    else
        return 0;
}

void expect_setsocketopt(int optname, int result, int expected_socket,
                         const char* cp_bind_if)
{
    if (optname == TCP_NODELAY)
    {
        SETSOCKOPT_TCP_NODELAY_RESULT = result;
        expect_value(__wrap_setsockopt, sockfd, expected_socket);
        expect_value(__wrap_setsockopt, level, IPPROTO_TCP);
        expect_value(__wrap_setsockopt, optname, TCP_NODELAY);
        expect_any(__wrap_setsockopt, optval);
        expect_value(__wrap_setsockopt, optlen, sizeof(int));
    }
    else if (optname == SO_REUSEADDR)
    {
        SETSOCKOPT_SO_REUSEADDR_RESULT = result;
        expect_value(__wrap_setsockopt, sockfd, expected_socket);
        expect_value(__wrap_setsockopt, level, SOL_SOCKET);
        expect_value(__wrap_setsockopt, optname, SO_REUSEADDR);
        expect_any(__wrap_setsockopt, optval);
        expect_value(__wrap_setsockopt, optlen, sizeof(int));
    }
    else if (optname == SO_BINDTODEVICE)
    {
        SETSOCKOPT_SO_BINDTODEVICE_RESULT = result;
        expect_value(__wrap_setsockopt, sockfd, expected_socket);
        expect_value(__wrap_setsockopt, level, SOL_SOCKET);
        expect_value(__wrap_setsockopt, optname, SO_BINDTODEVICE);
        expect_any(__wrap_setsockopt, optval);
        expect_value(__wrap_setsockopt, optlen, strlen(cp_bind_if));
    }
}

int BIND_RESULT = 0;
int __wrap_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    check_expected(sockfd);
    check_expected_ptr(addr);
    check_expected(addrlen);
    return BIND_RESULT;
}

void expect_bind(int result, int fd, int len)
{
    BIND_RESULT = result;
    expect_value(__wrap_bind, sockfd, fd);
    expect_any(__wrap_bind, addr);
    expect_value(__wrap_bind, addrlen, len);
}

int LISTEN_RESULT = 0;
int __wrap_listen(int sockfd, int backlog)
{
    check_expected(sockfd);
    check_expected(backlog);
    return LISTEN_RESULT;
}

void expect_listen(int result, int socket, int sessions)
{
    LISTEN_RESULT = result;
    expect_value(__wrap_listen, sockfd, socket);
    expect_value(__wrap_listen, backlog, sessions);
}

int ACCEPT_RESULT = 0;
int __wrap_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    check_expected(sockfd);
    check_expected_ptr(addr);
    check_expected_ptr(addrlen);
    return ACCEPT_RESULT;
}

int CLOSE_RESULT = 0;
int __wrap_close(int fd)
{
    check_expected(fd);
    return CLOSE_RESULT;
}

void extnet_init_invalid_params_test(void** state)
{
    (void)state;
    assert_null(extnet_init(EXTNET_HDLR_TLS, NULL, 0));
}

void extnet_init_malloc_fail_test(void** state)
{
    (void)state;
    int hndlr_data = 0;
    expect_value(__wrap_malloc, size, sizeof(ExtNet));
    malloc_fail = true;
    assert_null(extnet_init(EXTNET_HDLR_TLS, &hndlr_data, 0));
    malloc_fail = false;
}

void extnet_init_unsupported_type_test(void** state)
{
    (void)state;
    int hndlr_data = 0;
    // 99 will cast to an unsupported type
    assert_null(extnet_init(99, &hndlr_data, 0));
}

void extnet_init_handler_init_fail_test(void** state)
{
    (void)state;
    int expected_max = 8;
    int hndlr_data = 0;
    FAKE_EXTTCP_INIT = ST_ERR;
    assert_null(
        extnet_init(EXTNET_HDLR_NON_ENCRYPT, &hndlr_data, expected_max));
    FAKE_EXTTCP_INIT = ST_OK;
}

void extnet_init_non_encrypted_success_test(void** state)
{
    (void)state;
    int expected_max = 8;
    int hndlr_data = 0;

    expect_value(__wrap_signal, signum, SIGPIPE);
    expect_value(__wrap_signal, handler, SIG_IGN);
    ExtNet* extnet =
        extnet_init(EXTNET_HDLR_NON_ENCRYPT, &hndlr_data, expected_max);
    assert_non_null(extnet);
    assert_int_equal(extnet->n_max_sessions, expected_max);
    free(extnet);
}

void extnet_init_tls_success_test(void** state)
{
    (void)state;
    int expected_max = 8;
    int hndlr_data = 0;

    expect_value(__wrap_signal, signum, SIGPIPE);
    expect_value(__wrap_signal, handler, SIG_IGN);
    ExtNet* extnet = extnet_init(EXTNET_HDLR_TLS, &hndlr_data, expected_max);
    assert_non_null(extnet);
    assert_int_equal(extnet->n_max_sessions, expected_max);
    free(extnet);
}

void extnet_open_external_socket_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    const char* bind_if = "eth2";
    uint16_t port = 5123;
    int sock;
    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(NULL, bind_if, port, &sock));
    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, bind_if, port, NULL));
}

void extnet_open_external_socket_create_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int sock;

    expect_socket_create(-1);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, NULL, 5123, &sock));
}

void extnet_open_external_socket_setsockopt_TCP_NODELAY_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;

    expect_socket_create(expected_socket);

    expect_setsocketopt(TCP_NODELAY, -1, expected_socket, NULL);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, NULL, 5123, &sock));
}

void extnet_open_external_socket_setsockopt_SO_REUSEADDR_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;

    expect_socket_create(expected_socket);
    expect_setsocketopt(TCP_NODELAY, 0, expected_socket, NULL);
    expect_setsocketopt(SO_REUSEADDR, -1, expected_socket, NULL);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, NULL, 5123, &sock));
}

void extnet_open_external_socket_setsockopt_SO_BINDTODEVICE_fail_test(
    void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;
    const char* eth = "eth2";

    expect_socket_create(expected_socket);
    expect_setsocketopt(TCP_NODELAY, 0, expected_socket, NULL);
    expect_setsocketopt(SO_REUSEADDR, 0, expected_socket, NULL);
    expect_setsocketopt(SO_BINDTODEVICE, -1, expected_socket, eth);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, eth, 5123, &sock));
}

void extnet_open_external_socket_bind_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;
    const char* eth = "eth2";

    expect_socket_create(expected_socket);
    expect_setsocketopt(TCP_NODELAY, 0, expected_socket, NULL);
    expect_setsocketopt(SO_REUSEADDR, 0, expected_socket, NULL);
    expect_setsocketopt(SO_BINDTODEVICE, 0, expected_socket, eth);
    expect_bind(-1, expected_socket, sizeof(struct sockaddr_in6));
    CLOSE_RESULT = 0;
    expect_any(__wrap_close, fd);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, eth, 5123, &sock));
}

void extnet_open_external_socket_listen_fail_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;
    const char* eth = "eth2";
    extnet.n_max_sessions = 65;

    expect_socket_create(expected_socket);
    expect_setsocketopt(TCP_NODELAY, 0, expected_socket, NULL);
    expect_setsocketopt(SO_REUSEADDR, 0, expected_socket, NULL);
    expect_setsocketopt(SO_BINDTODEVICE, 0, expected_socket, eth);
    expect_bind(0, expected_socket, sizeof(struct sockaddr_in6));
    expect_listen(-1, expected_socket, extnet.n_max_sessions);
    CLOSE_RESULT = 0;
    expect_any(__wrap_close, fd);

    assert_int_equal(ST_ERR,
                     extnet_open_external_socket(&extnet, eth, 5123, &sock));
}

void extnet_open_external_socket_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    int expected_socket = 4;
    int sock;
    const char* eth = "eth2";
    extnet.n_max_sessions = 65;

    expect_socket_create(expected_socket);
    expect_setsocketopt(TCP_NODELAY, 0, expected_socket, NULL);
    expect_setsocketopt(SO_REUSEADDR, 0, expected_socket, NULL);
    expect_setsocketopt(SO_BINDTODEVICE, 0, expected_socket, eth);
    expect_bind(0, expected_socket, sizeof(struct sockaddr_in6));
    expect_listen(0, expected_socket, extnet.n_max_sessions);

    assert_int_equal(ST_OK,
                     extnet_open_external_socket(&extnet, eth, 5123, &sock));
    assert_int_equal(sock, expected_socket);
}

void extnet_accept_connection_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    assert_int_equal(ST_ERR, extnet_accept_connection(NULL, 0, &connection));
    assert_int_equal(ST_ERR, extnet_accept_connection(&extnet, 0, NULL));
    extnet.p_hdlrs = NULL;
    assert_int_equal(ST_ERR, extnet_accept_connection(&extnet, 0, &connection));
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->on_accept = NULL;
    assert_int_equal(ST_ERR, extnet_accept_connection(&extnet, 0, &connection));
    extnet.p_hdlrs->on_accept = fake_exttcp_on_accept;
}

void extnet_accept_connection_failure_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    int sockfd = 78;

    ACCEPT_RESULT = -1;
    expect_value(__wrap_accept, sockfd, sockfd);
    expect_any(__wrap_accept, addr);
    expect_any(__wrap_accept, addrlen);

    assert_int_equal(ST_ERR,
                     extnet_accept_connection(&extnet, sockfd, &connection));
}

void extnet_accept_connection_on_accept_failure_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet_conn_t connection;
    int sockfd = 78;

    ACCEPT_RESULT = 0;
    expect_value(__wrap_accept, sockfd, sockfd);
    expect_any(__wrap_accept, addr);
    expect_any(__wrap_accept, addrlen);
    CLOSE_RESULT = 0;
    expect_any(__wrap_close, fd);

    FAKE_ON_ACCEPT_RESULT = ST_ERR;

    assert_int_equal(ST_ERR,
                     extnet_accept_connection(&extnet, sockfd, &connection));
}

void extnet_accept_connection_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet_conn_t connection;
    int sockfd = 78;

    ACCEPT_RESULT = sockfd;
    expect_value(__wrap_accept, sockfd, sockfd);
    expect_any(__wrap_accept, addr);
    expect_any(__wrap_accept, addrlen);

    FAKE_ON_ACCEPT_RESULT = ST_OK;

    assert_int_equal(ST_OK,
                     extnet_accept_connection(&extnet, sockfd, &connection));
    assert_int_equal(connection.sockfd, sockfd);
}

void extnet_init_client_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    extnet.p_hdlrs = &tcp_hdlrs;
    assert_int_equal(ST_ERR, extnet_init_client(NULL, &connection));
    assert_int_equal(ST_ERR, extnet_init_client(&extnet, NULL));
    extnet.p_hdlrs->init_client = NULL;
    assert_int_equal(ST_ERR, extnet_init_client(&extnet, &connection));
    extnet.p_hdlrs = NULL;
    assert_int_equal(ST_ERR, extnet_init_client(&extnet, &connection));
}

void extnet_init_client_failure_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->init_client = fake_exttcp_init_client;
    FAKE_INIT_CLIENT_RESULT = ST_ERR;
    assert_int_equal(ST_ERR, extnet_init_client(&extnet, &connection));
}

void extnet_init_client_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->init_client = fake_exttcp_init_client;
    FAKE_INIT_CLIENT_RESULT = ST_OK;
    assert_int_equal(ST_OK, extnet_init_client(&extnet, &connection));
    assert_int_equal(UNUSED_SOCKET_FD, connection.sockfd);
}

void extnet_close_client_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    extnet.p_hdlrs = &tcp_hdlrs;
    assert_int_equal(ST_ERR, extnet_close_client(NULL, &connection));
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, NULL));
    extnet.p_hdlrs->on_close_client = NULL;
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, &connection));
    extnet.p_hdlrs = NULL;
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, &connection));
}

void extnet_close_client_already_closed_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    connection.sockfd = UNUSED_SOCKET_FD;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->on_close_client = fake_exttcp_on_close_client;
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, &connection));
}

void extnet_close_client_failure_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    connection.sockfd = 9;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->on_close_client = fake_exttcp_on_close_client;
    ON_CLOSE_CLIENT_RESULT = ST_ERR;
    CLOSE_RESULT = 0;
    expect_any(__wrap_close, fd);
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, &connection));
}

void extnet_close_client_socket_close_failure_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    connection.sockfd = 9;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->on_close_client = fake_exttcp_on_close_client;
    ON_CLOSE_CLIENT_RESULT = ST_OK;
    CLOSE_RESULT = -1;
    expect_any(__wrap_close, fd);
    assert_int_equal(ST_ERR, extnet_close_client(&extnet, &connection));
}

void extnet_close_client_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    connection.sockfd = 9;
    extnet.p_hdlrs = &tcp_hdlrs;
    extnet.p_hdlrs->on_close_client = fake_exttcp_on_close_client;
    ON_CLOSE_CLIENT_RESULT = ST_OK;

    CLOSE_RESULT = 0;
    expect_value(__wrap_close, fd, connection.sockfd);
    assert_int_equal(ST_OK, extnet_close_client(&extnet, &connection));
    assert_int_equal(connection.sockfd, UNUSED_SOCKET_FD);
    assert_null(connection.p_hdlr_data);
}

void extnet_is_client_closed_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;

    assert_false(extnet_is_client_closed(NULL, &connection));
    assert_false(extnet_is_client_closed(&extnet, NULL));
}

void extnet_is_client_closed_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;

    connection.sockfd = UNUSED_SOCKET_FD;
    assert_true(extnet_is_client_closed(&extnet, &connection));
    connection.sockfd = 0;
    assert_false(extnet_is_client_closed(&extnet, &connection));
    connection.sockfd = 56;
    assert_false(extnet_is_client_closed(&extnet, &connection));
}

void extnet_recv_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    size_t len = 15;
    char data[len];
    bool pending;

    assert_int_equal(-1, extnet_recv(NULL, &connection, &data, len, &pending));
    assert_int_equal(-1, extnet_recv(&extnet, NULL, &data, len, &pending));
    assert_int_equal(-1,
                     extnet_recv(&extnet, &connection, NULL, len, &pending));
    assert_int_equal(-1, extnet_recv(&extnet, &connection, &data, len, NULL));
    extnet.p_hdlrs = NULL;
    assert_int_equal(-1,
                     extnet_recv(&extnet, &connection, &data, len, &pending));
    extnet.p_hdlrs = &tcp_hdlrs;
}

void extnet_recv_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    size_t len = 15;
    char data[len];
    bool pending;
    int expected = 6;
    extnet.p_hdlrs = &tcp_hdlrs;
    FAKE_RECV_RESULT = expected;
    assert_int_equal(expected,
                     extnet_recv(&extnet, &connection, &data, len, &pending));
}

void extnet_send_invalid_params_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    size_t len = 15;
    char data[len];

    assert_int_equal(-1, extnet_send(NULL, &connection, &data, len));
    assert_int_equal(-1, extnet_send(&extnet, NULL, &data, len));
    assert_int_equal(-1, extnet_send(&extnet, &connection, NULL, len));
    extnet.p_hdlrs = NULL;
    assert_int_equal(-1, extnet_send(&extnet, &connection, &data, len));
    extnet.p_hdlrs = &tcp_hdlrs;
}

void extnet_send_success_test(void** state)
{
    (void)state;
    ExtNet extnet;
    extnet_conn_t connection;
    size_t len = 15;
    char data[len];
    int expected = 9;
    extnet.p_hdlrs = &tcp_hdlrs;
    FAKE_SEND_RESULT = expected;
    assert_int_equal(expected, extnet_send(&extnet, &connection, &data, len));
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(extnet_init_invalid_params_test),
        cmocka_unit_test(extnet_init_malloc_fail_test),
        cmocka_unit_test(extnet_init_unsupported_type_test),
        cmocka_unit_test(extnet_init_handler_init_fail_test),
        cmocka_unit_test(extnet_init_non_encrypted_success_test),
        cmocka_unit_test(extnet_init_tls_success_test),
        cmocka_unit_test(extnet_open_external_socket_invalid_params_test),
        cmocka_unit_test(extnet_open_external_socket_create_fail_test),
        cmocka_unit_test(
            extnet_open_external_socket_setsockopt_TCP_NODELAY_fail_test),
        cmocka_unit_test(
            extnet_open_external_socket_setsockopt_SO_REUSEADDR_fail_test),
        cmocka_unit_test(
            extnet_open_external_socket_setsockopt_SO_BINDTODEVICE_fail_test),
        cmocka_unit_test(extnet_open_external_socket_bind_fail_test),
        cmocka_unit_test(extnet_open_external_socket_listen_fail_test),
        cmocka_unit_test(extnet_open_external_socket_success_test),
        cmocka_unit_test(extnet_accept_connection_invalid_params_test),
        cmocka_unit_test(extnet_accept_connection_failure_test),
        cmocka_unit_test(extnet_accept_connection_on_accept_failure_test),
        cmocka_unit_test(extnet_accept_connection_success_test),
        cmocka_unit_test(extnet_init_client_invalid_params_test),
        cmocka_unit_test(extnet_init_client_failure_test),
        cmocka_unit_test(extnet_init_client_success_test),
        cmocka_unit_test(extnet_close_client_invalid_params_test),
        cmocka_unit_test(extnet_close_client_already_closed_test),
        cmocka_unit_test(extnet_close_client_failure_test),
        cmocka_unit_test(extnet_close_client_socket_close_failure_test),
        cmocka_unit_test(extnet_close_client_success_test),
        cmocka_unit_test(extnet_is_client_closed_invalid_params_test),
        cmocka_unit_test(extnet_is_client_closed_success_test),
        cmocka_unit_test(extnet_recv_invalid_params_test),
        cmocka_unit_test(extnet_recv_success_test),
        cmocka_unit_test(extnet_send_invalid_params_test),
        cmocka_unit_test(extnet_send_success_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
