// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <sys/mman.h>

#include "mboxd.h"
#include "mtd/backend.h"
#include "transport_mbox.h"

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

static const uint8_t mark_write_erased[] = {
	0x0a, 0x02, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t response[] = {
	0x0a, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const uint8_t write_flush[] = {
	0x08, 0x03, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t start_data[] = { 0xaa, 0x55, 0xaa };
const uint8_t finish_data[] = { 0xaa, 0xff, 0xaa };

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

	rc = mbox_command_dispatch(ctx, mark_write_erased,
			sizeof(mark_write_erased));
	assert(rc == 1);

	rc = mbox_cmp(ctx, response, sizeof(response));
	assert(rc == 0);

	rc = memcmp(ctx->mem, finish_data, sizeof(finish_data));
	assert(rc == 0);

	map = mmap(NULL, MEM_SIZE, PROT_READ, MAP_PRIVATE,
			((struct mtd_data *)ctx->backend.priv)->fd, 0);
	assert(map != MAP_FAILED);

	rc = memcmp(start_data, map, sizeof(start_data));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, write_flush, sizeof(write_flush));
	assert(rc == 1);

	rc = memcmp(finish_data, map, sizeof(finish_data));
	assert(rc == 0);

	return rc;
};
