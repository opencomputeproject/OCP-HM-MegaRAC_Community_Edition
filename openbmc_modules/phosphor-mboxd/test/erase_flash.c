// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/types.h>

#include "mbox.h"
#include "mboxd_flash.h"

#include "test/tmpf.h"

static struct tmpf mtd;

void cleanup_mtd(void)
{
	tmpf_destroy(&mtd);
}

char *get_dev_mtd(void)
{
	int rc;

	rc = tmpf_init(&mtd, "flash-store.XXXXXX");
	if (rc < 0)
		return NULL;

	return strdup(mtd.path);
}

struct erase_info_user *recorded;
int n_ioctls;

#define MEM_SIZE 3
#define ERASE_SIZE 1

int ioctl(int fd, unsigned long request, ...)
{
	va_list ap;
	struct erase_info_user *provided, *alloced;

	if (!(request == MEMERASE || request == MEMGETINFO)) {
		printf("Uh-oh: ioctl() called with request 0x%08lx\n", request);
		return -1;
	}

	switch (request) {
	case MEMGETINFO:
	{
		struct mtd_info_user *info;

		va_start(ap, request);
		info = va_arg(ap, struct mtd_info_user *);
		info->size = MEM_SIZE;
		info->erasesize = ERASE_SIZE;
		va_end(ap);
		break;
	}
	case MEMERASE:
		va_start(ap, request);
		provided = va_arg(ap, struct erase_info_user *);

		n_ioctls++;

		alloced = realloc(recorded, n_ioctls * sizeof(*recorded));
		if (!alloced)
			return -1;
		recorded = alloced;

		memcpy(&recorded[n_ioctls - 1], provided, sizeof(*provided));

		va_end(ap);
		break;
	default:
		break;
	}

	return 0;
}

void dump_ioctls(void)
{
	int i;

	printf("n_ioctls: %d\n", n_ioctls);

	for (i = 0; i < n_ioctls; i++)
		printf("%d: start: %d, length %d\n",
				i, recorded[i].start, recorded[i].length);
}

int main(void)
{
	struct mbox_context context;
	char data[MEM_SIZE];
	int rc;

	rc = atexit(cleanup_mtd);
	if (rc)
		return rc;

	mbox_vlog = &mbox_log_console;

	n_ioctls = 0;
	recorded = NULL;

	init_flash_dev(&context);

	/* Erase from an unknown state */
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 1);
	assert(recorded[0].start == 0);
	assert(recorded[0].length == sizeof(data));

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	/* Erase an erased flash */
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 0);

	memset(data, 0xaa, sizeof(data));

	/* Erase written flash */
	rc = write_flash(&context, 0, data, sizeof(data));
	assert(rc == 0);
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 1);
	assert(recorded[0].start == 0);
	assert(recorded[0].length == sizeof(data));

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	/* Erase the start of flash */
	rc = write_flash(&context, 0, data, sizeof(data) - 1);
	assert(rc == 0);
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 1);
	assert(recorded[0].start == 0);
	assert(recorded[0].length == sizeof(data) - 1);

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	/* Erase the end of flash */
	rc = write_flash(&context, 1, data, sizeof(data) - 1);
	assert(rc == 0);
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 1);
	assert(recorded[0].start == 1);
	assert(recorded[0].length == sizeof(data) - 1);

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	/* Erase each end of flash */
	rc = write_flash(&context, 0, data, 1);
	rc = write_flash(&context, 2, data, 1);
	assert(rc == 0);
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 2);
	assert(recorded[0].start == 0);
	assert(recorded[0].length == 1);
	assert(recorded[1].start == 2);
	assert(recorded[1].length == 1);

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	/* Erase the middle of flash */
	rc = write_flash(&context, 1, data, 1);
	assert(rc == 0);
	rc = erase_flash(&context, 0, sizeof(data));

	assert(rc == 0);
	assert(n_ioctls == 1);
	assert(recorded[0].start == 1);
	assert(recorded[0].length == 1);

	free(recorded);
	recorded = NULL;
	n_ioctls = 0;

	free_flash_dev(&context);

	return rc;
}
