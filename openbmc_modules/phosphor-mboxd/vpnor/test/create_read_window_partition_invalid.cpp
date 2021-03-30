// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include <assert.h>
#include <string.h>

#include "config.h"
#include "vpnor/mboxd_pnor_partition_table.h"

extern "C" {
#include "test/mbox.h"
#include "test/system.h"
}

#include "vpnor/test/tmpd.hpp"

const std::string toc[] = {
    "partition01=HBB,00002000,00003000,80,ECC,READONLY",
};

static constexpr auto BLOCK_SIZE = 4096;
static constexpr auto MEM_SIZE = BLOCK_SIZE * 2;
static constexpr auto ERASE_SIZE = BLOCK_SIZE;
static constexpr auto N_WINDOWS = 1;
static constexpr auto WINDOW_SIZE = BLOCK_SIZE;

static const uint8_t get_info[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

/* Request access below the specified partition */
static const uint8_t create_read_window[] = {0x04, 0x01, 0x01, 0x00, 0x01, 0x00,
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};

int main()
{
    namespace test = openpower::virtual_pnor::test;

    struct mbox_context *ctx;
    int rc;

    system_set_reserved_size(MEM_SIZE);
    system_set_mtd_sizes(MEM_SIZE, ERASE_SIZE);

    ctx = mbox_create_test_context(N_WINDOWS, WINDOW_SIZE);
    test::VpnorRoot root(ctx, toc, BLOCK_SIZE);
    init_vpnor_from_paths(ctx);

    rc = mbox_command_dispatch(ctx, get_info, sizeof(get_info));
    assert(rc == 1);

    rc = mbox_command_dispatch(ctx, create_read_window,
                               sizeof(create_read_window));
    return !(rc == 1);
}
