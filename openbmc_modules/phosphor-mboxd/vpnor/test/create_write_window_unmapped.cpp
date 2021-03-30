// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>

#include "config.h"
#include "vpnor/mboxd_pnor_partition_table.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

static constexpr auto BLOCK_SIZE = 0x1000;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = BLOCK_SIZE;
static constexpr auto MEM_SIZE = WINDOW_SIZE;
static constexpr auto PNOR_SIZE = 3 * BLOCK_SIZE;

const std::string toc[] = {
    "partition01=HBB,00001000,00002000,80,ECC,READWRITE",
};

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

// offset 0x100 and size 6
static const uint8_t create_write_window[] = {
    0x06, 0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t response[] = {0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07};

namespace test = openpower::virtual_pnor::test;

int main()
{
    struct mbox_context *ctx;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(PNOR_SIZE, ERASE_SIZE);

    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);

    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);

    init_vpnor_from_paths(ctx);

    int rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == MBOX_R_SUCCESS);

    rc = mbox_command_dispatch(ctx, create_write_window,
                               sizeof(create_write_window));
    assert(rc == MBOX_R_WINDOW_ERROR);

    rc = mbox_cmp(ctx, response, sizeof(response));
    assert(rc == 0);

    return rc;
}
