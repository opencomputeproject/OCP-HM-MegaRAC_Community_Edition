/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef TEST_MBOX_H
#define TEST_MBOX_H

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "mboxd.h"
#include "transport_mbox.h"

#include "tmpf.h"

struct mbox_context *mbox_create_test_context(int n_windows, size_t len);
struct mbox_context *mbox_create_frontend_context(int n_windows, size_t len);

int mbox_set_mtd_data(struct mbox_context *context, const void *data,
		size_t len);

void mbox_dump(struct mbox_context *context);

void mbox_rspcpy(struct mbox_context *context, struct mbox_msg *msg);

int mbox_cmp(struct mbox_context *context, const uint8_t *expected, size_t len);

int mbox_command_write(struct mbox_context *context, const uint8_t *command,
		size_t len);

int mbox_command_dispatch(struct mbox_context *context, const uint8_t *command,
	size_t len);

/* Helpers */
void dump_buf(const void *buf, size_t len);

#endif /* TEST_MBOX_H */
