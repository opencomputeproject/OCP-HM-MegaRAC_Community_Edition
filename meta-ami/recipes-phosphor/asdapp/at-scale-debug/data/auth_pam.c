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

/**
 * @file auth_pam.c
 * @brief Functions supporting authentication of credentials from client,
 * tracking authentication attempts and locking out authentication when a
 * threshold is exceeded.
 */
#include "auth_pam.h"

#include <errno.h>
#include <openssl/rand.h>
#include <safe_str_lib.h>
#include <security/pam_appl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "asd_common.h"
#include "authenticate.h"
#include "ext_network.h"
#include "logging.h"
#include "session.h"

typedef enum
{
    AUTHRET_OK,
    AUTHRET_SYSERR,
    AUTHRET_INVALIDDATA,
    AUTHRET_UNAUTHORIZED,
    AUTHRET_LOCKOUT,
} auth_ret_t;

STATUS authpam_init(void* p_hdlr_data);
STATUS authpam_client_handshake(Session* session, ExtNet* net_state,
                                extnet_conn_t* p_extconn);

auth_hdlrs_t authpam_hdlrs = {authpam_init, authpam_client_handshake};

/** @brief Initialize PAM authentication handler
 *  @param [in] p_hdlr_data Pointer to handler specific data (not used)
 */
STATUS authpam_init(void* p_hdlr_data)
{
    (void)p_hdlr_data;
    return ST_OK;
}

// function used to get user input
int pam_conversation_function(int numMsg, const struct pam_message** msg,
                              struct pam_response** resp, void* appdata_ptr)
{
    int result = PAM_AUTH_ERR;
    if (appdata_ptr && msg && resp)
    {
        *resp = NULL;
        size_t len = strlen((char*)(appdata_ptr)) + 1;
        char* pass = (char*)malloc(len);
        if (pass)
        {
            explicit_bzero(pass, len);
            if (strcpy_s(pass, len, (char*)appdata_ptr))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                        ASD_LogOption_None,
                        "strcpy_safe: appdata_ptr to pass str \
						copy failed.");
                result = ST_ERR;
            }
            *resp = (struct pam_response*)calloc((size_t)numMsg,
                                                 sizeof(struct pam_response));

            if (*resp)
            {
                for (int i = 0; i < numMsg; ++i)
                {
                    /* Ignore all PAM messages except
                     * prompting for hidden input */
                    if (msg[i]->msg_style != PAM_PROMPT_ECHO_OFF)
                    {
                        continue;
                    }

                    /* Assume PAM is only prompting for
                     * the password as hidden input */
                    resp[i]->resp = pass;
                    result = PAM_SUCCESS;
                }
            }
        }
        if (result != PAM_SUCCESS)
        {
            if (resp)
            {
                free(*resp);
                *resp = NULL;
            }
            if (pass)
            {
                explicit_bzero(pass, len);
                free(pass);
            }
        }
    }

    return result;
}

/** @brief Validate the version and credentials of the client
 *
 *  Checks client credentials to determine if authenticated. The user name is
 *  fixed (not passed from client). Only the special debug user is
 * authenticated.
 *
 *  @param [in] cp_password Buffer containing password
 *  @param [in] n_pwlen length of password
 *
 *  @return Returns AUTHRET_SYSERR if unable to configure PAM;
 *                  AUTHRET_OK if authenticated;
 *                  AUTHRET_UNAUTHORIZED if password is invalid
 */
static auth_ret_t credentials_are_valid(unsigned char* cp_password,
                                        unsigned int n_pwlen)
{
    char* cp_username = ASD_PAM_USER;
    char* pam_svc = ASD_PAM_SERVICE;
    pam_handle_t* pamh = NULL;
    const struct pam_conv pamc = {pam_conversation_function, cp_password};
    int pamerr;
    auth_ret_t ret = AUTHRET_UNAUTHORIZED;

    pamerr = pam_start(pam_svc, cp_username, &pamc, &pamh);
    if (PAM_SUCCESS != pamerr)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Unable to use PAM service '%s'! Error %d", pam_svc, pamerr);
        ret = AUTHRET_SYSERR;
    }
    else
    {
        pamerr = pam_authenticate(pamh, PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);
        if (PAM_SUCCESS != pamerr)
        {
            ASD_log(
                ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "pam_authenticate(PAM_DISALLOW_NULL_AUTHTOK) error %d", pamerr);
        }
        else
        {
            ret = AUTHRET_OK;
        }
    }
    // remove all traces of password
    explicit_bzero(cp_password, n_pwlen);

    if (pamh)
    {
        if (PAM_SUCCESS != (pamerr = pam_end(pamh, pamerr)))
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "pam_end(%d) error %d", pamerr);
        }
    }
    return ret;
}

/** @brief Record an invalid auth attempt and determine auth lockout.
 *
 *  Tracks unsuccessful auth attempts and indicates if a lockout should be
 *  enforced. if the number of invalid attempts is greater than
 *  INVALID_AUTH_MAX_ATTEMPS within INVALID_AUTH_PERIOD_NSECS seconds then
 *  auth is locked out.
 *
 *  @return Returns AUTHRET_LOCKOUT if invalid auth attempts are too frequent.
 */
static auth_ret_t auth_track_attempt(auth_ret_t err_code)
{
    static time_t ats_attempts[INVALID_AUTH_MAX_ATTEMPTS] = {0};
    time_t t_now = time(0L);
    int n_invalid_in_period = 1;
    int n_oldest_index = 0;

    if (AUTHRET_OK == err_code)
    {
        // Valid authentication, clear out attempts.
        explicit_bzero(ats_attempts, sizeof(ats_attempts));
    }
    else if (AUTHRET_UNAUTHORIZED == err_code ||
             AUTHRET_INVALIDDATA == err_code)
    {
        int i;
        for (i = 0; i < INVALID_AUTH_MAX_ATTEMPTS; i++)
        {
            // Find the oldest attempt.
            if (ats_attempts[i] < ats_attempts[n_oldest_index])
            {
                n_oldest_index = i;
            }
            // Increment invalid login count for each attempt within
            // the window
            if (ats_attempts[i] > (t_now - INVALID_AUTH_PERIOD_NSECS))
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "Invalid auth attempt [%d] %ld > %ld", i,
                        ats_attempts[i], (t_now - INVALID_AUTH_PERIOD_NSECS));
#endif
                n_invalid_in_period++;
            }
        }
        // Record this invalid attempt replacing the oldest.
        ats_attempts[n_oldest_index] = t_now;
        if (n_invalid_in_period > INVALID_AUTH_MAX_ATTEMPTS)
        {
            err_code = AUTHRET_LOCKOUT;
        }
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_None,
                "Invalid auth attempt #%d/%d [%d] %ld err %d",
                n_invalid_in_period, INVALID_AUTH_MAX_ATTEMPTS, n_oldest_index,
                t_now, err_code);
#endif
    }
    return err_code;
}

/** @brief Validate the client header including version and password.
 *
 *  @param [in] cp_buf Buffer containing client header
 *  @param [in] n_buflen length of buffer read
 *
 *  @return Returns AUTHRET_SYSERR if unable to configure PAM;
 *                  AUTHRET_OK if authenticated;
 *                  AUTHRET_UNAUTHORIZED if password is invalid
 *                  AUTHRET_INVALIDDATA if header version is invalid
 *                  AUTHAUTH_LOCKOUT if too many invalid auth attempts have
 *                  been made
 */
static auth_ret_t authenticate_client(char* cp_buf, int n_num_read)
{
    auth_ret_t ret;
    auth_handshake_req_t* phdr = (auth_handshake_req_t*)cp_buf;

    if (n_num_read > sizeof(auth_handshake_req_t) ||
        phdr->auth_hdr_version > AUTH_HDR_VERSION)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Invalid auth header version 0x%02x received len: %d",
                phdr->auth_hdr_version, n_num_read);
        ret = AUTHRET_INVALIDDATA;
    }
    else
    {
        char* cp;
        // Testing with openssl s_client adding newline, strip it.
        cp = memchr(cp_buf, '\n', (size_t)n_num_read);
        if (cp)
        {
            *cp = '\0';
        }

        // Validate the password.
        ret = credentials_are_valid(
            phdr->auth_password, n_num_read - sizeof(phdr->auth_hdr_version));
        if (AUTHRET_OK != ret)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network,
                    ASD_LogOption_None, "Unauthenticated connection attempt");
        }
    }
    return auth_track_attempt(ret);
}

/** @brief Send handshake response back to the client
 *
 *  Called after client authentication to report result to client.
 *
 *  @param [in] p_extconn Connection handle pointer
 *  @return ST_OK if successful,
 */
static STATUS auth_handshake_response(ExtNet* net_state,
                                      extnet_conn_t* p_extconn,
                                      auth_handshake_ret_t c_resp)
{
    STATUS st_ret = ST_OK;
    auth_handshake_resp_t resp;
    int n_wr;

    explicit_bzero(&resp, sizeof(resp));
    resp.svr_hdr_version = AUTH_HDR_VERSION;
    resp.result_code = c_resp;

    if (sizeof(resp) !=
        (n_wr = extnet_send(net_state, p_extconn, &resp, sizeof(resp))))
    {
        st_ret = ST_ERR;
    }
    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
            "wrote %d bytes response 0x%x", n_wr, c_resp);
    return st_ret;
}

/** @brief Get random printable string
 *  @param [in] cp_buf Buffer for storage of random password.
 *  @param [in] n_sz Sizeof cp_buf.
 *  @return ST_OK if successful
 */
static STATUS get_random_string(unsigned char* cp_buf, int n_sz)
{
    STATUS st_ret = ST_ERR;
    char ca_ascii[75] = RANDOM_ASCII_CHARACTERS;
    size_t n_len = strnlen(ca_ascii, sizeof(ca_ascii));

    if (1 != RAND_bytes(cp_buf, n_sz))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "RAND_bytes error %d", errno);
    }
    else
    {
        for (int i = 0; i < n_sz; i++)
        {
            size_t n_index = cp_buf[i] % n_len;
            cp_buf[i] = (unsigned char)ca_ascii[n_index];
        }
        cp_buf[n_sz - 1] = '\0';
        st_ret = ST_OK;
    }
    return st_ret;
}

/** @brief Read and validate client header and password.
 *
 *  Called when client has not been authenticated each time data is available
 *  on the external socket.
 *
 *  @param [in] p_extconn pointer
 *  @return AUTHRET_OK if successful, otherwise another auth_ret_t value.
 */
STATUS authpam_client_handshake(Session* session, ExtNet* net_state,
                                extnet_conn_t* p_extconn)
{
    static char ca_buf[10240];
    static time_t t_lockout = 0;
    bool b_data_pending = false;
    int n_read;
    auth_ret_t ret;
    auth_handshake_req_t lockout_req;

    if (!session || !net_state || !p_extconn)
    {
        return ST_ERR;
    }

    // We only need this for b_lockout but always do it so timing will be
    // same
    lockout_req.auth_hdr_version = AUTH_HDR_VERSION;
    if (ST_OK != get_random_string(lockout_req.auth_password,
                                   sizeof(lockout_req.auth_password)))
    {
        ret = AUTHRET_INVALIDDATA;
    }
    else
    {
        if ((n_read = extnet_recv(net_state, p_extconn, ca_buf, sizeof(ca_buf),
                                  &b_data_pending)) <= 0)
        {
            ret = AUTHRET_INVALIDDATA;
        }
        else
        {
            if (time(0) < t_lockout)
            {
                // Validate a random password to keep timing the
                // same.
                authenticate_client((char*)&lockout_req, 20);
                ret = AUTHRET_LOCKOUT;
            }
            else if (b_data_pending)
            {
                // more data should not be pending at this
                // point. Treat this as an error.
                ret = AUTHRET_INVALIDDATA;
            }
            else
            {
                // Validate the header and password as read.
                ret = authenticate_client(ca_buf, n_read);
            }
        }
        explicit_bzero(&ca_buf, sizeof(ca_buf));
    }
    switch (ret)
    {
        case AUTHRET_OK:
            // if session_get_authenticated_conn returns ST_ERR, then
            // there is no current session, which is what we want.
            if (ST_OK != session_get_authenticated_conn(session, NULL))
            {
                if (auth_handshake_response(net_state, p_extconn,
                                            AUTH_HANDSHAKE_SUCCESS) != ST_OK)
                    ret = AUTHRET_SYSERR;
            }
            else
            {
                auth_handshake_response(net_state, p_extconn,
                                        AUTH_HANDSHAKE_BUSY);
                ret = AUTHRET_UNAUTHORIZED;
            }
            break;
        case AUTHRET_SYSERR:
            auth_handshake_response(net_state, p_extconn,
                                    AUTH_HANDSHAKE_SYSERR);
            break;
        case AUTHRET_LOCKOUT:
            t_lockout = time(0L) + INVALID_AUTH_LOCKOUT_NSECS;
            // intentional fallthrough
        case AUTHRET_UNAUTHORIZED:
        case AUTHRET_INVALIDDATA:
        default:
            auth_handshake_response(net_state, p_extconn,
                                    AUTH_HANDSHAKE_FAILURE);
            break;
    }
    return (ret == AUTHRET_OK ? ST_OK : ST_ERR);
}
