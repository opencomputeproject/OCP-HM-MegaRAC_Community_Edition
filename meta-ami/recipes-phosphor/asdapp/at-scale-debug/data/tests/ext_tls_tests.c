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

#include <errno.h>
#include <getopt.h>
#include <openssl/ossl_typ.h>
#include <openssl/ssl.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>

#include "../ext_network.h"
#include "../ext_tls.h"
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

unsigned long __wrap_ERR_get_error(void)
{
    return 0;
}

void __wrap_ERR_error_string_n(unsigned long e, char* buf, size_t len)
{
    check_expected(e);
    check_expected_ptr(buf);
    check_expected(len);
}

void expect_ERR_error_string_n()
{
    expect_any(__wrap_ERR_error_string_n, e);
    expect_any(__wrap_ERR_error_string_n, buf);
    expect_any(__wrap_ERR_error_string_n, len);
}

void __wrap_ERR_print_errors_fp(FILE* fp)
{
    check_expected_ptr(fp);
}

int __wrap_OPENSSL_init_ssl(uint64_t opts,
                            const OPENSSL_INIT_SETTINGS* settings)
{
    check_expected(opts);
    check_expected_ptr(settings);
    return 0;
}

void expect_SSL_load_error_strings()
{
    expect_value(__wrap_OPENSSL_init_ssl, opts,
                 OPENSSL_INIT_LOAD_SSL_STRINGS |
                     OPENSSL_INIT_LOAD_CRYPTO_STRINGS);
    expect_any(__wrap_OPENSSL_init_ssl, settings);
}

void expect_OpenSSL_add_ssl_algorithms()
{
    expect_value(__wrap_OPENSSL_init_ssl, opts, 0);
    expect_any(__wrap_OPENSSL_init_ssl, settings);
}

SSL_CTX* SSL_CTX_NEW_RESULT = NULL;
SSL_CTX* __wrap_SSL_CTX_new(const SSL_METHOD* meth)
{
    check_expected_ptr(meth);
    return SSL_CTX_NEW_RESULT;
}

int DUMMY_SSL_CONTEXT;
void expect_SSL_CTX_new(bool success)
{
    expect_any(__wrap_SSL_CTX_new, meth);
    if (success)
        SSL_CTX_NEW_RESULT = (SSL_CTX*)&DUMMY_SSL_CONTEXT;
    else
        SSL_CTX_NEW_RESULT = NULL;
}

void __wrap_SSL_CTX_free(SSL_CTX* ctx)
{
    check_expected_ptr(ctx);
}

unsigned long SSL_CTX_SET_OPTIONS_RESULT = 0;
unsigned long __wrap_SSL_CTX_set_options(SSL_CTX* ctx, unsigned long op)
{
    check_expected_ptr(ctx);
    check_expected(op);
    return SSL_CTX_SET_OPTIONS_RESULT;
}

void expect_SSL_CTX_set_options()
{
    expect_any(__wrap_SSL_CTX_set_options, ctx);
    expect_value(__wrap_SSL_CTX_set_options, op,
                 SSL_OP_NO_SSLv3 | SSL_OP_NO_SSLv2 | SSL_OP_NO_TLSv1 |
                     SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION |
                     SSL_OP_CIPHER_SERVER_PREFERENCE);
    SSL_CTX_SET_OPTIONS_RESULT = 0;
}

int SSL_CTX_SET_CIPHER_LIST_RESULT = 1;
int __wrap_SSL_CTX_set_cipher_list(SSL_CTX* ctx, const char* str)
{
    check_expected_ptr(ctx);
    check_expected_ptr(str);
    return SSL_CTX_SET_CIPHER_LIST_RESULT;
}

void expect_SSL_CTX_set_cipher_list(bool success)
{
    if (success)
        SSL_CTX_SET_CIPHER_LIST_RESULT = 1;
    else
        SSL_CTX_SET_CIPHER_LIST_RESULT = 0;
    expect_any(__wrap_SSL_CTX_set_cipher_list, ctx);
    expect_string(__wrap_SSL_CTX_set_cipher_list, str,
                  "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:!"
                  "aNULL:!eNULL@STRENGTH");
}

int SSL_USE_CERTIFICATE_FILE_RESULT = 0;
int __wrap_SSL_CTX_use_certificate_file(SSL_CTX* ctx, const char* file,
                                        int type)
{
    check_expected_ptr(ctx);
    check_expected_ptr(file);
    check_expected(type);
    return SSL_USE_CERTIFICATE_FILE_RESULT;
}

void expect_SSL_CTX_use_certificate_file(bool success, char* certfile)
{
    expect_any(__wrap_SSL_CTX_use_certificate_file, ctx);
    expect_string(__wrap_SSL_CTX_use_certificate_file, file, certfile);
    expect_value(__wrap_SSL_CTX_use_certificate_file, type, SSL_FILETYPE_PEM);
    if (success)
        SSL_USE_CERTIFICATE_FILE_RESULT = 1;
    else
        SSL_USE_CERTIFICATE_FILE_RESULT = 0;
}

int SSL_USE_PRIVATEKEY_FILE_RESULT = 0;
int __wrap_SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file, int type)
{
    check_expected_ptr(ctx);
    check_expected_ptr(file);
    check_expected(type);
    return SSL_USE_PRIVATEKEY_FILE_RESULT;
}

void expect_SSL_CTX_use_PrivateKey_file(bool success, char* privatekey)
{
    expect_any(__wrap_SSL_CTX_use_PrivateKey_file, ctx);
    expect_string(__wrap_SSL_CTX_use_PrivateKey_file, file, privatekey);
    expect_value(__wrap_SSL_CTX_use_PrivateKey_file, type, SSL_FILETYPE_PEM);
    if (success)
        SSL_USE_PRIVATEKEY_FILE_RESULT = 1;
    else
        SSL_USE_PRIVATEKEY_FILE_RESULT = 0;
}

int SSL_CTX_CHECK_PRIVATE_KEY_RESULT = 1;
int __wrap_SSL_CTX_check_private_key(const SSL_CTX* ctx)
{
    check_expected_ptr(ctx);
    return SSL_CTX_CHECK_PRIVATE_KEY_RESULT;
}

void expect_SSL_CTX_check_private_key(bool success)
{
    expect_any(__wrap_SSL_CTX_check_private_key, ctx);
    if (success)
        SSL_CTX_CHECK_PRIVATE_KEY_RESULT = 1;
    else
        SSL_CTX_CHECK_PRIVATE_KEY_RESULT = 0;
}

SSL* SSL_NEW_RESULT = NULL;
SSL* __wrap_SSL_new(SSL_CTX* ctx)
{
    check_expected_ptr(ctx);
    return SSL_NEW_RESULT;
}

int DUMMY_SSL_OBJECT;
void expect_SSL_new(bool success)
{
    expect_any(__wrap_SSL_new, ctx);
    if (success)
    {
        SSL_NEW_RESULT = (SSL*)&DUMMY_SSL_OBJECT;
    }
    else
    {
        SSL_NEW_RESULT = NULL;
        expect_ERR_error_string_n();
    }
}

int SSL_SET_FD_RESULT = 1;
int __wrap_SSL_set_fd(SSL* s, int fd)
{
    check_expected_ptr(s);
    check_expected(fd);
    return SSL_SET_FD_RESULT;
}

void expect_SSL_set_fd(bool success, int expected_fd)
{
    expect_any(__wrap_SSL_set_fd, s);
    expect_value(__wrap_SSL_set_fd, fd, expected_fd);
    if (success)
    {
        SSL_SET_FD_RESULT = 1;
    }
    else
    {
        SSL_SET_FD_RESULT = 0;
        expect_ERR_error_string_n();
    }
}

int SSL_ACCEPT_ERRNO = 0;
int SSL_ACCEPT_RESULT = 1;
int __wrap_SSL_accept(SSL* ssl)
{
    if (SSL_ACCEPT_ERRNO != 0)
        errno = SSL_ACCEPT_ERRNO;
    check_expected_ptr(ssl);
    return SSL_ACCEPT_RESULT;
}

int SSL_GET_ERROR_RESULT = 0;
int __wrap_SSL_get_error(const SSL* s, int ret_code)
{
    check_expected_ptr(s);
    check_expected(ret_code);
    return SSL_GET_ERROR_RESULT;
}

int SSL_READ_RESULT = 0;
char SSL_READ_DATA[256];
int SSL_READ_DATA_LEN = 0;
int __wrap_SSL_read(SSL* ssl, void* buf, int num)
{
    check_expected_ptr(ssl);
    check_expected_ptr(buf);
    check_expected(num);
    if (SSL_READ_RESULT > 0)
    {
        memcpy(buf, &SSL_READ_DATA, SSL_READ_DATA_LEN);
    }
    return SSL_READ_RESULT;
}

void expect_SSL_read(int result, int expected_size, int ssl_error,
                     char* expected_data, int len_expected_data)
{
    expect_any(__wrap_SSL_read, ssl);
    expect_any(__wrap_SSL_read, buf);
    expect_value(__wrap_SSL_read, num, expected_size);
    SSL_READ_RESULT = result;
    if (result <= 0)
    {
        expect_any(__wrap_SSL_get_error, s);
        expect_value(__wrap_SSL_get_error, ret_code, result);
        SSL_GET_ERROR_RESULT = ssl_error;
        if (ssl_error != SSL_ERROR_WANT_WRITE &&
            ssl_error != SSL_ERROR_WANT_READ)
            expect_ERR_error_string_n();
    }
    else
    {
        memcpy(&SSL_READ_DATA, expected_data, len_expected_data);
        SSL_READ_DATA_LEN = len_expected_data;
    }
}

int SSL_PENDING_RESULT = 0;
int __wrap_SSL_pending(const SSL* s)
{
    check_expected_ptr(s);
    return SSL_PENDING_RESULT;
}

int SSL_WRITE_RESULT = 0;
int __wrap_SSL_write(SSL* ssl, const void* buf, int num)
{
    check_expected_ptr(ssl);
    check_expected_ptr(buf);
    check_expected(num);
    return SSL_WRITE_RESULT;
}

void expect_SSL_write(int result, int expect_num_bytes, int ssl_error)
{
    expect_any(__wrap_SSL_write, ssl);
    expect_any(__wrap_SSL_write, buf);
    expect_value(__wrap_SSL_write, num, expect_num_bytes);
    SSL_WRITE_RESULT = result;
    if (result <= 0)
    {
        expect_any(__wrap_SSL_get_error, s);
        expect_value(__wrap_SSL_get_error, ret_code, result);
        SSL_GET_ERROR_RESULT = ssl_error;
        expect_ERR_error_string_n();
    }
}

void __wrap_SSL_free(SSL* ssl)
{
    check_expected_ptr(ssl);
}

STATUS __wrap_extnet_close_client(ExtNet* state, extnet_conn_t* pconn)
{
    check_expected_ptr(state);
    check_expected_ptr(pconn);
    return ST_OK;
}

typedef enum
{
    ACCEPT_FAIL_NONE = 0,
    ACCEPT_FAIL_TIMEOUT,
    ACCEPT_FAIL_NOT_SUCCESSFUL, // 1
    ACCEPT_FAIL_FATAL,          // -1
} acceptFailType;

int expect_SSL_accept(acceptFailType type)
{
    SSL_ACCEPT_ERRNO = 0;
    expect_any(__wrap_SSL_accept, ssl);
    if (type == ACCEPT_FAIL_NONE)
    {
        SSL_ACCEPT_RESULT = 1;
    }
    else
    {
        if (type == ACCEPT_FAIL_TIMEOUT)
        {
            SSL_ACCEPT_RESULT = -1;
            SSL_ACCEPT_ERRNO = EWOULDBLOCK;
        }
        else if (type == ACCEPT_FAIL_NOT_SUCCESSFUL)
        {
            SSL_ACCEPT_RESULT = 0;
            expect_ERR_error_string_n();
        }
        else if (type == ACCEPT_FAIL_FATAL)
        {
            SSL_ACCEPT_RESULT = -1;
            expect_ERR_error_string_n();
        }
        expect_any(__wrap_SSL_free, ssl);
        expect_any(__wrap_extnet_close_client, state);
        expect_any(__wrap_extnet_close_client, pconn);
    }
}

X509* SSL_GET_PEER_CERTIFICATE_RESULT = NULL;
X509* __wrap_SSL_get_peer_certificate(const SSL* s)
{
    check_expected_ptr(s);
    return SSL_GET_PEER_CERTIFICATE_RESULT;
}

int FAKE_X509;
void expect_SSL_get_peer_certificate(bool success)
{
    expect_any(__wrap_SSL_get_peer_certificate, s);
    if (success)
    {
        SSL_GET_PEER_CERTIFICATE_RESULT = (X509*)&FAKE_X509;

        expect_any(__wrap_X509_get_subject_name, a);
        expect_any(__wrap_X509_NAME_oneline, a);
        expect_any(__wrap_X509_NAME_oneline, buf);
        expect_any(__wrap_X509_NAME_oneline, size);

        expect_any(__wrap_X509_get_issuer_name, a);
        expect_any(__wrap_X509_NAME_oneline, a);
        expect_any(__wrap_X509_NAME_oneline, buf);
        expect_any(__wrap_X509_NAME_oneline, size);

        expect_any(__wrap_X509_free, a);
    }
    else
    {
        SSL_GET_PEER_CERTIFICATE_RESULT = NULL;
    }
}

const SSL_METHOD* __wrap_TLS_server_method(void)
{
    return NULL;
}

X509_NAME* __wrap_X509_get_subject_name(const X509* a)
{
    check_expected_ptr(a);
    return NULL;
}

char* X509_NAME_ONELINE_RESULT = "blahblahblah";
char* __wrap_X509_NAME_oneline(const X509_NAME* a, char* buf, int size)
{
    check_expected_ptr(a);
    check_expected_ptr(buf);
    check_expected(size);
    return X509_NAME_ONELINE_RESULT;
}

X509_NAME* __wrap_X509_get_issuer_name(const X509* a)
{
    check_expected_ptr(a);
    return NULL;
}

void __wrap_X509_free(X509* a)
{
    check_expected_ptr(a);
}

void expect_cleanup()
{
    expect_any(__wrap_SSL_CTX_free, ctx);
}

int SETSOCKOPT_RESULT = 0;
int __wrap_setsockopt(int sockfd, int level, int optname, const void* optval,
                      socklen_t optlen)
{
    check_expected(sockfd);
    check_expected(level);
    check_expected(optname);
    check_expected_ptr(optval);
    check_expected(optlen);
    return SETSOCKOPT_RESULT;
}

void expect_setsockopt(bool success, int expected_fd)
{
    expect_value(__wrap_setsockopt, sockfd, expected_fd);
    expect_value(__wrap_setsockopt, level, SOL_SOCKET);
    expect_value(__wrap_setsockopt, optname, SO_RCVTIMEO);
    expect_any(__wrap_setsockopt, optval);
    expect_any(__wrap_setsockopt, optlen);
    if (success)
        SETSOCKOPT_RESULT = 0;
    else
        SETSOCKOPT_RESULT = 1;
}

void expect_exttls_init(char* certfile)
{
    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(true);
    expect_SSL_CTX_set_options();
    expect_SSL_CTX_set_cipher_list(true);
    expect_SSL_CTX_use_certificate_file(true, certfile);
    expect_SSL_CTX_use_PrivateKey_file(true, certfile);
    expect_SSL_CTX_check_private_key(true);
}

void exttls_init_invalid_params_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, exttls_init(NULL));
}

void exttls_init_SSL_ctx_new_failed_test(void** state)
{
    (void)state;

    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(false);
    expect_ERR_error_string_n();
    assert_int_equal(ST_ERR, exttls_init("some/file.pem"));
}

void exttls_init_no_valid_cipher_found_test(void** state)
{
    (void)state;

    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(true);
    expect_SSL_CTX_set_options();

    expect_SSL_CTX_set_cipher_list(false);
    expect_ERR_error_string_n();
    expect_cleanup();
    assert_int_equal(ST_ERR, exttls_init("some/file.pem"));
}

void exttls_init_SSL_CTX_use_certificate_file_failure_test(void** state)
{
    (void)state;

    char* certfile = "some/cert_file.pem";
    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(true);
    expect_SSL_CTX_set_options();
    expect_SSL_CTX_set_cipher_list(true);
    expect_SSL_CTX_use_certificate_file(false, certfile);
    expect_ERR_error_string_n();
    expect_cleanup();
    assert_int_equal(ST_ERR, exttls_init(certfile));
}

void exttls_init_SSL_CTX_use_PrivateKey_file_failure_test(void** state)
{
    (void)state;

    char* certfile = "some/cert_file.pem";
    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(true);
    expect_SSL_CTX_set_options();
    expect_SSL_CTX_set_cipher_list(true);
    expect_SSL_CTX_use_certificate_file(true, certfile);
    expect_SSL_CTX_use_PrivateKey_file(false, certfile);
    expect_ERR_error_string_n();
    expect_cleanup();
    assert_int_equal(ST_ERR, exttls_init(certfile));
}

void exttls_init_SSL_CTX_check_private_key_failure_test(void** state)
{
    (void)state;

    char* certfile = "some/cert_file.pem";
    expect_SSL_load_error_strings();
    expect_OpenSSL_add_ssl_algorithms();
    expect_SSL_CTX_new(true);
    expect_SSL_CTX_set_options();
    expect_SSL_CTX_set_cipher_list(true);
    expect_SSL_CTX_use_certificate_file(true, certfile);
    expect_SSL_CTX_use_PrivateKey_file(true, certfile);
    expect_SSL_CTX_check_private_key(false);
    expect_ERR_error_string_n();
    expect_cleanup();
    assert_int_equal(ST_ERR, exttls_init(certfile));
}

void exttls_init_success_test(void** state)
{
    (void)state;
    char* certfile = "some/cert_file.pem";
    expect_exttls_init(certfile);
    assert_int_equal(ST_OK, exttls_init(certfile));
}

void exttls_cleanup_success_test(void** state)
{
    (void)state;
    char* certfile = "some/cert_file.pem";
    expect_exttls_init(certfile);
    assert_int_equal(ST_OK, exttls_init(certfile));

    expect_cleanup();
    exttls_cleanup();
}

void exttls_on_accept_invalid_params_test(void** state)
{
    (void)state;
    int dummy_net_state;
    extnet_conn_t conn;
    assert_int_equal(ST_ERR, exttls_on_accept(NULL, &conn));
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, NULL));
    assert_int_equal(ST_ERR, exttls_on_accept(NULL, NULL));
}

void exttls_on_accept_invalid_socket_test(void** state)
{
    (void)state;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = -1;
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_ssl_new_failure_test(void** state)
{
    (void)state;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = 1;
    expect_SSL_new(false);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_SSL_set_fd_failure_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(false, expected_fd);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_setsockopt_failure_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(false, expected_fd);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_SSL_accept_fatal_failure_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(true, expected_fd);
    expect_SSL_accept(ACCEPT_FAIL_FATAL);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_SSL_accept_not_successful_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(true, expected_fd);
    expect_SSL_accept(ACCEPT_FAIL_NOT_SUCCESSFUL);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_SSL_accept_timeout_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(true, expected_fd);
    expect_SSL_accept(ACCEPT_FAIL_TIMEOUT);
    assert_int_equal(ST_ERR, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_SSL_get_peer_certificate_failure_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(true, expected_fd);
    expect_SSL_accept(ACCEPT_FAIL_NONE);
    // a client certificate is not required
    expect_SSL_get_peer_certificate(false);
    assert_int_equal(ST_OK, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_on_accept_success_test(void** state)
{
    (void)state;
    int expected_fd = 77;
    int dummy_net_state;
    extnet_conn_t conn;
    conn.sockfd = expected_fd;
    expect_SSL_new(true);
    expect_SSL_set_fd(true, expected_fd);
    expect_setsockopt(true, expected_fd);
    expect_SSL_accept(ACCEPT_FAIL_NONE);
    expect_SSL_get_peer_certificate(true);
    assert_int_equal(ST_OK, exttls_on_accept(&dummy_net_state, &conn));
}

void exttls_init_client_invalid_params_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, exttls_init_client(NULL));
}

void exttls_init_client_success_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    assert_int_equal(ST_OK, exttls_init_client(&conn));
}

void exttls_on_close_client_invalid_params_test(void** state)
{
    (void)state;
    assert_int_equal(ST_ERR, exttls_on_close_client(NULL));
}

void exttls_on_close_client_success_test(void** state)
{
    (void)state;
    int dummy_data = 9;
    extnet_conn_t conn;
    conn.p_hdlr_data = &dummy_data;
    expect_any(__wrap_SSL_free, ssl);
    assert_int_equal(ST_OK, exttls_on_close_client(&conn));
    assert_null(conn.p_hdlr_data);
}

void exttls_recv_invalid_params_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    size_t sz_len = 5;
    void* buf = malloc(sz_len);
    bool pending = false;
    // invalid pointer parameters
    assert_int_equal(-1, exttls_recv(NULL, buf, sz_len, &pending));
    assert_int_equal(-1, exttls_recv(&conn, NULL, sz_len, &pending));
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, NULL));
    conn.sockfd = -1;
    // invalid socket file descriptor
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    conn.sockfd = 1;
    conn.p_hdlr_data = NULL;
    // invalid ssl ptr
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    free(buf);
}

void exttls_recv_errors_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    size_t sz_len = 5;
    int fake_ssl = 8;
    void* buf = malloc(sz_len);
    bool pending = false;
    conn.sockfd = 1;
    conn.p_hdlr_data = &fake_ssl;
    expect_SSL_read(0, sz_len, SSL_ERROR_NONE, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(0, sz_len, SSL_ERROR_ZERO_RETURN, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(-1, sz_len, SSL_ERROR_WANT_WRITE, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(-1, sz_len, SSL_ERROR_WANT_READ, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(-1, sz_len, SSL_ERROR_SYSCALL, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(-1, sz_len, SSL_ERROR_SSL, NULL, 0);
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    expect_SSL_read(-1, sz_len, 999, NULL, 0); // some random other error
    assert_int_equal(-1, exttls_recv(&conn, buf, sz_len, &pending));
    free(buf);
}

void exttls_recv_success_test(void** state)
{
    (void)state;
    char* expect_data = "blah";
    extnet_conn_t conn;
    size_t sz_len = 5;
    int fake_ssl = 8;
    void* buf = malloc(sz_len);
    bool pending = false;
    conn.sockfd = 1;
    conn.p_hdlr_data = &fake_ssl;

    // test pending false
    expect_SSL_read(sz_len, (int)sz_len, 0, expect_data,
                    (int)strlen(expect_data));
    expect_any(__wrap_SSL_pending, s);
    SSL_PENDING_RESULT = 0;
    assert_int_equal(sz_len, exttls_recv(&conn, buf, sz_len, &pending));
    assert_false(pending);

    // test pending true
    expect_SSL_read(sz_len, (int)sz_len, 0, expect_data,
                    (int)strlen(expect_data));
    expect_any(__wrap_SSL_pending, s);
    SSL_PENDING_RESULT = 2;
    assert_int_equal(sz_len, exttls_recv(&conn, buf, sz_len, &pending));
    assert_true(pending);

    free(buf);
}

void exttls_send_invalid_params_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    size_t sz_len = 5;
    void* buf = malloc(sz_len);
    // invalid pointer parameters
    assert_int_equal(-1, exttls_send(NULL, buf, sz_len));
    assert_int_equal(-1, exttls_send(&conn, NULL, sz_len));
    conn.sockfd = -1;
    // invalid socket file descriptor
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    conn.sockfd = 1;
    conn.p_hdlr_data = NULL;
    // invalid ssl ptr
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    free(buf);
}

void exttls_send_errors_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    size_t sz_len = 5;
    int fake_ssl = 8;
    void* buf = malloc(sz_len);
    conn.sockfd = 1;
    conn.p_hdlr_data = &fake_ssl;

    expect_SSL_write(0, sz_len, SSL_ERROR_NONE);
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    expect_SSL_write(0, sz_len, SSL_ERROR_ZERO_RETURN);
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    expect_SSL_write(-1, sz_len, SSL_ERROR_SYSCALL);
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    expect_SSL_write(-1, sz_len, SSL_ERROR_SSL);
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));
    // any other error
    expect_any(__wrap_ERR_print_errors_fp, fp);
    expect_SSL_write(-1, sz_len, 999);
    assert_int_equal(-1, exttls_send(&conn, buf, sz_len));

    free(buf);
}

void exttls_send_success_test(void** state)
{
    (void)state;
    extnet_conn_t conn;
    size_t sz_len = 5;
    int fake_ssl = 8;
    void* buf = malloc(sz_len);
    conn.sockfd = 1;
    conn.p_hdlr_data = &fake_ssl;

    expect_SSL_write(sz_len, sz_len, 0);
    assert_int_equal(sz_len, exttls_send(&conn, buf, sz_len));

    free(buf);
}

int main()
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(exttls_init_invalid_params_test),
        cmocka_unit_test(exttls_init_SSL_ctx_new_failed_test),
        cmocka_unit_test(exttls_init_no_valid_cipher_found_test),
        cmocka_unit_test(exttls_init_SSL_CTX_use_certificate_file_failure_test),
        cmocka_unit_test(exttls_init_SSL_CTX_use_PrivateKey_file_failure_test),
        cmocka_unit_test(exttls_init_SSL_CTX_check_private_key_failure_test),
        cmocka_unit_test(exttls_init_success_test),
        cmocka_unit_test(exttls_cleanup_success_test),
        cmocka_unit_test(exttls_on_accept_invalid_params_test),
        cmocka_unit_test(exttls_on_accept_invalid_socket_test),
        cmocka_unit_test(exttls_on_accept_ssl_new_failure_test),
        cmocka_unit_test(exttls_on_accept_SSL_set_fd_failure_test),
        cmocka_unit_test(exttls_on_accept_setsockopt_failure_test),
        cmocka_unit_test(exttls_on_accept_SSL_accept_fatal_failure_test),
        cmocka_unit_test(exttls_on_accept_SSL_accept_not_successful_test),
        cmocka_unit_test(exttls_on_accept_SSL_accept_timeout_test),
        cmocka_unit_test(
            exttls_on_accept_SSL_get_peer_certificate_failure_test),
        cmocka_unit_test(exttls_on_accept_success_test),
        cmocka_unit_test(exttls_init_client_invalid_params_test),
        cmocka_unit_test(exttls_init_client_success_test),
        cmocka_unit_test(exttls_on_close_client_invalid_params_test),
        cmocka_unit_test(exttls_on_close_client_success_test),
        cmocka_unit_test(exttls_recv_invalid_params_test),
        cmocka_unit_test(exttls_recv_errors_test),
        cmocka_unit_test(exttls_recv_success_test),
        cmocka_unit_test(exttls_send_invalid_params_test),
        cmocka_unit_test(exttls_send_errors_test),
        cmocka_unit_test(exttls_send_success_test),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
