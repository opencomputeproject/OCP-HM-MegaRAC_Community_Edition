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
 * @file auth_pam.h
 * @brief Functions and definitions to authenticate client credentials with PAM.
 */

#ifndef __AUTH_PAM_H_
#define __AUTH_PAM_H_

#include <security/pam_appl.h>

#include "authenticate.h"

extern auth_hdlrs_t authpam_hdlrs;

/** Auth Header version. */
#define AUTH_HDR_VERSION                                                       \
    0x30 /* Set to 30 to allow testing with ascii                              \
          * tools.                                                             \
          */

#define MAX_PW_LEN 128

/** PAM service (/etc/pam.d/service) establishing rules for auth */
#define ASD_PAM_SERVICE "asd"

/** User used for authentication. Only this user may authenticate */
#define ASD_PAM_USER "asdbg"

/** Number of invalid auth attempts permitted before lockout */
#define INVALID_AUTH_MAX_ATTEMPTS 3

/** Period of time for which invalid auth attempts are measured */
#define INVALID_AUTH_PERIOD_NSECS 60

/** Amount of time auth attempts are locked out when threshold is exceeded */
#define INVALID_AUTH_LOCKOUT_NSECS 60

/* Characters for the random string generator to use */
#define RANDOM_ASCII_CHARACTERS                                                \
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-!#$%()+" \
    "=*"

/** Header/Handshake responses */
typedef enum
{
    AUTH_HANDSHAKE_SUCCESS = 0x30,
    AUTH_HANDSHAKE_SYSERR = 0x24, // system error.
    AUTH_HANDSHAKE_BUSY = 0x2b,   // session already in progress
    AUTH_HANDSHAKE_FAILURE = 0x3f,
} __attribute__((packed)) auth_handshake_ret_t;

/** Structure passed from client to provide credentials for auth */
typedef struct
{
    /** Header version */
    unsigned char auth_hdr_version;

    /** Auth password*/
    unsigned char auth_password[MAX_PW_LEN];
} __attribute__((packed)) auth_handshake_req_t;

/** Structure returned to client with auth result */
typedef struct
{
    /** Header version */
    unsigned char svr_hdr_version;
    auth_handshake_ret_t result_code;
} __attribute__((packed)) auth_handshake_resp_t;

#ifdef UNIT_TESTING_ONLY
// expose callback for testing
int pam_conversation_function(int numMsg, const struct pam_message** msg,
                              struct pam_response** resp, void* appdata_ptr);
#endif

#endif
