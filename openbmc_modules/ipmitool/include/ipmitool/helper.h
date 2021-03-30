/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef IPMI_HELPER_H
#define IPMI_HELPER_H

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef tboolean
#define tboolean   int
#endif

/* IPMI spec. - UID 0 reserved, 63 maximum UID which can be used */
#ifndef IPMI_UID_MIN
# define IPMI_UID_MIN 1
#endif
#ifndef IPMI_UID_MAX
# define IPMI_UID_MAX 63
#endif

struct ipmi_intf;

struct valstr {
	uint16_t val;
	const char * str;
};
struct oemvalstr {
	uint32_t oem;
   uint16_t val;
	const char * str;
};

const char * val2str(uint16_t val, const struct valstr * vs);
const char * oemval2str(uint32_t oem,uint16_t val, const struct oemvalstr * vs);

int str2double(const char * str, double * double_ptr);
int str2long(const char * str, int64_t * lng_ptr);
int str2ulong(const char * str, uint64_t * ulng_ptr);
int str2int(const char * str, int32_t * int_ptr);
int str2uint(const char * str, uint32_t * uint_ptr);
int str2short(const char * str, int16_t * shrt_ptr);
int str2ushort(const char * str, uint16_t * ushrt_ptr);
int str2char(const char * str, int8_t * chr_ptr);
int str2uchar(const char * str, uint8_t * uchr_ptr);

int eval_ccode(const int ccode);

int is_fru_id(const char *argv_ptr, uint8_t *fru_id_ptr);
int is_ipmi_channel_num(const char *argv_ptr, uint8_t *channel_ptr);
int is_ipmi_user_id(const char *argv_ptr, uint8_t *ipmi_uid_ptr);
int is_ipmi_user_priv_limit(const char *argv_ptr, uint8_t *ipmi_priv_limit_ptr);

uint16_t str2val(const char * str, const struct valstr * vs);
void print_valstr(const struct valstr * vs, const char * title, int loglevel);
void print_valstr_2col(const struct valstr * vs, const char * title, int loglevel);


uint16_t buf2short(uint8_t * buf);
uint32_t buf2long(uint8_t * buf);
const char * buf2str(uint8_t * buf, int len);
void printbuf(const uint8_t * buf, int len, const char * desc);
uint8_t ipmi_csum(uint8_t * d, int s);
FILE * ipmi_open_file(const char * file, int rw);
void ipmi_start_daemon(struct ipmi_intf *intf);
uint16_t ipmi_get_oem_id(struct ipmi_intf *intf);

#define ipmi_open_file_read(file)	ipmi_open_file(file, 0)
#define ipmi_open_file_write(file)	ipmi_open_file(file, 1)

#ifndef __min
# define __min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef __max
# define __max(a, b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef __minlen
# define __minlen(a, b) ({ int x=strlen(a); int y=strlen(b); (x < y) ? x : y;})
#endif

#ifndef __maxlen
# define __maxlen(a, b) ({ int x=strlen(a); int y=strlen(b); (x > y) ? x : y;})
#endif

#endif /* IPMI_HELPER_H */
