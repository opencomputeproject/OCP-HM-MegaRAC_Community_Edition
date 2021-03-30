// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <sys/mman.h>

#include "mbox.h"
#include "mboxd_msg.h"

#include "test/mbox.h"
#include "test/system.h"

static const uint8_t get_info[] = {
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t create_write_window[] = {
	0x06, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_dirty_left[] = {
	0x07, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_dirty_right[] = {
	0x07, 0x03, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_erase_middle[] = {
	0x0a, 0x04, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_erase_left[] = {
	0x0a, 0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_erase_right[] = {
	0x0a, 0x06, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t mark_write_dirty_middle[] = {
	0x07, 0x07, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t write_flush[] = {
	0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t flush_response[] = {
	0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

const uint8_t start_data[] = { 0xaa, 0x55, 0xaa };
const uint8_t flush_dirty_erased_dirty_data[] = { 0x55, 0xff, 0x55 };
const uint8_t flush_erased_dirty_erased_data[] = { 0xff, 0x55, 0xff };

#define MEM_SIZE	sizeof(start_data)
#define ERASE_SIZE	1
#define N_WINDOWS	1
#define WINDOW_SIZE	sizeof(start_data)

int main(void)
{
	struct mbox_context *ctx;
	uint8_t *map;
	int rc;

	system_set_reserved_size(MEM_SIZE);
	system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

	ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
	rc = mbox_set_mtd_data(ctx, start_data, sizeof(start_data));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, create_write_window,
			sizeof(create_write_window));
	assert(rc == 1);

	/* { dirty, erased, dirty } */

	((uint8_t *)ctx->mem)[0] = 0x55;

	rc = mbox_command_dispatch(ctx, mark_write_dirty_left,
			sizeof(mark_write_dirty_left));
	assert(rc == 1);

	((uint8_t *)ctx->mem)[2] = 0x55;

	rc = mbox_command_dispatch(ctx, mark_write_dirty_right,
			sizeof(mark_write_dirty_right));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, mark_write_erase_middle,
			sizeof(mark_write_erase_middle));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, write_flush, sizeof(write_flush));
	assert(rc == 1);

	rc = mbox_cmp(ctx, flush_response, sizeof(flush_response));
	assert(rc == 0);

	map = mmap(NULL, MEM_SIZE, PROT_READ, MAP_PRIVATE,
			ctx->fds[MTD_FD].fd, 0);
	assert(map != MAP_FAILED);

	rc = memcmp(flush_dirty_erased_dirty_data, map,
			sizeof(flush_dirty_erased_dirty_data));
	assert(rc == 0);

	/* { erased, dirty, erased } */

	((uint8_t *)ctx->mem)[1] = 0x55;

	rc = mbox_command_dispatch(ctx, mark_write_dirty_middle,
			sizeof(mark_write_dirty_middle));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, mark_write_erase_left,
			sizeof(mark_write_erase_left));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, mark_write_erase_right,
			sizeof(mark_write_erase_right));
	assert(rc == 1);

	rc = mbox_command_dispatch(ctx, write_flush, sizeof(write_flush));
	assert(rc == 1);

	rc = mbox_cmp(ctx, flush_response, sizeof(flush_response));
	assert(rc == 0);

	rc = memcmp(flush_erased_dirty_erased_data, map,
			sizeof(flush_erased_dirty_erased_data));
	assert(rc == 0);

	return rc;
}
