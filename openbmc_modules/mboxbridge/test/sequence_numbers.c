// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>

#include "mboxd.h"
#include "transport_mbox.h"

#include "test/mbox.h"
#include "test/system.h"

static const uint8_t get_mbox_info[] = {
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t get_mbox_info_response[] = {
	0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
};

static const uint8_t get_flash_info0[] = {
	0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t get_flash_info_response0[] = {
	0x03, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static const uint8_t get_flash_info1[] = {
	0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t get_flash_info_response1[] = {
	0x03, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

#define MEM_SIZE	3
#define ERASE_SIZE	1
#define N_WINDOWS	1
#define WINDOW_SIZE	1

int main(void)
{
	struct mbox_context *ctx;
	int rc;

	system_set_reserved_size(MEM_SIZE);
	system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

	ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);

	/* Consecutive GET_MBOX_INFOs can use "invalid" sequence numbers */
	rc = mbox_command_dispatch(ctx, get_mbox_info, sizeof(get_mbox_info));
	assert(rc == 1);

	rc = mbox_cmp(ctx, get_mbox_info_response,
			sizeof(get_mbox_info_response));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, get_mbox_info, sizeof(get_mbox_info));
	assert(rc == 1);

	/* Other commands must use valid sequence numbers */
	rc = mbox_command_dispatch(ctx, get_flash_info0,
			sizeof(get_flash_info0));
	assert(rc == 1);

	rc = mbox_cmp(ctx, get_flash_info_response0,
			sizeof(get_flash_info_response0));
	assert(rc == 0);

	rc = mbox_command_dispatch(ctx, get_flash_info0,
			sizeof(get_flash_info0));
	assert(rc == 8);

	/* Retry with a "valid" sequence number */
	rc = mbox_command_dispatch(ctx, get_flash_info1,
			sizeof(get_flash_info1));
	assert(rc == 1);

	rc = mbox_cmp(ctx, get_flash_info_response1,
			sizeof(get_flash_info_response1));
	assert(rc == 0);

	return rc;
}
