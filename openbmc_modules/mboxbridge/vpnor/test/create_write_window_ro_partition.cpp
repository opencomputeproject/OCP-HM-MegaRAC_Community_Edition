// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "config.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

#include <cassert>

#include "vpnor/backend.h"

const std::string toc[] = {
    "partition01=HBB,00001000,00002000,80,ECC,READONLY",
};

static const auto BLOCK_SIZE = 4096;
static const auto MEM_SIZE = BLOCK_SIZE * 2;
static const auto ERASE_SIZE = BLOCK_SIZE;
static const auto N_WINDOWS = 1;
static const auto WINDOW_SIZE = BLOCK_SIZE;

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

// offset 0x100 and size 6
static const uint8_t create_write_window[] = {
    0x06, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t response[] = {0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07};

namespace test = openpower::virtual_pnor::test;

int main()
{
    struct mbox_context* ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

    ctx = mbox_create_frontend_context(N_WINDOWS, WINDOW_SIZE);

    test::VpnorRoot root(&ctx->backend, toc, BLOCK_SIZE);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    rc = mbox_command_dispatch(ctx, create_write_window,
                               sizeof(create_write_window));
    assert(rc == 7);

    rc = mbox_cmp(ctx, response, sizeof(response));
    assert(rc == 0);

    return rc;
}
