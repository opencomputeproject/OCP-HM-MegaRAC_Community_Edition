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

/** @file ext_tls.c
 * This file contains support for encrypted TLS external client connections.
 */

#include "ext_tls.h"

#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "asd_common.h"
#include "ext_network.h"
#include "logging.h"

static struct
{
    SSL_CTX* ssl_ctx;
} sg_data;

extnet_hdlrs_t tls_hdlrs = {
    exttls_init, exttls_on_accept, exttls_on_close_client, exttls_init_client,
    exttls_recv, exttls_send,      exttls_cleanup,
};

static SSL_CTX* init_ssl_context(const char* cp_certfile,
                                 const char* cp_keyfile);
void cleanup(SSL_CTX* ctx);

/** @brief Initialize OpenSSL
 *
 *  Called to initialize External Network Interface
 */
STATUS exttls_init(void* p_hdlr_data)
{
    STATUS st_ret = ST_ERR;
    char* cp_certkeyfile = (char*)p_hdlr_data;

    if (cp_certkeyfile)
    {
        // initialize OpenSSL libraries
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        // Initialize our SSL context;
        sg_data.ssl_ctx = init_ssl_context(cp_certkeyfile, cp_certkeyfile);
        if (sg_data.ssl_ctx)
            st_ret = ST_OK;
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Cannot initialize TLS without cert/key file!");
        sg_data.ssl_ctx = NULL;
    }
    return st_ret;
}

/** @brief Cleans up SSL context and connections.
 *
 *  Called to cleanup connection.
 */
void exttls_cleanup(void)
{
    if (sg_data.ssl_ctx)
    {
        cleanup(sg_data.ssl_ctx);
    }
    sg_data.ssl_ctx = NULL;
}

void cleanup(SSL_CTX* ctx)
{
    if (ctx)
    {
        SSL_CTX_free(ctx);
        EVP_cleanup();
    }
}

/** @brief Initializes SSL context
 *
 *  Called once to initialize SSL at the beginning of the program.
 *
 *  @param [in] cp_certfile filename containing the SSL certificate in PEM
 * format.
 *
 *  @param [in] cp_keyfile filename containing the SSL private key in PEM
 * format. If the certificate and key are in the same file, cp_certfile and
 *         cp_keyfile may be set to the same file.
 *
 *  @return The SSL_CTX pointer
 */
static SSL_CTX* init_ssl_context(const char* cp_certfile,
                                 const char* cp_keyfile)
{
    const SSL_METHOD* method = SSLv23_server_method();
    const long flags = SSL_OP_NO_SSLv3 | SSL_OP_NO_SSLv2 | SSL_OP_NO_TLSv1 |
                       SSL_OP_NO_TLSv1_1 | SSL_OP_NO_COMPRESSION |
                       SSL_OP_CIPHER_SERVER_PREFERENCE;

    const char* cipher_list = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-"
                              "GCM-SHA384:!aNULL:!eNULL@STRENGTH";
    char ca_errstr[256];

    SSL_CTX* ctx = SSL_CTX_new(method); // create the context
    if (!ctx)
    {
        ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Error creating SSL context: %s", ca_errstr);
    }
    else
    {
        SSL_CTX_set_options(ctx, flags);
        // Set list of ciphers.
        if (SSL_CTX_set_cipher_list(ctx, cipher_list) != 1)
        {
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "No valid ciphers found!: %s",
                    ca_errstr);
            cleanup(ctx);
            ctx = NULL;
        }
        else
        {
            /* configure ECDH to allow ECDH/ECDHE ciphers */
            SSL_CTX_set1_curves_list(ctx, "P-384");

            /* configure certificate */
            if (SSL_CTX_use_certificate_file(ctx, cp_certfile,
                                             SSL_FILETYPE_PEM) != 1)
            {
                ERR_error_string_n(ERR_get_error(), ca_errstr,
                                   sizeof(ca_errstr));
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "Error with certificate file '%s': %s", cp_certfile,
                        ca_errstr);
                cleanup(ctx);
                ctx = NULL;
            }

            /* configure private key */
            else if (SSL_CTX_use_PrivateKey_file(ctx, cp_keyfile,
                                                 SSL_FILETYPE_PEM) != 1)
            {
                ERR_error_string_n(ERR_get_error(), ca_errstr,
                                   sizeof(ca_errstr));
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "Error with private key file '%s': %s", cp_keyfile,
                        ca_errstr);
                cleanup(ctx);
                ctx = NULL;
            }
            else if (SSL_CTX_check_private_key(ctx) != 1)
            {
                ERR_error_string_n(ERR_get_error(), ca_errstr,
                                   sizeof(ca_errstr));
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "Error with private key file '%s': %s\n"
                        "Private key does not match the public certificate",
                        cp_keyfile, ca_errstr);
                cleanup(ctx);
                ctx = NULL;
            }
        }
    }

    return ctx;
}

/** @brief Accepts the socket connection and handles client key
 *
 *  Called each time a new connection is accepted on the listening SSL socket.
 *
 *  @param [in] ext_listen_sockfd File descriptor for listening socket.
 *  @param [in,out] pconn Pointer to the connection structure.
 *  @return ST_OK if successful.
 */
STATUS exttls_on_accept(void* net_state, extnet_conn_t* pconn)
{
    STATUS st_ret = ST_OK;
    struct timeval timeout;
    char ca_errstr[256];

    if (!net_state || !pconn)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
        st_ret = ST_ERR;
    }
    else if (pconn->sockfd < 0)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid socket fd: %d", __FUNCTION__,
                pconn->sockfd);
        st_ret = ST_ERR;
    }
    else
    {
        pconn->p_hdlr_data = SSL_new(sg_data.ssl_ctx);
        if (!pconn->p_hdlr_data)
        {
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "%s errno %d: %s", __FUNCTION__, errno,
                    ca_errstr);
            st_ret = ST_ERR;
        }
    }

    if (st_ret == ST_OK)
    {
        if (SSL_set_fd((SSL*)pconn->p_hdlr_data, pconn->sockfd) != 1)
        {
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "SSL_accept() errno %d: %s", errno,
                    ca_errstr);
            st_ret = ST_ERR;
        }
    }
    if (st_ret == ST_OK)
    {
        /* Set timeout on the SSL_Connect to prevent connecting
         * without negotiating the SSL and hanging the listener */
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        if (setsockopt(pconn->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout)) != 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "setsockopt(SO_RCVTIMEO) failed errno: %d", errno);
            st_ret = ST_ERR;
        }
        // Negotiate the SSL connection
        else if (SSL_accept((SSL*)pconn->p_hdlr_data) != 1)
        {
            if (EWOULDBLOCK == errno)
            {
                snprintf(ca_errstr, sizeof(ca_errstr),
                         "Timeout waiting for the connecting client to "
                         "initiate the SSL handshake!");
            }
            else
            {
                ERR_error_string_n(ERR_get_error(), ca_errstr,
                                   sizeof(ca_errstr));
            }
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "SSL_accept() errno %d: %s", errno,
                    ca_errstr);
            SSL_free((SSL*)pconn->p_hdlr_data);
            pconn->p_hdlr_data = NULL;
            extnet_close_client(net_state, pconn);
            st_ret = ST_ERR;
        }
        else
        {
            // Valid/Successful SSL new client connection,
            // get client cert for logging purposes only
            X509* cert = SSL_get_peer_certificate((SSL*)pconn->p_hdlr_data);
            if (cert == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                        ASD_LogOption_None, "No client certificate\n");
            }
            else
            {
                char data[256];

                X509_NAME_oneline(X509_get_subject_name(cert), data,
                                  sizeof(data));
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                        ASD_LogOption_None, "Client certificate subject: %s\n",
                        data);
#endif
                X509_NAME_oneline(X509_get_issuer_name(cert), data,
                                  sizeof(data));
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                        ASD_LogOption_None, "Client certificate issuer: %s\n",
                        data);
#endif
                X509_free(cert);
            }
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                    ASD_LogOption_None, "Accepted client fd %d", pconn->sockfd);
#endif
        }
    }

    return st_ret;
}

/** @brief Initialize client connection
 *
 *  Called to initialize external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful, RET_SSL_ERR otherwise.
 */
STATUS exttls_init_client(extnet_conn_t* pconn)
{
    STATUS result = ST_ERR;
    if (!pconn)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else
    {
        pconn->p_hdlr_data = NULL;
        result = ST_OK;
    }
    return result;
}

/** @brief Close client connection and free pointer.
 *
 *  Called to close external client connection
 *
 *  @param [in,out] pconn Pointer to the connection pointer.
 *  @return RET_OK if successful, RET_SSL_ERR otherwise.
 */
STATUS exttls_on_close_client(extnet_conn_t* pconn)
{
    STATUS result = ST_ERR;
    if (!pconn)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else
    {
        if (pconn->p_hdlr_data)
        {
            SSL_free((SSL*)pconn->p_hdlr_data);
            pconn->p_hdlr_data = NULL;
        }
        result = ST_OK;
    }
    return result;
}

/** @brief Validate SSL read return code for close operation.
 *
 *  Called after SSL_read() is called.
 *
 *  @param [in] ssl SSL pointer returned from SSL_new().
 *  @return ST_OK if successful, ST_ERR otherwise.
 */
static STATUS exttls_has_read_error_closed(SSL* ssl, int n_errcode)
{
    int ssl_err = SSL_get_error(ssl, n_errcode);
    char ca_errstr[256] = {0};

    if ((ssl_err == SSL_ERROR_NONE) || (ssl_err == SSL_ERROR_ZERO_RETURN))
    {
        ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Connection was closed: %s", ca_errstr);
#endif
        return ST_OK;
    }
    else
    {
        return ST_ERR;
    }
}

/** @brief Validate SSL read return code.
 *
 *  Called after SSL_read() is called.
 *
 *  @param [in] ssl SSL pointer returned from SSL_new().
 *  @return RET_OK if successful, RET_SSL_ERR otherwise.
 */
static STATUS exttls_has_read_error(SSL* ssl, int n_errcode)
{
    int n_oerrno = errno;
    int ssl_err = SSL_get_error(ssl, n_errcode);
    STATUS st_ret = ST_OK;
    char ca_errstr[256] = {0};

    switch (ssl_err)
    {
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Read BLOCK");
            break;
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "SSL_read() returns %d; error %d; errno %d: %s", n_errcode,
                    ssl_err, n_oerrno, ca_errstr);
            st_ret = ST_ERR;
            break;
        default:
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "SSL_read() returns %d; error %d; errno %d: %s", n_errcode,
                    ssl_err, n_oerrno, ca_errstr);
            break;
    }
    return st_ret;
}

/** @brief Validate SSL write return code.
 *
 *  Called after SSL_write() is called.
 *
 *  @param [in] ssl SSL pointer returned from SSL_new().
 *  @return RET_OK if successful, RET_SSL_ERR otherwise.
 */
STATUS exttls_has_write_error(SSL* ssl, int n_errcode)
{
    int n_oerrno = errno;
    int ssl_err = SSL_get_error(ssl, n_errcode);
    STATUS st_ret = ST_OK;
    char ca_errstr[256];

    switch (ssl_err)
    {
        case SSL_ERROR_NONE:
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
            st_ret = ST_ERR;
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "SSL error %d on write! %s", n_errcode,
                    ca_errstr);
            break;
        default:
            ERR_error_string_n(ERR_get_error(), ca_errstr, sizeof(ca_errstr));
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None,
                    "SSL_write() returns %d; error %d; errno %d: %s", n_errcode,
                    ssl_err, n_oerrno, ca_errstr);
            ERR_print_errors_fp(stderr);
            break;
    }
    return st_ret;
}

/** @brief Read data from external network connection
 *
 *  Called each time data is available on the external socket.
 *
 *  @param [in] pconn Connetion pointer
 *  @param [out] pv_buf Buffer where data will be stored.
 *  @param [in] sz_len sizeof pv_buf
 *  @param [out] b_data_pending Indicates more data is available.
 *  @return number of bytes received.
 */
int exttls_recv(extnet_conn_t* pconn, void* pv_buf, size_t sz_len,
                bool* b_data_pending)
{
    int n_read = -1;

    if (!pconn || !pv_buf || !b_data_pending)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else if (pconn->sockfd < 0)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid file descriptor %d", __FUNCTION__,
                pconn->sockfd);
    }
    else if (!pconn->p_hdlr_data)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid SSL pointer", __FUNCTION__);
    }
    else
    {
        *b_data_pending = false;
        if ((n_read =
                 SSL_read((SSL*)pconn->p_hdlr_data, pv_buf, (int)sz_len)) <= 0)
        {
            if (ST_OK !=
                exttls_has_read_error_closed((SSL*)pconn->p_hdlr_data, n_read))
            {
                n_read = 0;
            }
            else if (ST_OK !=
                     exttls_has_read_error((SSL*)pconn->p_hdlr_data, n_read))
            {
                n_read = -1;
            }
        }
        else if (SSL_pending((SSL*)pconn->p_hdlr_data) > 0)
        {
            *b_data_pending = true;
        }
    }
    return n_read;
}

/** @brief Write data to external network connection
 *
 *  Called each time data is available on the external socket.
 *
 *  @param [in] pconn Connetion pointer
 *  @param [out] pv_buf Buffer where data will be stored.
 *  @param [in] sz_len sizeof pv_buf
 *  @return number of bytes received.
 */
int exttls_send(extnet_conn_t* pconn, void* pv_buf, size_t sz_len)
{
    int n_wr = -1;

    if (!pconn || !pv_buf)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid pointer", __FUNCTION__);
    }
    else if (pconn->sockfd < 0)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid file descriptor %d", __FUNCTION__,
                pconn->sockfd);
    }
    else if (!pconn->p_hdlr_data)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "%s called with invalid SSL pointer", __FUNCTION__);
    }
    else if ((n_wr = SSL_write((SSL*)pconn->p_hdlr_data, pv_buf,
                               (int)sz_len)) <= 0)
    {
        if (ST_OK != exttls_has_write_error((SSL*)pconn->p_hdlr_data, n_wr))
        {
            n_wr = -1;
        }
    }
    return n_wr;
}
