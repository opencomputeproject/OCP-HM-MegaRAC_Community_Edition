// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mboxd.h"
#include "protocol.h"
#include "transport_mbox.h"

#include "test/mbox.h"
#include "test/system.h"

#define FLAGS 0xc3

static const uint8_t get_info[] = {
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t command[] = {
	0x09, 0xaa, FLAGS, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, FLAGS
};

uint8_t data[3] = { 0xaa, 0x55, 0xaa };

#define MEM_SIZE	3
#define ERASE_SIZE	1
#define N_WINDOWS	1
#define WINDOW_SIZE	1

int main(void)
{
	struct mbox_context *ctx;
	struct stat details;
	uint8_t *map;
	int rc;

	system_set_reserved_size(MEM_SIZE);
	system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

	ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
	rc = mbox_set_mtd_data(ctx, data, sizeof(data));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
	assert(rc == 1);

	protocol_events_set(ctx, FLAGS);

	rc = mbox_command_dispatch(ctx, command, sizeof(command));
	assert(rc == 1);

	rc = fstat(ctx->fds[MBOX_FD].fd, &details);
	assert(rc == 0);

	map = mmap(NULL, details.st_size, PROT_READ, MAP_PRIVATE,
			ctx->fds[MBOX_FD].fd, 0);
	assert(map != MAP_FAILED);

	assert(details.st_size >= 16);
	assert(map[15] == 0xc0);

	return 0;
}
