// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <string.h>

#include "config.h"
#include "mboxd_msg.h"
#include "vpnor/mboxd_pnor_partition_table.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

struct test_context
{
    uint8_t seq;
    struct mbox_context *ctx;
};

// Configure the system and the paritions such that we eventually request a
// window that covers the last section of flash, but the remaining flash is
// smaller than the window size
static constexpr auto BLOCK_SIZE = 4096;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 3;
static constexpr auto WINDOW_SIZE = 2 * BLOCK_SIZE;
static constexpr auto MEM_SIZE = N_WINDOWS * WINDOW_SIZE;
static constexpr auto PNOR_SIZE = (4 * BLOCK_SIZE);

const std::string toc[] = {
    "partition01=ONE,00001000,00003000,80,ECC,READONLY",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

static constexpr auto MBOX_CREATE_READ_WINDOW = 4;

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
    namespace test = openpower::virtual_pnor::test;

    struct test_context _tctx = {0}, *tctx = &_tctx;
    size_t len;
    size_t pos;
    int rc;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    tctx->ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(tctx->ctx, toc, BLOCK_SIZE);
    init_vpnor_from_paths(tctx->ctx);

    rc = mbox_command_dispatch(tctx->ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    pos = 0;
    while (pos < (PNOR_SIZE / BLOCK_SIZE))
    {
        struct mbox_msg _msg, *msg = &_msg;

        rc = mbox_create_read_window(tctx, pos, (WINDOW_SIZE / BLOCK_SIZE));
        assert(rc == 1);

        mbox_rspcpy(tctx->ctx, msg);

        len = get_u16(&msg->args[2]);
        pos = get_u16(&msg->args[4]) + len;
    }

    return 0;
}
