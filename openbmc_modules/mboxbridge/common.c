// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#define _GNU_SOURCE
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>

#include "common.h"

void (*mbox_vlog)(int p, const char *fmt, va_list args);

enum verbose verbosity;

void mbox_log_console(int p, const char *fmt, va_list args)
{
	struct timespec time;
	FILE *s = (p < LOG_WARNING) ? stdout : stderr;

	clock_gettime(CLOCK_REALTIME, &time);

	fprintf(s, "[%s %ld.%.9ld] ", PREFIX, time.tv_sec, time.tv_nsec);

	vfprintf(s, fmt, args);

	if (s == stdout)
		fflush(s);
}

__attribute__((format(printf, 2, 3)))
void mbox_log(int p, const char *fmt, ...)
{
	static bool warned = false;
	va_list args;

	if (!mbox_vlog) {
		if (!warned) {
			fprintf(stderr, "Logging backend not configured, "
					"log output disabled\n");
			warned = true;
		}

		return;
	}

	va_start(args, fmt);
	mbox_vlog(p, fmt, args);
	va_end(args);
}

uint16_t get_u16(uint8_t *ptr)
{
	return le16toh(*(uint16_t *)ptr);
}

void put_u16(uint8_t *ptr, uint16_t val)
{
	val = htole16(val);
	memcpy(ptr, &val, sizeof(val));
}

uint32_t get_u32(uint8_t *ptr)
{
	return le32toh(*(uint32_t *)ptr);
}

void put_u32(uint8_t *ptr, uint32_t val)
{
	val = htole32(val);
	memcpy(ptr, &val, sizeof(val));
}

