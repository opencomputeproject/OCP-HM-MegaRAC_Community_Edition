// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <string.h>

#include "config.h"
#include "transport_mbox.h"
#include "windows.h"

#include "test/mbox.h"
#include "test/system.h"

struct test_context
{
    uint8_t seq;
    struct mbox_context *ctx;
};

// Configure the system and the paritions such that we eventually request a
// window that covers the last section of flash, but the remaining flash is
// smaller than the window size
#define BLOCK_SIZE	4096
#define ERASE_SIZE	BLOCK_SIZE
#define N_WINDOWS	3
#define WINDOW_SIZE	BLOCK_SIZE
#define MEM_SIZE	(N_WINDOWS * WINDOW_SIZE)
#define PNOR_SIZE	((N_WINDOWS + 1) * WINDOW_SIZE)

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

#define MBOX_CREATE_READ_WINDOW 4

static int mbox_create_read_window(struct test_context *tctx, size_t offset,
                                   size_t len)
{
    union mbox_regs regs;

    memset(&regs, 0, sizeof(regs));
    regs.msg.command = MBOX_CREATE_READ_WINDOW;
    regs.msg.seq = ++tctx->seq;
    put_u16(&regs.msg.args[0], offset);
    put_u16(&regs.msg.args[2], len);

    return mbox_command_dispatch(tctx->ctx, regs.raw, sizeof(regs.raw));
}

int main()
{
    struct test_context _tctx = {0}, *tctx = &_tctx;
    size_t len;
    size_t pos;
    int rc;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    tctx->ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);

    rc = mbox_command_dispatch(tctx->ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    pos = 0;
    while (pos < ((PNOR_SIZE - BLOCK_SIZE) / BLOCK_SIZE))
    {
        struct mbox_msg _msg, *msg = &_msg;

        rc = mbox_create_read_window(tctx, pos, (WINDOW_SIZE / BLOCK_SIZE));
        assert(rc == 1);

        mbox_rspcpy(tctx->ctx, msg);

        len = get_u16(&msg->args[2]);
        pos = get_u16(&msg->args[4]) + len;
    }

    windows_reset_all(tctx->ctx);

    rc = mbox_create_read_window(tctx, pos, (WINDOW_SIZE / BLOCK_SIZE));
    assert(rc == 1);

    return 0;
}
