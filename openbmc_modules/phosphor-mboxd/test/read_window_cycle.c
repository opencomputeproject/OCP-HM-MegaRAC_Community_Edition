// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>

#include "mbox.h"
#include "mboxd_msg.h"

#include "test/mbox.h"
#include "test/system.h"

static const uint8_t get_info[] = {
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t create_read_window_block_0[] = {
	0x04, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t create_read_window_block_1[] = {
	0x04, 0x02, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t create_read_window_block_2[] = {
	0x04, 0x03, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t data[] = { 0x00, 0x01, 0x02 };

#define MEM_SIZE	sizeof(data)
#define ERASE_SIZE	1
#define N_WINDOWS	MEM_SIZE - 1
#define WINDOW_SIZE	1

int main(void)
{
	struct mbox_context *ctx;
	int rc;
	int i;

	system_set_reserved_size(N_WINDOWS);
	system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

	ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
	rc = mbox_set_mtd_data(ctx, data, sizeof(data));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
	assert(rc == 1);

	/* Make each block appear in each window */
	for (i = 0; i < 2; i++) {
		rc = mbox_command_dispatch(ctx, create_read_window_block_0,
				sizeof(create_read_window_block_0));
		assert(rc == 1);
		assert(((uint8_t *)ctx->mem)[(0 + i) & 1] == data[0]);

		rc = mbox_command_dispatch(ctx, create_read_window_block_1,
				sizeof(create_read_window_block_1));
		assert(rc == 1);
		assert(((uint8_t *)ctx->mem)[(1 + i) & 1] == data[1]);

		rc = mbox_command_dispatch(ctx, create_read_window_block_2,
				sizeof(create_read_window_block_2));
		assert(rc == 1);
		assert(((uint8_t *)ctx->mem)[(2 + i) & 1] == data[2]);
	}

	return !(rc == 1);
};
