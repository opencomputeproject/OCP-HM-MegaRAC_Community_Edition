// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <mtd/mtd-abi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common.h"
#include "mboxd.h"
#include "backend.h"

#include "test/tmpf.h"

struct tmpf _tmp, *tmp = &_tmp;

void cleanup(void)
{
	tmpf_destroy(tmp);
}

char *get_dev_mtd(void)
{
	int rc;

	rc = tmpf_init(tmp, "flash-store.XXXXXX");
	if (rc < 0)
		return NULL;

	return tmp->path;
}

#define MEM_SIZE 3
#define ERASE_SIZE 1

int ioctl(int fd, unsigned long request, ...)
{
	va_list ap;

	if (request != MEMGETINFO)
		return -1;

	struct mtd_info_user *info;

	va_start(ap, request);
	info = va_arg(ap, struct mtd_info_user *);
	info->size = MEM_SIZE;
	info->erasesize = ERASE_SIZE;
	va_end(ap);

	return 0;
}

int main(void)
{
	struct mbox_context _context, *context = &_context;
	struct backend *backend = &context->backend;
	char src[MEM_SIZE];
	uint8_t *map;
	int rc;

	atexit(cleanup);

	mbox_vlog = &mbox_log_console;

	rc = backend_probe_mtd(backend, get_dev_mtd());
	assert(rc == 0);

	map = mmap(NULL, MEM_SIZE, PROT_READ, MAP_PRIVATE, tmp->fd, 0);
	assert(map != MAP_FAILED);

	memset(src, 0xaa, sizeof(src));
	rc = backend_write(backend, 0, src, sizeof(src));
	assert(rc == 0);
	rc = memcmp(src, map, sizeof(src));
	assert(rc == 0);

	memset(src, 0x55, sizeof(src));
	rc = backend_write(backend, 0, src, sizeof(src));
	assert(rc == 0);
	rc = memcmp(src, map, sizeof(src));
	assert(rc == 0);

	src[0] = 0xff;
	rc = backend_write(backend, 0, src, 1);
	assert(rc == 0);
	rc = memcmp(src, map, sizeof(src));
	assert(rc == 0);

	src[1] = 0xff;
	rc = backend_write(backend, 1, &src[1], 1);
	assert(rc == 0);
	rc = memcmp(src, map, sizeof(src));
	assert(rc == 0);

	src[2] = 0xff;
	rc = backend_write(backend, 2, &src[2], 1);
	assert(rc == 0);
	rc = memcmp(src, map, sizeof(src));
	assert(rc == 0);

	backend_free(backend);

	return rc;
}
