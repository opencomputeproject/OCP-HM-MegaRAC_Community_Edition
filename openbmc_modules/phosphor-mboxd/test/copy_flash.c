// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "mbox.h"
#include "mboxd_flash.h"

#include "test/tmpf.h"

#define TEST_SIZE 4096

static struct tmpf tmp;

void cleanup(void)
{
	tmpf_destroy(&tmp);
}

int main(void)
{
	struct mbox_context context;
	ssize_t processed;
	int rand_fd;
	char *src;
	char *dst;
	int rc;

	atexit(cleanup);

	mbox_vlog = &mbox_log_console;

	src = malloc(TEST_SIZE);
	dst = malloc(TEST_SIZE);
	if (!(src && dst)) {
		rc = -1;
		goto free;
	}

	rand_fd = open("/dev/urandom", O_RDONLY);
	if (rand_fd < 0) {
		rc = rand_fd;
		goto free;
	}

	rc = tmpf_init(&tmp, "flash-store.XXXXXX");
	if (rc < 0)
		goto free;

	processed = read(rand_fd, src, TEST_SIZE);
	if (processed != TEST_SIZE) {
		rc = -1;
		goto free;
	}

	processed = write(tmp.fd, src, TEST_SIZE);
	if (processed != TEST_SIZE) {
		rc = -1;
		goto free;
	}

	context.fds[MTD_FD].fd = tmp.fd;

	copy_flash(&context, 0, dst, TEST_SIZE);
	assert(0 == memcmp(src, dst, TEST_SIZE));

free:
	free(src);
	free(dst);

	return rc;
}
