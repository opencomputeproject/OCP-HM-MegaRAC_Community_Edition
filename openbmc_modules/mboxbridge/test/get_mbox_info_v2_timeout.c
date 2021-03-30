// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>

#include "mboxd.h"
#include "transport_mbox.h"

#include "test/mbox.h"
#include "test/system.h"

static const uint8_t command[] = {
	0x02, 0xaa, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t response[] = {
	0x02, 0xaa, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* Window size == 1MB -> suggested timeout == 8 (seconds) */
	0x08, 0x00, 0x00, 0x00, 0x00, 0x01,
};

/*
 * Suggested timeout is based on a milliseconds-per-MB constant. Thus to
 * get a non-zero value we need a window of size atleast 1MB.
 */
#define MEM_SIZE	1 << 20
#define ERASE_SIZE	1
#define N_WINDOWS	1
#define WINDOW_SIZE	1 << 20

int main(void)
{
	struct mbox_context *ctx;
	int rc;

	system_set_reserved_size(MEM_SIZE);
	system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

	ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);

	rc = mbox_command_dispatch(ctx, command, sizeof(command));
	assert(rc == 1);

	rc = mbox_cmp(ctx, response, sizeof(response));
	assert(rc == 0);

	return rc;
}
